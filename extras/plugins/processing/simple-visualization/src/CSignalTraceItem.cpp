// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CSignalTraceItem.h"

#include "CTraceBuffer.h"

#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {

/** Fraction of a lane the trace is allowed to fill, leaving a gutter between neighbours. */
constexpr double LANE_FILL = 0.90;
/** Floor on the half-range, so a DC-flat channel cannot produce an infinite gain. */
constexpr double MIN_HALF_RANGE = 1e-9;
/**
 * Only shrink the gain once the signal has dropped well below the current range: this is the
 * hysteresis band that stops the trace breathing on every refresh.
 */
constexpr double SHRINK_RATIO = 0.6;
/** Exponential settle factor for the lane centre. */
constexpr double CENTRE_SETTLE = 0.30;
/** Headroom kept above the observed peak. */
constexpr double HEADROOM = 1.05;

/** Grows the range at once but shrinks it only past the hysteresis band. */
void applyHysteresis(QPair<double, double>& state, const double newCentre, const double newHalf)
{
    const double target = std::max(newHalf * HEADROOM, MIN_HALF_RANGE);
    if (target > state.second || target < state.second * SHRINK_RATIO) { state.second = target; }
    state.first += CENTRE_SETTLE * (newCentre - state.first);
}

/**
 * \brief The existing child at this index, or null when one still has to be built.
 *
 * Deliberately does not create and append: a QSGGeometryNode must never enter the live tree
 * before its geometry and material are set, or the renderer dereferences a null QSGGeometry.
 */
QSGGeometryNode* existingGeometryNode(QSGNode* root, const int index)
{
    return index < root->childCount() ? static_cast<QSGGeometryNode*>(root->childAtIndex(index)) : nullptr;
}

void trimChildren(QSGNode* root, const int keep)
{
    while (root->childCount() > keep)
    {
        QSGNode* last = root->childAtIndex(root->childCount() - 1);
        root->removeChildNode(last);
        delete last;
    }
}

}  // namespace

CSignalTraceItem::CSignalTraceItem(QQuickItem* parent) : QQuickItem(parent) { setFlag(ItemHasContents, true); }

void CSignalTraceItem::setBuffer(CTraceBuffer* buffer)
{
    m_buffer = buffer;
    if (recomputeLayout()) { emit layoutChanged(); }
    emit timeWindowChanged();
    update();
}

void CSignalTraceItem::setSettings(const SDisplaySettings& settings)
{
    m_settings = settings;
    if (recomputeLayout()) { emit layoutChanged(); }
    emit timeWindowChanged();
    update();
}

void CSignalTraceItem::setVisibleChannels(const QVector<int>& channels)
{
    if (channels == m_visible) { return; }
    m_visible = channels;
    if (recomputeLayout()) { emit layoutChanged(); }
    update();
}

void CSignalTraceItem::setChannelColors(const QVector<QColor>& colors)
{
    m_colors = colors;
    if (recomputeLayout()) { emit layoutChanged(); }
    update();
}

void CSignalTraceItem::setTraceColor(const QColor& color)
{
    if (color == m_traceColor) { return; }
    m_traceColor = color;
    emit traceColorChanged();
    if (recomputeLayout()) { emit layoutChanged(); }
    update();
}

void CSignalTraceItem::setPerChannelColors(const bool enabled)
{
    if (enabled == m_perChannelColors) { return; }
    m_perChannelColors = enabled;
    emit traceColorChanged();
    if (recomputeLayout()) { emit layoutChanged(); }
    update();
}

void CSignalTraceItem::scaleFor(const int channel, double& centre, double& halfRange) const
{
    switch (m_settings.scaling)
    {
        case ESignalDisplayScaling::None:
            centre    = m_settings.baselineOffset;
            halfRange = m_settings.verticalScale;
            break;
        case ESignalDisplayScaling::Global:
            centre    = m_globalScale.first;
            halfRange = m_globalScale.second;
            break;
        default:
            if (channel >= 0 && channel < m_scale.size())
            {
                centre    = m_scale.at(channel).first;
                halfRange = m_scale.at(channel).second;
            }
            else
            {
                centre    = 0.0;
                halfRange = m_settings.verticalScale;
            }
            break;
    }
    // Written so that a NaN half-range is caught too.
    if (!(halfRange > MIN_HALF_RANGE)) { halfRange = MIN_HALF_RANGE; }
}

void CSignalTraceItem::refreshAutoScale()
{
    if (m_buffer == nullptr || m_settings.scaling == ESignalDisplayScaling::None) { return; }

    CTraceBuffer::CReader r(*m_buffer);
    if (!r.valid()) { return; }

    const size_t capacity = r.capacity();
    const size_t filled   = r.filled();
    const size_t nChannel = r.channels();
    if (filled == 0 || capacity == 0 || nChannel == 0) { return; }

    if (m_scale.size() != int(nChannel)) { m_scale.assign(int(nChannel), { 0.0, m_settings.verticalScale }); }

    // The valid window is at most two contiguous spans of the ring.
    const size_t first = (r.writePos() + capacity - filled) % capacity;
    const size_t head  = std::min(filled, capacity - first);
    const size_t tail  = filled - head;

    double globalLo = std::numeric_limits<double>::max();
    double globalHi = std::numeric_limits<double>::lowest();

    for (const int c : m_visible)
    {
        if (c < 0 || size_t(c) >= nChannel) { continue; }
        const float* src = r.channel(size_t(c));

        float lo = std::numeric_limits<float>::max();
        float hi = std::numeric_limits<float>::lowest();
        for (size_t i = 0; i < head; ++i)
        {
            const float v = src[first + i];
            lo            = std::min(lo, v);
            hi            = std::max(hi, v);
        }
        for (size_t i = 0; i < tail; ++i)
        {
            const float v = src[i];
            lo            = std::min(lo, v);
            hi            = std::max(hi, v);
        }
        if (!(lo <= hi)) { continue; }  // an all-NaN channel

        applyHysteresis(m_scale[c], 0.5 * (double(lo) + double(hi)), 0.5 * (double(hi) - double(lo)));
        globalLo = std::min(globalLo, double(lo));
        globalHi = std::max(globalHi, double(hi));
    }

    if (m_settings.scaling == ESignalDisplayScaling::Global && globalLo <= globalHi)
    {
        applyHysteresis(m_globalScale, 0.5 * (globalLo + globalHi), 0.5 * (globalHi - globalLo));
    }
}

bool CSignalTraceItem::recomputeLayout()
{
    QVector<SRow> rows;

    const qreal h = height();
    const int   n = int(m_visible.size());
    if (n > 0 && h > 0.0)
    {
        const bool  butterfly = m_settings.layout == ELayoutMode::Butterfly;
        const qreal lane      = butterfly ? h : h / qreal(n);
        const qreal halfLane  = lane * 0.5 * LANE_FILL;

        rows.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            const int c = m_visible.at(i);

            double centre    = 0.0;
            double halfRange = 1.0;
            scaleFor(c, centre, halfRange);

            SRow row;
            row.channel  = c;
            row.baseline = float(butterfly ? h * 0.5 : (qreal(i) + 0.5) * lane);
            row.gain     = float(halfLane / halfRange);
            row.centre   = float(centre);
            row.color    = (m_perChannelColors && i < m_colors.size()) ? m_colors.at(i) : m_traceColor;
            rows.append(row);
        }
    }

    if (rows.size() == m_rows.size())
    {
        bool same = true;
        for (int i = 0; i < rows.size() && same; ++i)
        {
            const SRow& a = rows.at(i);
            const SRow& b = m_rows.at(i);
            same          = a.channel == b.channel && a.baseline == b.baseline && a.gain == b.gain && a.centre == b.centre && a.color == b.color;
        }
        if (same) { return false; }
    }

    m_rows = rows;
    return true;
}

int CSignalTraceItem::buildRuns(const size_t capacity, const size_t filled, const size_t writePos, const qreal w, SRun out[2]) const
{
    if (capacity == 0 || filled == 0 || w <= 0.0) { return 0; }

    const qreal dx = w / qreal(capacity);

    if (m_settings.timeMode == ESignalDisplayMode::Scroll)
    {
        // The newest sample sits at the right edge; the span may wrap the ring.
        const size_t first = (writePos + capacity - filled) % capacity;
        const qreal  x0    = w - qreal(filled - 1) * dx;
        const size_t head  = std::min(filled, capacity - first);

        out[0] = { first, head, x0, dx };
        if (head == filled) { return 1; }
        out[1] = { 0, filled - head, x0 + qreal(head) * dx, dx };
        return 2;
    }

    // Scan: slot i is pinned to x = i * dx. Split at the write cursor so that no segment is drawn
    // across the sweep discontinuity between the old page and the new one.
    if (filled < capacity)
    {
        out[0] = { 0, filled, 0.0, dx };
        return 1;
    }

    int n = 0;
    if (writePos < capacity) { out[n++] = { writePos, capacity - writePos, qreal(writePos) * dx, dx }; }
    if (writePos > 0) { out[n++] = { 0, writePos, 0.0, dx }; }
    return n;
}

QSGNode* CSignalTraceItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* /*data*/)
{
    QSGNode* root = oldNode != nullptr ? oldNode : new QSGNode;

    const qreal w = width();
    const qreal h = height();
    if (m_buffer == nullptr || m_rows.isEmpty() || w <= 0.0 || h <= 0.0)
    {
        trimChildren(root, 0);
        return root;
    }

    // The only lock taken here. The GUI thread is blocked for the whole synchronisation phase,
    // which is also what makes reading m_rows without a lock safe.
    CTraceBuffer::CReader r(*m_buffer);
    if (!r.valid())
    {
        trimChildren(root, 0);
        return root;
    }

    SRun      runs[2];
    const int nRuns = buildRuns(r.capacity(), r.filled(), r.writePos(), w, runs);
    if (nRuns == 0)
    {
        trimChildren(root, 0);
        return root;
    }

    const qreal  dpr      = window() != nullptr ? window()->effectiveDevicePixelRatio() : 1.0;
    const size_t nChannel = r.channels();
    int          used     = 0;

    for (const SRow& row : m_rows)
    {
        if (row.channel < 0 || size_t(row.channel) >= nChannel) { continue; }
        const float* src = r.channel(size_t(row.channel));

        for (int k = 0; k < nRuns; ++k)
        {
            const SRun& run = runs[k];
            if (run.count < 2) { continue; }

            // Pixel columns are anchored to the item's origin rather than to the run, so a given
            // ring slot always falls in the same column from one frame to the next. Partitioning
            // relative to the run instead would re-bin every sample whenever the run's length
            // changed -- which in Scan mode it does on every chunk, as the split at the write
            // cursor moves -- and the trace would shimmer even on a perfectly steady signal.
            const qreal colWidth = 1.0 / dpr;  // logical width of one device pixel
            const qreal runEnd   = run.x0 + qreal(run.count) * run.dx;
            const int   firstCol = int(std::floor(run.x0 / colWidth));
            const int   lastCol  = int(std::ceil(runEnd / colWidth));
            const int   columns  = std::max(2, lastCol - firstCol);
            const bool  decimate = run.count > size_t(columns);
            const int   vertices = decimate ? 2 * columns : int(run.count);

            QSGGeometryNode* node  = existingGeometryNode(root, used);
            const bool       isNew = node == nullptr;
            if (isNew) { node = new QSGGeometryNode; }
            ++used;

            QSGGeometry* geometry = node->geometry();
            if (geometry == nullptr)
            {
                geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertices);
                geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
                node->setGeometry(geometry);
                node->setFlag(QSGNode::OwnsGeometry);
            }
            else if (geometry->vertexCount() != vertices) { geometry->allocate(vertices); }

            QSGGeometry::Point2D* v = geometry->vertexDataAsPoint2D();

            if (!decimate)
            {
                for (size_t i = 0; i < run.count; ++i)
                {
                    const float s = src[run.first + i];
                    v[i].set(float(run.x0 + qreal(i) * run.dx), row.baseline - (s - row.centre) * row.gain);
                }
            }
            else
            {
                // Min/max per pixel column: two vertices per column, so the envelope of every
                // sample in that column is preserved exactly and no spike can be skipped.
                float prevY = std::numeric_limits<float>::quiet_NaN();
                for (int c = 0; c < columns; ++c)
                {
                    const qreal colStart = qreal(firstCol + c) * colWidth;

                    // Samples whose x lands in [colStart, colStart + colWidth).
                    int64_t a = int64_t(std::ceil((colStart - run.x0) / run.dx));
                    int64_t b = int64_t(std::ceil((colStart + colWidth - run.x0) / run.dx));
                    a         = std::max<int64_t>(a, 0);
                    b         = std::min<int64_t>(b, int64_t(run.count));
                    // A column narrower than one sample still has to draw something.
                    if (b <= a)
                    {
                        a = std::min<int64_t>(a, int64_t(run.count) - 1);
                        b = a + 1;
                    }

                    const size_t i0 = size_t(a);
                    const size_t i1 = size_t(b);

                    float lo = src[run.first + i0];
                    float hi = lo;
                    for (size_t i = i0 + 1; i < i1; ++i)
                    {
                        const float s = src[run.first + i];
                        lo            = std::min(lo, s);
                        hi            = std::max(hi, s);
                    }

                    const float yHi = row.baseline - (hi - row.centre) * row.gain;
                    const float yLo = row.baseline - (lo - row.centre) * row.gain;
                    const float x   = float(colStart + colWidth * 0.5);

                    // Emit the pair ordered by proximity to the previous column's exit vertex.
                    // Otherwise the joining segment is arbitrary and slow signals show a moire.
                    if (std::isnan(prevY) || std::abs(prevY - yHi) <= std::abs(prevY - yLo))
                    {
                        v[2 * c].set(x, yHi);
                        v[2 * c + 1].set(x, yLo);
                        prevY = yLo;
                    }
                    else
                    {
                        v[2 * c].set(x, yLo);
                        v[2 * c + 1].set(x, yHi);
                        prevY = yHi;
                    }
                }
            }
            node->markDirty(QSGNode::DirtyGeometry);

            auto* material = static_cast<QSGFlatColorMaterial*>(node->material());
            if (material == nullptr)
            {
                material = new QSGFlatColorMaterial;
                material->setColor(row.color);
                node->setMaterial(material);
                node->setFlag(QSGNode::OwnsMaterial);
            }
            else if (material->color() != row.color)
            {
                material->setColor(row.color);
                node->markDirty(QSGNode::DirtyMaterial);
            }

            // Only now that geometry and material are both in place is the node safe to publish.
            if (isNew) { root->appendChildNode(node); }
        }
    }

    trimChildren(root, used);
    return root;
}

void CSignalTraceItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() == oldGeometry.size()) { return; }

    if (recomputeLayout()) { emit layoutChanged(); }
    emit timeWindowChanged();
    update();
}

QVariantList CSignalTraceItem::rowBaselines() const
{
    QVariantList out;
    out.reserve(m_rows.size());
    for (const SRow& row : m_rows) { out.append(qreal(row.baseline)); }
    return out;
}

QVariantList CSignalTraceItem::rowChannels() const
{
    QVariantList out;
    out.reserve(m_rows.size());
    for (const SRow& row : m_rows) { out.append(row.channel); }
    return out;
}

QVariantList CSignalTraceItem::rowHalfRanges() const
{
    QVariantList out;
    out.reserve(m_rows.size());
    for (const SRow& row : m_rows)
    {
        double centre    = 0.0;
        double halfRange = 1.0;
        scaleFor(row.channel, centre, halfRange);
        out.append(halfRange);
    }
    return out;
}

qreal CSignalTraceItem::rowHeight() const
{
    if (m_rows.isEmpty() || height() <= 0.0) { return 0.0; }
    return m_settings.layout == ELayoutMode::Butterfly ? height() : height() / qreal(m_rows.size());
}

qreal CSignalTraceItem::windowSeconds() const
{
    if (m_buffer == nullptr) { return 0.0; }
    CTraceBuffer::CReader r(*m_buffer);
    if (!r.valid() || r.sampling() <= 0.0) { return 0.0; }
    return qreal(r.capacity()) / r.sampling();
}

qreal CSignalTraceItem::windowStartSeconds() const { return timeAtX(0.0); }

qreal CSignalTraceItem::oldestSeconds() const
{
    if (m_buffer == nullptr) { return 0.0; }
    CTraceBuffer::CReader r(*m_buffer);
    if (!r.valid() || r.sampling() <= 0.0) { return 0.0; }
    return r.firstSampleSeconds() + (double(r.totalSamples()) - double(r.filled())) / r.sampling();
}

qreal CSignalTraceItem::newestSeconds() const
{
    if (m_buffer == nullptr) { return 0.0; }
    CTraceBuffer::CReader r(*m_buffer);
    if (!r.valid() || r.sampling() <= 0.0) { return 0.0; }
    return r.firstSampleSeconds() + double(r.totalSamples()) / r.sampling();
}

qreal CSignalTraceItem::timeAtX(const qreal x) const
{
    if (m_buffer == nullptr || width() <= 0.0) { return 0.0; }
    CTraceBuffer::CReader r(*m_buffer);
    if (!r.valid() || r.sampling() <= 0.0) { return 0.0; }

    const size_t capacity = r.capacity();
    const double total    = double(r.totalSamples());
    const qreal  dx       = width() / qreal(capacity);

    double index;
    if (m_settings.timeMode == ESignalDisplayMode::Scroll) { index = total - 1.0 - (width() - x) / dx; }
    else
    {
        const double slot = x / dx;
        const double wp   = double(r.writePos());
        // How far back the sample currently held by that slot was written.
        const double back = slot < wp ? wp - slot : wp - slot + double(capacity);
        index             = total - back;
    }
    return r.firstSampleSeconds() + index / r.sampling();
}

qreal CSignalTraceItem::xAtTime(const qreal seconds) const
{
    if (m_buffer == nullptr || width() <= 0.0) { return -1.0; }
    CTraceBuffer::CReader r(*m_buffer);
    if (!r.valid() || r.sampling() <= 0.0) { return -1.0; }

    const size_t capacity = r.capacity();
    const double total    = double(r.totalSamples());
    const double index    = (seconds - r.firstSampleSeconds()) * r.sampling();
    const double oldest   = total - double(r.filled());
    if (index < oldest || index > total) { return -1.0; }

    const qreal dx = width() / qreal(capacity);
    if (m_settings.timeMode == ESignalDisplayMode::Scroll) { return width() - (total - 1.0 - index) * dx; }
    return std::fmod(index, double(capacity)) * dx;
}

qreal CSignalTraceItem::cursorX() const
{
    if (m_settings.timeMode != ESignalDisplayMode::Scan || m_buffer == nullptr || width() <= 0.0) { return -1.0; }
    CTraceBuffer::CReader r(*m_buffer);
    if (!r.valid() || r.capacity() == 0) { return -1.0; }
    return qreal(r.writePos()) * (width() / qreal(r.capacity()));
}

int CSignalTraceItem::channelAt(const qreal y) const
{
    if (m_rows.isEmpty() || height() <= 0.0 || m_settings.layout == ELayoutMode::Butterfly) { return -1; }
    const int i = int(y / (height() / qreal(m_rows.size())));
    if (i < 0 || i >= m_rows.size()) { return -1; }
    return m_rows.at(i).channel;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
