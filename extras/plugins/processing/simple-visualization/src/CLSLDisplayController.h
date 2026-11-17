// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.28. The LSL display's view side: one CLSLStreamView per data stream, the options panel,
// and the markers, on the GUI thread; the data come from CLSLReader.

#pragma once

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
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <parametersObject.h>

#include "CLSLReader.h"
#include "CSignalTraceItem.h"
#include "SDisplaySettings.h"

class QQmlEngine;

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/**
 * \brief One data stream of the LSL display: its ring, its trace, and which of its channels are shown.
 *
 * QML sees it as an entry of the controller's streams list; its panel hands it its trace with attachTrace().
 */
class CLSLStreamView final : public QObject
{
    Q_OBJECT

    /// "name (type, 32 ch, 2048 Hz)"
    Q_PROPERTY(QString label READ label CONSTANT)
    /// One entry per channel: { name, unit, index, color }, as the Signal display's
    Q_PROPERTY(QVariantList channels READ channels NOTIFY channelsChanged)
    Q_PROPERTY(int visibleChannelCount READ visibleChannelCount NOTIFY channelsChanged)
    /**
     * How long after its date the stream's newest sample was here, as "42 ms behind". It is where this stream's cursor is,
     * left of now: streams sent in bigger chunks, or from further away, come in later. Their data are on their dates all
     * the same.
     */
    Q_PROPERTY(QString delay READ delay NOTIFY delayChanged)

public:
    CLSLStreamView(SLSLOpenedStream stream, QString label, QObject* parent);
    ~CLSLStreamView() override;

    QString label() const { return m_label; }
    QVariantList channels() const { return m_channels; }
    int visibleChannelCount() const { return int(m_visible.size()); }
    QString delay() const { return m_delay; }
    /// Shows how far behind \p now, on the LSL clock, the stream's newest sample is.
    void showDelay(double now);

    /// Called by the stream's panel once its trace exists; the view drives it from then on.
    Q_INVOKABLE void attachTrace(QQuickItem* item);

    const std::string& uid() const { return m_stream.info.uid; }
    /// The channels shown, counted from 0, in the stream's order; those recorded
    const QVector<int>& visibleChannels() const { return m_visible; }
    int channelCount() const { return int(m_names.size()); }
    CTraceBuffer& ring() const { return *m_stream.ring; }
    CSignalTraceItem* trace() const { return m_trace; }
    Parameters::ParameterMultiSelectObject* channelParameter() const { return m_channelParameter; }

    /// Gives the trace the display settings, and the colouring.
    void applySettings(const SDisplaySettings& settings, bool perChannelColors);
    /// Follows the channel selection; called when it changes.
    void refreshVisibleChannels();
    /**
     * \brief One screen update: the scale for a new page, the layout, the time window.
     * \param scrollLagSeconds How far behind the newest of all streams this one is, in Scroll mode.
     */
    void tick(double scrollLagSeconds);
    /// The time of this stream's oldest and newest sample shown; false while it has none.
    bool timeSpan(double& oldest, double& newest) const;

signals:
    void channelsChanged();
    void delayChanged();

private:
    SLSLOpenedStream m_stream;
    QString m_label;
    QStringList m_names;
    QVector<EChannelKind> m_kinds;
    QVector<int> m_visible;
    QVariantList m_channels;
    QString m_delay;
    Parameters::ParameterMultiSelectObject* m_channelParameter = nullptr;
    QPointer<CSignalTraceItem> m_trace;
    SDisplaySettings m_settings;
    bool m_perChannelColors = false;

    // The automatic scale, worked out again for every page as in CSignalDisplayController::onTick()
    QElapsedTimer m_sinceScaleRefresh;
    uint64_t m_scaledPage        = 0;
    bool m_rescale               = true;
    uint64_t m_lastRenderedTotal = 0;
    bool m_layoutDirty           = true;
};

/**
 * \brief The LSL display: the streams to show, the options, the markers, and the repaint, on the GUI thread.
 *
 * This is the object LSLDisplay.qml sees as "dataSource". The options are the Signal display's (SDisplaySettings), and apply
 * to every stream; each data stream has its own channel selection, added to the panel when the stream is shown.
 */
class CLSLDisplayController final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QJSValue parameters READ parameters NOTIFY parametersChanged)
    Q_PROPERTY(QList<QObject*> streams READ streams NOTIFY streamsChanged)
    /// Markers inside the time shown: { time, name, id }
    Q_PROPERTY(QVariantList stimulations READ stimulations NOTIFY stimulationsChanged)
    Q_PROPERTY(bool showTimeRuler READ showTimeRuler NOTIFY settingsChanged)
    Q_PROPERTY(bool showAmplitudeRuler READ showAmplitudeRuler NOTIFY settingsChanged)
    Q_PROPERTY(bool butterfly READ butterfly NOTIFY settingsChanged)
    /// Markers are being read, though no data stream is shown
    Q_PROPERTY(bool hasMarkerStreams READ hasMarkerStreams NOTIFY streamsChanged)
    /// Whether the streams ticked are being recorded, by an LSL XDF writer connected to the box's output
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    /// How long the recording has run, "1:23"
    Q_PROPERTY(QString recordingTime READ recordingTime NOTIFY recordingTimeChanged)
    /// Whether a stream is ticked, so that there is something to record
    Q_PROPERTY(bool canRecord READ canRecord NOTIFY tickedChanged)

public:
    CLSLDisplayController(Kernel::ILogManager& logManager, const SDisplaySettings& initial, QQmlEngine* engine);
    ~CLSLDisplayController() override;

    QQuickItem* rootItem() const { return m_root; }

    QString name() const { return QStringLiteral("LSL display"); }
    QJSValue parameters() const;
    QList<QObject*> streams() const;
    QVariantList stimulations() const { return m_stimulations; }
    bool showTimeRuler() const { return m_settings.showTimeRuler; }
    bool showAmplitudeRuler() const { return m_settings.showAmplitudeRuler; }
    bool butterfly() const { return m_settings.layout == ELayoutMode::Butterfly; }
    bool hasMarkerStreams() const { return m_markerStreams > 0; }
    bool recording() const { return m_recording; }
    QString recordingTime() const { return m_recordingTime; }
    bool canRecord() const;

    /**
     * \brief Starts recording the streams ticked, of each only the channels shown, or stops. While recording, the ticks and the
     *        channel selections can not be changed, so that a file holds the same channels throughout.
     */
    Q_INVOKABLE void toggleRecording();

    /**
     * \brief The streams to record now, for the box's output: a label per stream, its uid and the channels shown
     *        (CLSLRecorder::recordLabel()), while recording; none otherwise. Any thread.
     * \param version Goes up by one each time they change, so that the box sends them only then.
     */
    std::vector<std::string> streamsToRecord(uint64_t& version) const;

signals:
    void parametersChanged();
    void streamsChanged();
    void recordingChanged();
    void recordingTimeChanged();
    void tickedChanged();
    void stimulationsChanged();
    void settingsChanged();

private:
    void buildParameters();
    void buildQml(QQmlEngine* engine);
    void updateEnabledParameters();
    /// Every view takes the settings again.
    void applySettings();
    /// Lists the streams on the network in the Streams parameter, keeping those selected.
    void refreshAvailableStreams();
    /// Tells the reader which streams are selected.
    void selectStreams();
    void addStream(SLSLOpenedStream stream);
    void removeStream(const std::string& uid);
    void collectMarkers();
    void onTick();

    Kernel::ILogManager& m_log;
    SDisplaySettings m_settings;
    QQmlEngine* m_engine = nullptr;
    QPointer<QQuickItem> m_root;
    QTimer m_tick;
    QElapsedTimer m_sinceLookUp;
    QElapsedTimer m_sinceDelay;  ///< Since the streams' delays were last shown

    std::unique_ptr<CLSLReader> m_reader;
    std::vector<CLSLStreamView*> m_views;          ///< In the order they were opened
    std::map<QString, SLSLStreamInfo> m_byLabel;   ///< The streams listed, by their label in the Streams parameter
    int m_markerStreams = 0;                        ///< Selected marker streams

    mutable std::mutex m_tickedMutex;               // guards the four below, which the player thread reads
    std::vector<std::string> m_ticked;              ///< The streams ticked, by uid
    std::vector<std::string> m_recorded;            ///< The labels of the streams being recorded, see streamsToRecord()
    bool m_recording         = false;               ///< See toggleRecording()
    uint64_t m_tickedVersion = 0;                   ///< See streamsToRecord()
    QElapsedTimer m_recordingSince;
    QString m_recordingTime;

    std::deque<SLSLMarker> m_markers;
    QVariantList m_stimulations;

    Parameters::ParameterObjects m_parameters;  ///< The fixed ones; the views add their channel selections
    Parameters::ParameterMultiSelectObject* m_pStreams       = nullptr;
    Parameters::ParameterListStringObject*  m_pDisplayMode   = nullptr;
    Parameters::ParameterListStringObject*  m_pScaling       = nullptr;
    Parameters::ParameterDoubleObject*      m_pTimeScale     = nullptr;
    Parameters::ParameterDoubleObject*      m_pVerticalScale = nullptr;
    Parameters::ParameterDoubleObject*      m_pOffset        = nullptr;
    Parameters::ParameterBoolObject*        m_pButterfly     = nullptr;
    Parameters::ParameterBoolObject*        m_pColors        = nullptr;
    Parameters::ParameterBoolObject*        m_pTimeRuler     = nullptr;
    Parameters::ParameterBoolObject*        m_pAmpRuler      = nullptr;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
