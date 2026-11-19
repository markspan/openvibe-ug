// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see CLSLDisplayController.h.

#include "CLSLDisplayController.h"

#include "CLSLRecorder.h"  // recordLabel()
#include "SignalScaling.h"
#include "TracePalette.h"

#include <lsl_cpp.h>

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <set>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {
/// Repaint cadence, as the Signal display's.
constexpr int TICK_INTERVAL_MS = 16;
/// How often the list of streams on the network is brought up to date.
constexpr qint64 LOOKUP_INTERVAL_MS = 1000;
/// Upper bound on the markers kept, so that a fast marker stream can not grow the list without limit.
constexpr size_t MAX_MARKERS = 2048;
/// How often the streams' delays are shown again: often enough to follow, seldom enough to read.
constexpr qint64 DELAY_INTERVAL_MS = 500;

/// "2048 Hz", "500.5 Hz"
QString rateText(const double rate)
{
    return rate == std::floor(rate) ? QStringLiteral("%1 Hz").arg(qint64(rate)) : QStringLiteral("%1 Hz").arg(rate, 0, 'g', 6);
}

/// How a stream is listed in the Streams parameter: "EEG (EEG, 32 ch, 2048 Hz)", "Markers (Markers, text)".
QString streamLabel(const SLSLStreamInfo& info)
{
    const QString name = QString::fromStdString(info.name);
    const QString type = info.type.empty() ? QStringLiteral("no type") : QString::fromStdString(info.type);
    if (info.isMarkers()) { return QStringLiteral("%1 (%2, %3 markers)").arg(name, type, info.text ? QStringLiteral("text") : QStringLiteral("number")); }
    return QStringLiteral("%1 (%2, %3 ch, %4)").arg(name, type).arg(info.channels).arg(rateText(info.rate));
}
}  // namespace

//------------------------------------------------------------------------------------------------
// One data stream
//------------------------------------------------------------------------------------------------

CLSLStreamView::CLSLStreamView(SLSLOpenedStream stream, QString label, QObject* parent)
    : QObject(parent), m_stream(std::move(stream)), m_label(std::move(label))
{
    // The channel selection goes by name, so a name used twice gets the channel's number, as in the Signal display
    for (size_t i = 0; i < m_stream.labels.size(); ++i)
    {
        QString name = QString::fromStdString(m_stream.labels[i]);
        if (m_names.contains(name)) { name += QStringLiteral(" (%1)").arg(i + 1); }
        m_names.append(name);
        m_kinds.append(channelKind(name.toStdString()));

        QVariantMap entry;
        entry["index"] = int(i);
        entry["name"]  = name;
        entry["unit"]  = i < m_stream.units.size() ? QString::fromStdString(m_stream.units[i]) : QString();
        entry["color"] = paletteColor(int(i));
        m_channels.append(entry);
    }

    std::vector<std::string> names;
    for (const QString& name : m_names) { names.push_back(name.toStdString()); }
    m_channelParameter = new Parameters::ParameterMultiSelectObject(("Channels: " + m_stream.info.name).c_str(), "The channels of this stream to show",
                                                                    names, names);
    connect(m_channelParameter, &Parameters::ParameterMultiSelectObject::valueChanged, this, [this](const QStringList&) { refreshVisibleChannels(); });
    for (int i = 0; i < m_names.size(); ++i) { m_visible.append(i); }
}

CLSLStreamView::~CLSLStreamView()
{
    // The trace may outlive this view a moment, in QML; it must not draw from a ring that is going
    if (m_trace) { m_trace->setBuffer(nullptr); }
    delete m_channelParameter;
}

void CLSLStreamView::attachTrace(QQuickItem* item)
{
    m_trace = qobject_cast<CSignalTraceItem*>(item);
    if (!m_trace) { return; }
    m_trace->setBuffer(m_stream.ring.get());
    m_trace->setSettings(m_settings);
    m_trace->setChannelKinds(m_kinds);
    refreshVisibleChannels();
}

void CLSLStreamView::applySettings(const SDisplaySettings& settings)
{
    m_settings = settings;
    if (m_trace)
    {
        m_trace->setSettings(m_settings);
        m_trace->requestRescale();
    }
    m_layoutDirty = true;
}

void CLSLStreamView::refreshVisibleChannels()
{
    const QStringList selected = m_channelParameter->value();
    QVector<int> visible;
    QVector<QColor> colors;
    for (int i = 0; i < m_names.size(); ++i)
    {
        if (!selected.contains(m_names.at(i))) { continue; }
        visible.append(i);
        colors.append(paletteColor(i));
    }
    m_visible = visible;
    if (m_trace)
    {
        m_trace->setChannelColors(colors);
        m_trace->setVisibleChannels(visible);
    }
    if (m_trace) { m_trace->requestRescale(); }
    m_layoutDirty = true;
    emit channelsChanged();
}

void CLSLStreamView::showDelay(const double now)
{
    double oldest = 0.0, newest = 0.0;
    const QString delay = timeSpan(oldest, newest) ? QStringLiteral("%1 ms behind").arg(qint64(std::llround((now - newest) * 1000.0))) : QString();
    if (delay != m_delay)
    {
        m_delay = delay;
        emit delayChanged();
    }
}

bool CLSLStreamView::timeSpan(double& oldest, double& newest) const
{
    CTraceBuffer::CReader r(*m_stream.ring);
    if (!r.valid() || r.sampling() <= 0.0) { return false; }
    newest = r.firstSampleSeconds() + double(r.totalSamples()) / r.sampling();
    oldest = r.firstSampleSeconds() + (double(r.totalSamples()) - double(r.filled())) / r.sampling();
    return true;
}

void CLSLStreamView::tick(const double scrollLagSeconds)
{
    if (!m_trace) { return; }
    m_trace->setScrollLagSeconds(m_settings.timeMode == ESignalDisplayMode::Scroll ? scrollLagSeconds : 0.0);

    // The scale for a new page, the layout and the redraw, as in the Signal display (CSignalTraceItem::refresh())
    if (m_trace->refresh(m_layoutDirty)) { m_layoutDirty = false; }
}

//------------------------------------------------------------------------------------------------
// The display
//------------------------------------------------------------------------------------------------

CLSLDisplayController::CLSLDisplayController(Kernel::ILogManager& logManager, const SDisplaySettings& initial, QQmlEngine* engine)
    : m_log(logManager), m_settings(initial), m_engine(engine)
{
    m_reader = std::make_unique<CLSLReader>(m_settings.timeScaleSeconds);
    buildParameters();
    buildQml(engine);

    m_sinceLookUp.start();
    m_sinceDelay.start();
    m_recordingSince.start();
    m_tick.setInterval(TICK_INTERVAL_MS);
    m_tick.setTimerType(Qt::PreciseTimer);
    connect(&m_tick, &QTimer::timeout, this, &CLSLDisplayController::onTick);
    m_tick.start();
}

CLSLDisplayController::~CLSLDisplayController()
{
    m_tick.stop();
    m_reader.reset();  // stops the reader thread before the rings it writes go

    for (CLSLStreamView* view : m_views) { delete view; }  // detaches each trace from its ring
    m_views.clear();

    // As in CSignalDisplayController: take the item back from the window manager's QML before deleting it
    if (m_root)
    {
        m_root->setParentItem(nullptr);
        m_root->setParent(nullptr);
        delete m_root.data();
    }
    for (Parameters::ParameterObject* parameter : m_parameters) { delete parameter; }
}

void CLSLDisplayController::buildQml(QQmlEngine* engine)
{
    if (engine == nullptr)
    {
        m_log << Kernel::LogLevel_Error << "LSL display: no QML engine available\n";
        return;
    }
    auto* context = new QQmlContext(engine->rootContext());
    context->setContextProperty("dataSource", this);

    QQmlComponent component(engine);
    component.loadFromModule("OpenViBE.SignalDisplay", "LSLDisplay");
    if (component.isError())
    {
        for (const QQmlError& error : component.errors()) { m_log << Kernel::LogLevel_Error << "LSL display QML: " << error.toString().toUtf8().constData() << "\n"; }
        delete context;
        return;
    }
    QObject* object = component.create(context);
    m_root          = qobject_cast<QQuickItem*>(object);
    if (m_root == nullptr)
    {
        m_log << Kernel::LogLevel_Error << "LSL display: LSLDisplay.qml did not yield a QQuickItem\n";
        delete object;
        delete context;
        return;
    }
    context->setParent(m_root);
    QQmlEngine::setObjectOwnership(m_root, QQmlEngine::CppOwnership);
}

QJSValue CLSLDisplayController::parameters() const
{
    if (m_engine == nullptr) { return {}; }
    std::vector<Parameters::ParameterObject*> all(m_parameters.begin(), m_parameters.end());
    for (const CLSLStreamView* view : m_views) { all.push_back(view->channelParameter()); }

    QJSValue list = m_engine->newArray(uint(all.size()));
    for (size_t i = 0; i < all.size(); ++i)
    {
        QQmlEngine::setObjectOwnership(all[i], QQmlEngine::CppOwnership);
        list.setProperty(quint32(i), m_engine->newQObject(all[i]));
    }
    return list;
}

QList<QObject*> CLSLDisplayController::streams() const
{
    QList<QObject*> list;
    for (CLSLStreamView* view : m_views) { list.append(view); }
    return list;
}

void CLSLDisplayController::buildParameters()
{
    m_pStreams = new Parameters::ParameterMultiSelectObject("Streams", "The LSL streams on the network; tick those to show. Data streams are drawn, "
                                                            "marker streams are drawn as flags on all of them",
                                                            {});
    connect(m_pStreams, &Parameters::ParameterMultiSelectObject::valueChanged, this, [this](const QStringList&) { selectStreams(); });
    m_parameters.push_back(m_pStreams);

    // The options of the Signal display, shared with it (CDisplayPanel), applying to every stream
    m_panel = std::make_unique<CDisplayPanel>(m_settings, this, [this](const EDisplayChange change) { onPanelChange(change); });
    for (Parameters::ParameterObject* entry : m_panel->scaleEntries()) { m_parameters.push_back(entry); }
    for (Parameters::ParameterObject* entry : m_panel->viewEntries()) { m_parameters.push_back(entry); }
}

void CLSLDisplayController::onPanelChange(const EDisplayChange change)
{
    switch (change)
    {
        case EDisplayChange::Rulers:
            emit settingsChanged();
            return;
        case EDisplayChange::TimeScale:
            m_reader->setWindowSeconds(m_settings.timeScaleSeconds);
            for (CLSLStreamView* view : m_views) { view->ring().setWindowSeconds(m_settings.timeScaleSeconds); }
            break;
        default: break;
    }
    applySettings();
}

void CLSLDisplayController::applySettings()
{
    for (CLSLStreamView* view : m_views) { view->applySettings(m_settings); }
    emit settingsChanged();
}

void CLSLDisplayController::refreshAvailableStreams()
{
    // Streams that went away stay listed while they are selected, so that they come back when they do
    std::map<QString, SLSLStreamInfo> byLabel;
    const QStringList selected = m_pStreams->value();
    for (const QString& label : selected)
    {
        const auto it = m_byLabel.find(label);
        if (it != m_byLabel.end()) { byLabel.emplace(label, it->second); }
    }
    std::vector<SLSLStreamInfo> available = m_reader->available();
    std::map<QString, int> uses;
    for (const SLSLStreamInfo& info : available) { ++uses[streamLabel(info)]; }
    for (const SLSLStreamInfo& info : available)
    {
        // Two streams that look alike are told apart by the computer they come from
        QString label = streamLabel(info);
        if (uses[label] > 1) { label += QStringLiteral(" @%1").arg(QString::fromStdString(info.hostname)); }
        byLabel[label] = info;
    }

    QStringList labels;
    for (const auto& entry : byLabel) { labels.append(entry.first); }
    const bool changed = labels != m_pStreams->availableValues();
    m_byLabel = std::move(byLabel);
    if (changed) { m_pStreams->setAvailableValues(labels); }
}

void CLSLDisplayController::selectStreams()
{
    std::set<std::string> uids;
    int markers = 0;
    for (const QString& label : m_pStreams->value())
    {
        const auto it = m_byLabel.find(label);
        if (it == m_byLabel.end()) { continue; }
        uids.insert(it->second.uid);
        if (it->second.isMarkers()) { ++markers; }
    }
    m_reader->select(uids);
    {
        std::lock_guard<std::mutex> lock(m_tickedMutex);
        m_ticked.assign(uids.begin(), uids.end());
        ++m_tickedVersion;
    }
    emit tickedChanged();
    if (markers != m_markerStreams)
    {
        m_markerStreams = markers;
        if (markers == 0) { m_markers.clear(); }
        emit streamsChanged();
    }
}

std::vector<std::string> CLSLDisplayController::streamsToRecord(uint64_t& version) const
{
    std::lock_guard<std::mutex> lock(m_tickedMutex);
    version = m_tickedVersion;
    return m_recording ? m_recorded : std::vector<std::string>();
}

bool CLSLDisplayController::canRecord() const
{
    std::lock_guard<std::mutex> lock(m_tickedMutex);
    return !m_ticked.empty();
}

void CLSLDisplayController::toggleRecording()
{
    {
        std::lock_guard<std::mutex> lock(m_tickedMutex);
        if (!m_recording && m_ticked.empty()) { return; }  // nothing to record
        m_recording = !m_recording;
        // Of each data stream, the channels shown; a marker stream, or a stream not shown yet, whole. A data stream with no
        // channel shown is left out.
        m_recorded.clear();
        for (const std::string& uid : m_ticked)
        {
            const auto view = std::find_if(m_views.begin(), m_views.end(), [&uid](const CLSLStreamView* v) { return v->uid() == uid; });
            if (view == m_views.end()) { m_recorded.push_back(uid); continue; }
            const QVector<int>& shown = (*view)->visibleChannels();
            if (shown.isEmpty()) { continue; }
            CLSLRecorder::Channels channels;
            if (shown.size() < (*view)->channelCount()) { for (const int c : shown) { channels.push_back(size_t(c)); } }
            m_recorded.push_back(CLSLRecorder::recordLabel(uid, channels));
        }
        ++m_tickedVersion;
    }
    // While recording, the streams and their channels stay as they are: a file holds the same channels throughout
    m_pStreams->setEnabled(!m_recording);
    for (CLSLStreamView* view : m_views) { view->channelParameter()->setEnabled(!m_recording); }
    m_recordingSince.restart();
    m_recordingTime = m_recording ? QStringLiteral("0:00") : QString();
    emit recordingChanged();
    emit recordingTimeChanged();
}

void CLSLDisplayController::addStream(SLSLOpenedStream stream)
{
    QString label = streamLabel(stream.info);
    for (const auto& entry : m_byLabel)
    {
        if (entry.second.uid == stream.info.uid) { label = entry.first; }
    }
    auto* view = new CLSLStreamView(std::move(stream), label, this);
    view->applySettings(m_settings);
    view->channelParameter()->setEnabled(!m_recording);  // a stream that opens while recording is recorded whole
    m_views.push_back(view);
    emit streamsChanged();
    emit parametersChanged();
}

void CLSLDisplayController::removeStream(const std::string& uid)
{
    const auto it = std::find_if(m_views.begin(), m_views.end(), [&uid](const CLSLStreamView* view) { return view->uid() == uid; });
    if (it == m_views.end()) { return; }
    CLSLStreamView* view = *it;
    m_views.erase(it);
    if (view->trace()) { view->trace()->setBuffer(nullptr); }
    emit streamsChanged();
    emit parametersChanged();
    view->deleteLater();  // after QML has let go of its panel and its channel selection
}

void CLSLDisplayController::collectMarkers()
{
    for (SLSLMarker& marker : m_reader->takeMarkers()) { m_markers.push_back(std::move(marker)); }
    // Markers from several streams come in their own order; the overlay wants them in time
    std::stable_sort(m_markers.begin(), m_markers.end(), [](const SLSLMarker& a, const SLSLMarker& b) { return a.time < b.time; });
    while (m_markers.size() > MAX_MARKERS) { m_markers.pop_front(); }
}

void CLSLDisplayController::onTick()
{
    if (m_sinceLookUp.elapsed() >= LOOKUP_INTERVAL_MS)
    {
        refreshAvailableStreams();
        m_sinceLookUp.restart();
    }
    for (SLSLOpenedStream& stream : m_reader->takeOpened()) { addStream(std::move(stream)); }
    for (const std::string& uid : m_reader->takeClosed()) { removeStream(uid); }
    collectMarkers();

    // Where the streams' samples reach: in Scroll mode each is drawn as far behind as it lags the newest of them
    double oldest = 0.0, newest = 0.0;
    bool any      = false;
    std::vector<double> newestOf(m_views.size(), 0.0);
    for (size_t i = 0; i < m_views.size(); ++i)
    {
        double from = 0.0, to = 0.0;
        if (!m_views[i]->timeSpan(from, to)) { continue; }
        newestOf[i] = to;
        oldest      = any ? std::min(oldest, from) : from;
        newest      = any ? std::max(newest, to) : to;
        any         = true;
    }
    for (size_t i = 0; i < m_views.size(); ++i) { m_views[i]->tick(newestOf[i] > 0.0 ? newest - newestOf[i] : 0.0); }
    if (m_recording)
    {
        const qint64 seconds = m_recordingSince.elapsed() / 1000;
        const QString time   = QStringLiteral("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QLatin1Char('0'));
        if (time != m_recordingTime)
        {
            m_recordingTime = time;
            emit recordingTimeChanged();
        }
    }
    if (m_sinceDelay.elapsed() >= DELAY_INTERVAL_MS)
    {
        const double now = lsl::local_clock();  // the clock the samples' dates are on
        for (CLSLStreamView* view : m_views) { view->showDelay(now); }
        m_sinceDelay.restart();
    }

    // The markers within the time shown, on every stream
    QVariantList visible;
    if (any)
    {
        while (!m_markers.empty() && m_markers.front().time < oldest) { m_markers.pop_front(); }
        for (const SLSLMarker& marker : m_markers)
        {
            if (marker.time > newest) { break; }
            QVariantMap entry;
            entry["id"]   = QVariant::fromValue(marker.id);
            entry["time"] = marker.time;
            entry["name"] = QString::fromStdString(marker.text);
            visible.append(entry);
        }
    }
    if (visible != m_stimulations)
    {
        m_stimulations = visible;
        emit stimulationsChanged();
    }
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
