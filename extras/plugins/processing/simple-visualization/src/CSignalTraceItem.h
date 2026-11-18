// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QColor>
#include <QQuickItem>
#include <QVariantList>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

#include "SDisplaySettings.h"
#include "SignalScaling.h"  // UG fork

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class CTraceBuffer;

/**
 * \brief Draws every channel as one stacked trace, straight into the Qt Quick scene graph.
 *
 * One QSGGeometryNode per channel per contiguous run, filled with a line strip. Where more than
 * one sample maps to a pixel column the column is reduced to its (min, max) pair, so the vertex
 * count is capped at 2 * widthPx per channel whatever the sampling rate, while spikes survive
 * exactly -- which naive every-k-th-sample decimation would drop.
 *
 * Threading. m_rows is written on the GUI thread (recomputeLayout / setters) and read on the
 * render thread inside updatePaintNode. That is safe only because updatePaintNode runs during the
 * synchronisation phase, with the GUI thread blocked. Do not call the setters from anywhere else,
 * and do not "fix" this by moving work out of the sync phase without adding a lock.
 *
 * Not final: QML type registration instantiates QQmlPrivate::QQmlElement<CSignalTraceItem>.
 */
class CSignalTraceItem : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SignalTrace)

    /** UG fork: the fraction of a lane a trace may fill, for the amplitude ruler, which draws that extent. */
    Q_PROPERTY(qreal laneFill READ laneFill CONSTANT)
    /** Lane centre of each visible row, in item pixels. Index is the row, not the channel. */
    Q_PROPERTY(QVariantList rowBaselines READ rowBaselines NOTIFY layoutChanged)
    /** Channel index backing each visible row. */
    Q_PROPERTY(QVariantList rowChannels READ rowChannels NOTIFY layoutChanged)
    /** Signal units spanned by half a lane, per row. Feeds the amplitude scale bar. */
    Q_PROPERTY(QVariantList rowHalfRanges READ rowHalfRanges NOTIFY layoutChanged)
    Q_PROPERTY(qreal rowHeight READ rowHeight NOTIFY layoutChanged)
    Q_PROPERTY(qreal windowStartSeconds READ windowStartSeconds NOTIFY timeWindowChanged)
    Q_PROPERTY(qreal windowSeconds READ windowSeconds NOTIFY timeWindowChanged)
    /** Absolute time of the oldest and newest retained samples. The time ruler walks between them. */
    Q_PROPERTY(qreal oldestSeconds READ oldestSeconds NOTIFY timeWindowChanged)
    Q_PROPERTY(qreal newestSeconds READ newestSeconds NOTIFY timeWindowChanged)
    /** X of the sweep cursor in Scan mode, or -1 in Scroll mode. */
    Q_PROPERTY(qreal cursorX READ cursorX NOTIFY timeWindowChanged)
    Q_PROPERTY(QColor traceColor READ traceColor WRITE setTraceColor NOTIFY traceColorChanged)
    Q_PROPERTY(bool perChannelColors READ perChannelColors WRITE setPerChannelColors NOTIFY traceColorChanged)

public:
    explicit CSignalTraceItem(QQuickItem* parent = nullptr);
    ~CSignalTraceItem() override = default;

    // ---- GUI thread only ----
    void setBuffer(CTraceBuffer* buffer);
    void setSettings(const SDisplaySettings& settings);
    void setVisibleChannels(const QVector<int>& channels);
    void setChannelColors(const QVector<QColor>& colors);
    /// UG fork: the kind of each channel, indexed by channel, for scaling By type. Takes effect at the next refreshAutoScale().
    void setChannelKinds(const QVector<EChannelKind>& kinds);

    /**
     * \brief Recomputes the per-channel centre and half-range from the ring.
     *
     * UG fork: also when scaling is None, which uses the centres only. No hysteresis; refresh() calls it once per page.
     */
    void refreshAutoScale();

    /**
     * \brief One screen update, from the display's timer: works the scale out again at a new page (CPageRescale), rebuilds
     *        the layout when it changed, and redraws when samples came in.
     *
     * UG fork: the step the Signal display and the LSL display share.
     * \param layoutDirty A setting or the channels changed: rebuild the layout and redraw, even without new samples.
     * \return False when there was nothing to do: no new sample, and nothing changed.
     */
    bool refresh(bool layoutDirty);

    /// Works the scale out again at the next refresh(), whatever the page: after a change of scaling, time scale or channels.
    void requestRescale() { m_rescale.request(); }

    /**
     * \brief Rebuilds m_rows.
     *
     * \return True when the layout actually changed, so the caller can emit layoutChanged().
     */
    bool recomputeLayout();

    /** Emits timeWindowChanged() so QML rulers and overlays re-evaluate. */
    void notifyTimeWindowChanged() { emit timeWindowChanged(); }

    static qreal laneFill();
    QVariantList rowBaselines() const;
    QVariantList rowChannels() const;
    QVariantList rowHalfRanges() const;
    qreal        rowHeight() const;
    qreal        windowStartSeconds() const;
    qreal        windowSeconds() const;
    qreal        oldestSeconds() const;
    qreal        newestSeconds() const;
    qreal        cursorX() const;

    QColor traceColor() const { return m_traceColor; }
    void   setTraceColor(const QColor& color);
    bool   perChannelColors() const { return m_settings.perChannelColors; }
    void   setPerChannelColors(bool enabled);

    /**
     * \brief In Scroll mode, how far left of the right edge the newest sample is drawn, in seconds.
     *
     * UG fork, for the LSL display: streams that come in with different delays scroll together when each is drawn as far
     * behind as it lags the newest of them. 0 by default.
     */
    void setScrollLagSeconds(double seconds);

    /** Absolute time in seconds at an x coordinate. */
    Q_INVOKABLE qreal timeAtX(qreal x) const;
    /** X of an absolute time in seconds, or -1 when outside the window. */
    Q_INVOKABLE qreal xAtTime(qreal seconds) const;

signals:
    void layoutChanged();
    void timeWindowChanged();
    void traceColorChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;
    void     geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    /** One drawn lane. */
    struct SRow
    {
        int    channel  = 0;
        float  baseline = 0.0F;  ///< lane centre in item pixels
        float  gain     = 1.0F;  ///< pixels per signal unit
        float  centre   = 0.0F;  ///< signal value drawn at the baseline
        QColor color;
    };

    /** A span that is contiguous both in ring memory and in x, so the inner loop needs no modulo. */
    struct SRun
    {
        size_t first = 0;
        size_t count = 0;
        qreal  x0    = 0.0;
        qreal  dx    = 0.0;
    };

    /**
     * \brief Splits the visible span into 1 or 2 runs according to the time mode.
     *
     * \return The number of runs written to out.
     */
    int buildRuns(size_t capacity, size_t filled, size_t writePos, qreal w, double lagSamples, SRun out[2]) const;

    /** Centre and half-range to draw channel c with, honouring the scaling mode. */
    void scaleFor(int channel, double& centre, double& halfRange) const;
    /// UG fork: channels with the same group share a half-range: one group per channel, one for all, or one per kind.
    int scaleGroup(int channel) const;

    CTraceBuffer*    m_buffer = nullptr;
    SDisplaySettings m_settings;
    QVector<int>     m_visible;
    QVector<QColor>  m_colors;
    QVector<SRow>    m_rows;

    /** Per-channel {centre, halfRange}, from the last refreshAutoScale(), in every mode. UG fork: None too. */
    QVector<QPair<double, double>> m_scale;
    QVector<EChannelKind>          m_kinds;  ///< UG fork: see setChannelKinds()

    QColor m_traceColor{ 0x05, 0x1B, 0x38 };  ///< DesignSystem textPrimary
    double m_scrollLagSeconds = 0.0;          ///< UG fork: see setScrollLagSeconds()

    CPageRescale m_rescale;                ///< UG fork: when refresh() works the scale out
    uint64_t     m_lastRenderedTotal = 0;  ///< UG fork: the samples the last refresh() drew
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
