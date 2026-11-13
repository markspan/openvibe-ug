// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CSignalDisplayController.h"

#include "CSignalTraceItem.h"
#include "TracePalette.h"  // UG fork

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QVariantMap>

#include <algorithm>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {

/** Repaint cadence. The player pushes chunks far faster than this; the tick is what decouples them. */
constexpr int TICK_INTERVAL_MS = 16;
/** Upper bound on retained markers, so a long run cannot grow the list without limit. */
constexpr size_t MAX_STIMULATIONS = 512;
/**
 * UG fork: how often the automatic scale is worked out while the first page fills. After that it is worked out once
 * per page, but the first page would otherwise be drawn for its whole length at a scale guessed before any data.
 */
constexpr qint64 FIRST_PAGE_RESCALE_MS = 500;

// UG fork: paletteColor() is in TracePalette.h, shared with the LSL display

}  // namespace

CSignalDisplayController::CSignalDisplayController(Kernel::ILogManager& logManager, const SDisplaySettings& initial, QQmlEngine* engine)
    : m_log(logManager), m_settings(initial), m_engine(engine)
{
    m_buffer.setWindowSeconds(m_settings.timeScaleSeconds);  // UG fork: the buffer keeps it, see CTraceBuffer::reconfigure()
    buildParameters();
    buildQml(engine);

    m_sinceScaleRefresh.start();
    m_tick.setInterval(TICK_INTERVAL_MS);
    m_tick.setTimerType(Qt::PreciseTimer);
    connect(&m_tick, &QTimer::timeout, this, &CSignalDisplayController::onTick);
    m_tick.start();
}

CSignalDisplayController::~CSignalDisplayController()
{
    m_tick.stop();

    // m_buffer is a member and dies with this object, but the trace item holds a pointer to it.
    // deleteLater() would leave the item alive until the event loop next runs, and a render in
    // that window would lock an already-destroyed mutex, so the item has to go now.
    if (m_trace)
    {
        m_trace->setBuffer(nullptr);
        m_trace = nullptr;
    }

    // WindowManagerNodeComponent.qml reparents the item with `node.visual.parent = runtimeBox`,
    // and for a QQuickItem that also makes runtimeBox its QObject parent. Its cleanup is guarded
    // by `if (node?.visual != null)`, but uninitialize() has already cleared that through
    // clearVisualizationItem(), so QML skips the unparenting and still believes it owns the item.
    // Take ownership back explicitly before deleting, and let the QPointer tell us whether QML got
    // there first -- otherwise the second play of a scenario deletes it twice.
    if (m_root)
    {
        m_root->setParentItem(nullptr);
        m_root->setParent(nullptr);
        delete m_root.data();
    }
    m_root = nullptr;
    for (Parameters::ParameterObject* parameter : m_parameters) { delete parameter; }
    m_parameters.clear();
}

void CSignalDisplayController::setName(const QString& name)
{
    if (m_name == name) { return; }
    m_name = name;
    emit nameChanged();
}

//------------------------------------------------------------------------------------------------
// QML construction
//------------------------------------------------------------------------------------------------

void CSignalDisplayController::buildQml(QQmlEngine* engine)
{
    if (engine == nullptr)
    {
        m_log << Kernel::LogLevel_Error << "Signal display: no QML engine available\n";
        return;
    }

    auto* context = new QQmlContext(engine->rootContext());
    context->setContextProperty("dataSource", this);

    QQmlComponent component(engine);
    component.loadFromModule("OpenViBE.SignalDisplay", "SignalDisplay");
    if (component.isError())
    {
        for (const QQmlError& error : component.errors())
        {
            m_log << Kernel::LogLevel_Error << "Signal display QML: " << error.toString().toUtf8().constData() << "\n";
        }
        delete context;
        return;
    }

    QObject* object = component.create(context);
    m_root          = qobject_cast<QQuickItem*>(object);
    if (m_root == nullptr)
    {
        m_log << Kernel::LogLevel_Error << "Signal display: SignalDisplay.qml did not yield a QQuickItem\n";
        delete object;
        delete context;
        return;
    }
    context->setParent(m_root);
    QQmlEngine::setObjectOwnership(m_root, QQmlEngine::CppOwnership);

    m_trace = m_root->findChild<CSignalTraceItem*>(QStringLiteral("signalTrace"));
    if (m_trace == nullptr)
    {
        m_log << Kernel::LogLevel_Error << "Signal display: no SignalTrace named 'signalTrace' in SignalDisplay.qml\n";
        return;
    }
    m_trace->setBuffer(&m_buffer);
    m_trace->setSettings(m_settings);
}

QJSValue CSignalDisplayController::parameters() const
{
    if (m_engine == nullptr) { return {}; }

    const int nbParams  = int(m_parameters.size());
    QJSValue  jsParams  = m_engine->newArray(nbParams);
    for (int i = 0; i < nbParams; ++i)
    {
        // The controller owns these; without this the JS collector would delete them too.
        QQmlEngine::setObjectOwnership(m_parameters[i], QQmlEngine::CppOwnership);
        jsParams.setProperty(i, m_engine->newQObject(m_parameters[i]));
    }
    return jsParams;
}

//------------------------------------------------------------------------------------------------
// Parameter panel: one table, seeded from and writing back to m_settings
//------------------------------------------------------------------------------------------------

void CSignalDisplayController::buildParameters()
{
    // UG fork: the names of the list entries come from SDisplaySettings, as do those of the box setting
    std::vector<std::string> timeModes;
    for (const ESignalDisplayMode mode : SDisplaySettings::TIME_MODES) { timeModes.emplace_back(SDisplaySettings::toString(mode)); }
    m_pDisplayMode = new Parameters::ParameterListStringObject("Display Mode", "Scan sweeps a cursor across a fixed axis; Scroll slides the data leftwards",
                                                              timeModes, SDisplaySettings::toString(m_settings.timeMode));
    connect(m_pDisplayMode, &Parameters::ParameterListStringObject::valueChanged, this, [this](const QString& value)
    {
        if (!SDisplaySettings::parseTimeMode(value.toStdString(), m_settings.timeMode)) { return; }
        if (m_trace != nullptr) { m_trace->setSettings(m_settings); }
        m_layoutDirty = true;
    });
    m_parameters.push_back(m_pDisplayMode);

    // UG fork: By type, and the scale is worked out again for every page
    std::vector<std::string> scalings;
    for (const ESignalDisplayScaling scaling : SDisplaySettings::SCALINGS) { scalings.emplace_back(SDisplaySettings::toString(scaling)); }
    m_pScaling = new Parameters::ParameterListStringObject("Auto vertical scale",
                                                          "Scale each lane on its own, the EEG, EOG and digital channels each together, all lanes "
                                                          "together, or not at all. An automatic scale is worked out again for every new page",
                                                          scalings, SDisplaySettings::toString(m_settings.scaling));
    connect(m_pScaling, &Parameters::ParameterListStringObject::valueChanged, this, [this](const QString& value)
    {
        if (!SDisplaySettings::parseScaling(value.toStdString(), m_settings.scaling)) { return; }
        if (m_trace != nullptr) { m_trace->setSettings(m_settings); }
        // UG fork: the next tick works the scale out, for the new mode; upstream did it here as well
        m_rescale     = true;
        m_layoutDirty = true;
        updateEnabledParameters();
    });
    m_parameters.push_back(m_pScaling);

    m_pTimeScale = new Parameters::ParameterDoubleObject("Time Scale", "Duration displayed, in seconds", m_settings.timeScaleSeconds, 0.1, 600.0, 2);
    connect(m_pTimeScale, &Parameters::ParameterDoubleObject::valueChanged, this, [this](const double& value)
    {
        if (value <= 0.0) { return; }
        m_settings.timeScaleSeconds = value;
        m_buffer.setWindowSeconds(value);
        m_lastRenderedTotal = 0;
        m_rescale           = true;  // UG fork
        m_layoutDirty       = true;
    });
    m_parameters.push_back(m_pTimeScale);

    m_pVerticalScale = new Parameters::ParameterDoubleObject("Vertical Scale", "Signal units filling half a channel lane. Only used when Auto vertical scale is None",
                                                            m_settings.verticalScale, 1e-3, 1e6, 3);
    connect(m_pVerticalScale, &Parameters::ParameterDoubleObject::valueChanged, this, [this](const double& value)
    {
        if (value <= 0.0) { return; }
        m_settings.verticalScale = value;
        if (m_trace != nullptr) { m_trace->setSettings(m_settings); }
        m_layoutDirty = true;
    });
    m_parameters.push_back(m_pVerticalScale);

    m_pBaselineOffset = new Parameters::ParameterDoubleObject("Vertical Offset", "DC level removed from every sample before it is scaled. Only used when Auto vertical scale is None",
                                                             m_settings.baselineOffset,
                                                             -1e6, 1e6, 3);
    connect(m_pBaselineOffset, &Parameters::ParameterDoubleObject::valueChanged, this, [this](const double& value)
    {
        m_settings.baselineOffset = value;
        if (m_trace != nullptr) { m_trace->setSettings(m_settings); }
        m_layoutDirty = true;
    });
    m_parameters.push_back(m_pBaselineOffset);

    // UG fork: no "Scale refresh interval (secs)" here any more; the scale is worked out once per page

    m_pChannels = new Parameters::ParameterMultiSelectObject("Channels", "Channels to display", {});
    connect(m_pChannels, &Parameters::ParameterMultiSelectObject::valueChanged, this, [this](const QStringList&) { refreshVisibleChannels(); });
    m_parameters.push_back(m_pChannels);

    m_pChannelsPerPage = new Parameters::ParameterIntObject("Channels per page", "0 fits every channel, otherwise the view scrolls",
                                                           m_channelsPerPage, 0, 256);
    connect(m_pChannelsPerPage, &Parameters::ParameterIntObject::valueChanged, this, [this](const int& value)
    {
        m_channelsPerPage = std::max(0, value);
        emit settingsChanged();
    });
    m_parameters.push_back(m_pChannelsPerPage);

    m_pButterfly = new Parameters::ParameterBoolObject("Multiview", "Overlay every channel on a single baseline, instead of one lane each",
                                                      m_settings.layout == ELayoutMode::Butterfly);
    connect(m_pButterfly, &Parameters::ParameterBoolObject::valueChanged, this, [this](const bool& value)
    {
        m_settings.layout = value ? ELayoutMode::Butterfly : ELayoutMode::Stacked;
        if (m_trace) { m_trace->setSettings(m_settings); }
        m_layoutDirty = true;
        updateEnabledParameters();  // UG fork: Multiview greys out Channels per page
        // QML needs this: paging and the gutter behave differently on a single shared baseline.
        emit settingsChanged();
    });
    m_parameters.push_back(m_pButterfly);

    m_pChannelColors = new Parameters::ParameterBoolObject("Colour per channel", "Colour each trace instead of drawing them all alike", false);
    connect(m_pChannelColors, &Parameters::ParameterBoolObject::valueChanged, this, [this](const bool& value)
    {
        if (m_trace != nullptr) { m_trace->setPerChannelColors(value); }
    });
    m_parameters.push_back(m_pChannelColors);

    m_pTimeRuler = new Parameters::ParameterBoolObject("Bottom ruler", "Show the time ruler along the bottom", m_settings.showTimeRuler);
    connect(m_pTimeRuler, &Parameters::ParameterBoolObject::valueChanged, this, [this](const bool& value)
    {
        m_settings.showTimeRuler = value;
        emit settingsChanged();
    });
    m_parameters.push_back(m_pTimeRuler);

    m_pAmplitudeRuler = new Parameters::ParameterBoolObject("Left ruler", "Show the amplitude scale bar along the left", m_settings.showAmplitudeRuler);
    connect(m_pAmplitudeRuler, &Parameters::ParameterBoolObject::valueChanged, this, [this](const bool& value)
    {
        m_settings.showAmplitudeRuler = value;
        emit settingsChanged();
    });
    m_parameters.push_back(m_pAmplitudeRuler);

    updateEnabledParameters();
}

void CSignalDisplayController::updateEnabledParameters()
{
    // UG fork: the fixed scale and offset apply only without automatic scaling, and pages only to stacked lanes
    const bool fixedScale = m_settings.scaling == ESignalDisplayScaling::None;
    m_pVerticalScale->setEnabled(fixedScale);
    m_pBaselineOffset->setEnabled(fixedScale);
    m_pChannelsPerPage->setEnabled(m_settings.layout != ELayoutMode::Butterfly);
}

//------------------------------------------------------------------------------------------------
// Player thread
//------------------------------------------------------------------------------------------------

void CSignalDisplayController::pushHeader(const size_t channels, const double samplingRate, std::vector<std::string> labels)
{
    // UG fork: the ring is shaped here, at once, so that it takes the chunks that follow. Upstream shaped it at the
    // next tick, up to 16 ms later, and the chunks that came before were dropped for not matching it.
    m_buffer.reconfigure(channels, samplingRate);

    std::lock_guard<std::mutex> lock(m_metaMutex);
    m_pending.header   = true;
    m_pending.channels = channels;
    m_pending.labels   = std::move(labels);
}

void CSignalDisplayController::pushChunk(const double* buffer, const size_t channels, const size_t samples, const uint64_t startTime,
                                         const uint64_t endTime)
{
    // Deliberately the whole of the hot path: a short lock and a copy, no QObject, no allocation.
    m_buffer.appendChunk(buffer, channels, samples, startTime, endTime);
}

void CSignalDisplayController::pushStimulation(const uint64_t id, const uint64_t date, std::string name)
{
    std::lock_guard<std::mutex> lock(m_metaMutex);
    // The name is resolved here, once, and travels with the event: no id-keyed side map to get
    // out of step with the date-keyed lookup.
    m_pending.stims.push_back({ id, OVTimeToSeconds(date), std::move(name) });
    while (m_pending.stims.size() > MAX_STIMULATIONS) { m_pending.stims.pop_front(); }
}

void CSignalDisplayController::pushChannelUnits(std::vector<std::pair<std::string, std::string>> units)
{
    std::lock_guard<std::mutex> lock(m_metaMutex);
    m_pending.units    = true;
    m_pending.unitList = std::move(units);
}

//------------------------------------------------------------------------------------------------
// GUI thread
//------------------------------------------------------------------------------------------------

void CSignalDisplayController::applyPendingMetadata()
{
    SPending pending;
    {
        std::lock_guard<std::mutex> lock(m_metaMutex);
        if (!m_pending.header && !m_pending.units && m_pending.stims.empty()) { return; }
        pending = std::move(m_pending);
        m_pending = SPending();
    }

    if (pending.header)
    {
        m_channelNames.clear();
        m_channelNames.reserve(int(pending.channels));
        for (size_t i = 0; i < pending.channels; ++i)
        {
            const std::string& label = i < pending.labels.size() ? pending.labels[i] : std::string();
            QString            name  = label.empty() ? QStringLiteral("Channel %1").arg(i + 1) : QString::fromStdString(label);
            // UG fork: the selection goes by name, so two channels with one name were shown and hidden together. A name
            // that is taken gets the channel's number.
            if (m_channelNames.contains(name)) { name += QStringLiteral(" (%1)").arg(i + 1); }
            m_channelNames.append(name);
        }

        // Everything is selected until the user says otherwise.
        m_pChannels->setAvailableValues(m_channelNames);
        m_pChannels->setValue(m_channelNames);

        // UG fork: the kind of each channel, from its name, for scaling By type
        QVector<EChannelKind> kinds;
        kinds.reserve(m_channelNames.size());
        for (const QString& name : m_channelNames) { kinds.append(channelKind(name.toStdString())); }
        if (m_trace != nullptr) { m_trace->setChannelKinds(kinds); }

        rebuildChannelModel();
        refreshVisibleChannels();
        m_lastRenderedTotal = 0;
        m_rescale           = true;  // UG fork
        m_layoutDirty       = true;
    }

    if (pending.units)
    {
        // UG fork: kept, and shown by rebuildChannelModel() whenever the channels are known. Upstream dropped units
        // that came before the header, which can happen, as another box feeds the units input.
        m_channelUnits = std::move(pending.unitList);
        rebuildChannelModel();
    }

    if (!pending.stims.empty())
    {
        for (const SStimEvent& stim : pending.stims) { m_stims.push_back(stim); }
        while (m_stims.size() > MAX_STIMULATIONS) { m_stims.pop_front(); }
    }
}

void CSignalDisplayController::rebuildChannelModel()
{
    // Units decorate the channels; they never replace the names, which the channel selection goes by
    const auto unitText = [this](const int i)
    {
        if (i >= int(m_channelUnits.size())) { return QString(); }
        const auto& [unit, factor] = m_channelUnits[size_t(i)];
        const QString text         = QString::fromStdString(unit);
        return factor.empty() || factor == "Unspecified" ? text : QString::fromStdString(factor) + text;
    };

    m_channels.clear();
    m_channels.reserve(m_channelNames.size());
    for (int i = 0; i < m_channelNames.size(); ++i)
    {
        QVariantMap entry;
        entry["index"] = i;
        entry["name"]  = m_channelNames.at(i);
        entry["unit"]  = unitText(i);
        entry["color"] = paletteColor(i);
        m_channels.append(entry);
    }
    emit channelsChanged();
}

void CSignalDisplayController::refreshVisibleChannels()
{
    const QStringList selected = m_pChannels->value();

    QVector<int>    visible;
    QVector<QColor> colors;
    visible.reserve(m_channelNames.size());
    colors.reserve(m_channelNames.size());
    for (int i = 0; i < m_channelNames.size(); ++i)
    {
        if (!selected.contains(m_channelNames.at(i))) { continue; }
        visible.append(i);
        // UG fork: coloured by channel, as the Multiview legend is (rebuildChannelModel()), so that a channel keeps
        // its colour when others are hidden. Upstream coloured the traces by row, and the legend then disagreed.
        colors.append(paletteColor(i));
    }

    m_visible = visible;
    if (m_trace != nullptr)
    {
        m_trace->setChannelColors(colors);
        m_trace->setVisibleChannels(visible);
    }
    m_rescale     = true;  // UG fork: a channel shown again, or one fewer in its group, changes the scale
    m_layoutDirty = true;
    emit channelsChanged();
}

void CSignalDisplayController::pruneStimulations()
{
    double oldest = 0.0;
    double newest = 0.0;
    {
        CTraceBuffer::CReader r(m_buffer);
        if (!r.valid() || r.sampling() <= 0.0) { return; }
        newest = r.firstSampleSeconds() + double(r.totalSamples()) / r.sampling();
        oldest = r.firstSampleSeconds() + (double(r.totalSamples()) - double(r.filled())) / r.sampling();
    }

    while (!m_stims.empty() && m_stims.front().time < oldest) { m_stims.pop_front(); }

    QVariantList visible;
    visible.reserve(int(m_stims.size()));
    for (const SStimEvent& stim : m_stims)
    {
        if (stim.time < oldest || stim.time > newest) { continue; }
        QVariantMap entry;
        entry["id"]   = QVariant::fromValue(stim.id);
        entry["time"] = stim.time;
        entry["name"] = QString::fromStdString(stim.name);
        visible.append(entry);
    }

    if (visible != m_stimulations)
    {
        m_stimulations = visible;
        emit stimulationsChanged();
    }
}

void CSignalDisplayController::onTick()
{
    applyPendingMetadata();
    if (m_trace == nullptr) { return; }

    uint64_t total    = 0;
    size_t   capacity = 0;
    {
        CTraceBuffer::CReader r(m_buffer);
        total    = r.totalSamples();
        capacity = r.valid() ? r.capacity() : 0;
    }
    if (total == m_lastRenderedTotal && !m_layoutDirty) { return; }

    // UG fork: the automatic scale is worked out again for every new page, from the page just drawn: in Scan mode when
    // the cursor starts again at the left, in Scroll mode every window length. While the first page fills, it is
    // worked out every FIRST_PAGE_RESCALE_MS.
    if (m_settings.scaling != ESignalDisplayScaling::None && capacity > 0)
    {
        const uint64_t page      = total / capacity;
        const bool     firstPage = page == 0 && m_sinceScaleRefresh.elapsed() >= FIRST_PAGE_RESCALE_MS;
        if (m_rescale || page != m_scaledPage || firstPage)
        {
            m_trace->refreshAutoScale();
            m_scaledPage = page;
            m_rescale    = false;
            m_sinceScaleRefresh.restart();
            m_layoutDirty = true;
        }
    }

    if (m_layoutDirty)
    {
        if (m_trace->recomputeLayout()) { emit m_trace->layoutChanged(); }
        m_layoutDirty = false;
    }

    pruneStimulations();
    m_trace->notifyTimeWindowChanged();
    m_trace->update();
    m_lastRenderedTotal = total;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
