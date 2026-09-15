// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QJSValue>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVector>

#include <deque>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <openvibe/ov_all.h>
#include <parametersObject.h>

#include "CSignalTraceItem.h"
#include "CTraceBuffer.h"
#include "SDisplaySettings.h"

class QQmlEngine;

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {


/**
 * \brief Owns the view: builds the QML tree, feeds the ring, drives the repaint.
 *
 * This is the object QML sees as "dataSource".
 *
 * It is also the whole thread seam. The player thread calls only the push* methods, which take a
 * short lock and return; nothing QObject-related is touched from there. Everything else runs on
 * the GUI thread, paced by a single timer, so the repaint is throttled instead of being driven
 * synchronously by every incoming chunk.
 */
class CSignalDisplayController final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QJSValue parameters READ parameters NOTIFY parametersChanged)
    /** One entry per channel: { name, unit, index }. */
    Q_PROPERTY(QVariantList channels READ channels NOTIFY channelsChanged)
    /** Stimulations inside the displayed window: { time, name, id }. */
    Q_PROPERTY(QVariantList stimulations READ stimulations NOTIFY stimulationsChanged)
    Q_PROPERTY(bool showTimeRuler READ showTimeRuler NOTIFY settingsChanged)
    Q_PROPERTY(bool showAmplitudeRuler READ showAmplitudeRuler NOTIFY settingsChanged)
    /** 0 fits every channel in the viewport; otherwise the view scrolls at this density. */
    Q_PROPERTY(int channelsPerPage READ channelsPerPage NOTIFY settingsChanged)
    /** True when every channel shares a single baseline, which makes paging meaningless. */
    Q_PROPERTY(bool butterfly READ butterfly NOTIFY settingsChanged)
    /**
     * Number of channels selected for display. Comes from the channel selection, deliberately not
     * from the renderer's row count: the rows are a *result* of the item's height, so feeding them
     * back into a height binding is a loop.
     */
    Q_PROPERTY(int visibleChannelCount READ visibleChannelCount NOTIFY channelsChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY channelsChanged)

public:
    CSignalDisplayController(Kernel::ILogManager& logManager, const SDisplaySettings& initial, QQmlEngine* engine);
    ~CSignalDisplayController() override;

    /** The item handed to the window manager, or null when the QML failed to load. */
    QQuickItem* rootItem() const { return m_root; }

    void setName(const QString& name);

    // ---- called from the PLAYER thread ----
    void pushHeader(size_t channels, double samplingRate, std::vector<std::string> labels);
    void pushChunk(const double* buffer, size_t channels, size_t samples, uint64_t startTime, uint64_t endTime);
    void pushStimulation(uint64_t id, uint64_t date, std::string name);
    void pushChannelUnits(std::vector<std::pair<std::string, std::string>> units);

    // ---- read by QML, GUI thread ----
    QString      name() const { return m_name; }
    QJSValue     parameters() const;
    QVariantList channels() const { return m_channels; }
    QVariantList stimulations() const { return m_stimulations; }
    bool         showTimeRuler() const { return m_settings.showTimeRuler; }
    bool         showAmplitudeRuler() const { return m_settings.showAmplitudeRuler; }
    int          channelsPerPage() const { return m_channelsPerPage; }
    bool         butterfly() const { return m_settings.layout == ELayoutMode::Butterfly; }
    int          visibleChannelCount() const { return int(m_visible.size()); }
    bool         hasData() const { return !m_channelNames.isEmpty(); }

signals:
    void nameChanged();
    void parametersChanged();
    void channelsChanged();
    void stimulationsChanged();
    void settingsChanged();

private:
    struct SStimEvent
    {
        uint64_t    id   = 0;
        double      time = 0.0;  ///< seconds
        std::string name;
    };

    /** Metadata crossing from the player thread, drained by the tick. */
    struct SPending
    {
        bool                     header   = false;
        size_t                   channels = 0;
        double                   sampling = 0.0;
        std::vector<std::string> labels;

        bool                                             units = false;
        std::vector<std::pair<std::string, std::string>> unitList;

        std::deque<SStimEvent> stims;
    };

    void onTick();
    void buildParameters();
    void buildQml(QQmlEngine* engine);
    void applyPendingMetadata();
    void rebuildChannelModel();
    void refreshVisibleChannels();
    void pruneStimulations();

    Kernel::ILogManager& m_log;
    SDisplaySettings     m_settings;  ///< the single source of truth
    CTraceBuffer         m_buffer;

    // QPointers: QML owns the item once WindowManagerNodeComponent.qml reparents it, so it may
    // be destroyed from that side first. These null themselves if that happens.
    QPointer<QQuickItem>       m_root;
    QPointer<CSignalTraceItem> m_trace;
    QQmlEngine*       m_engine = nullptr;

    QTimer        m_tick;
    QElapsedTimer m_sinceScaleRefresh;
    uint64_t      m_lastRenderedTotal = 0;
    bool          m_layoutDirty       = true;

    mutable std::mutex m_metaMutex;
    SPending           m_pending;

    QString      m_name = "Signal Display";
    QStringList  m_channelNames;
    QStringList  m_channelUnits;
    QVariantList m_channels;
    QVariantList m_stimulations;
    QVector<int> m_visible;
    int          m_channelsPerPage = 0;

    std::deque<SStimEvent> m_stims;

    Parameters::ParameterObjects            m_parameters;
    Parameters::ParameterListStringObject*  m_pDisplayMode    = nullptr;
    Parameters::ParameterListStringObject*  m_pScaling        = nullptr;
    Parameters::ParameterDoubleObject*      m_pScaleRefresh   = nullptr;
    Parameters::ParameterDoubleObject*      m_pVerticalScale  = nullptr;
    Parameters::ParameterDoubleObject*      m_pBaselineOffset = nullptr;
    Parameters::ParameterDoubleObject*      m_pTimeScale      = nullptr;
    Parameters::ParameterBoolObject*        m_pTimeRuler      = nullptr;
    Parameters::ParameterBoolObject*        m_pAmplitudeRuler = nullptr;
    Parameters::ParameterBoolObject*        m_pButterfly      = nullptr;
    Parameters::ParameterBoolObject*        m_pChannelColors  = nullptr;
    Parameters::ParameterIntObject*         m_pChannelsPerPage = nullptr;
    Parameters::ParameterMultiSelectObject* m_pChannels       = nullptr;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
