// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CTraceBuffer.h"

#include <algorithm>
#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {
/** Never let a pathological header allocate an unbounded ring (256 M samples/channel). */
constexpr size_t MAX_CAPACITY = 256u * 1024u * 1024u;
}  // namespace

void CTraceBuffer::reallocate()
{
    size_t capacity = 0;
    if (m_sampling > 0.0 && m_windowSeconds > 0.0)
    {
        const double slots = std::ceil(m_sampling * m_windowSeconds);
        if (slots >= 1.0) { capacity = slots > double(MAX_CAPACITY) ? MAX_CAPACITY : size_t(slots); }
    }

    m_capacity = capacity;
    m_writePos = 0;
    m_total    = 0;
    m_samples.assign(m_channels * m_capacity, 0.0F);
}

void CTraceBuffer::reconfigure(const size_t channels, const double samplingRate)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const double sampling = samplingRate > 0.0 ? samplingRate : 0.0;
    if (channels == m_channels && sampling == m_sampling) { return; }

    m_channels = channels;
    m_sampling = sampling;
    reallocate();
}

void CTraceBuffer::setWindowSeconds(const double windowSeconds)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (windowSeconds <= 0.0 || windowSeconds == m_windowSeconds) { return; }

    m_windowSeconds = windowSeconds;
    reallocate();
}

uint64_t CTraceBuffer::totalSamples() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_total;
}

void CTraceBuffer::appendChunk(const double* buffer, const size_t channels, const size_t samples, const uint64_t startTime, const uint64_t endTime)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (buffer == nullptr || samples == 0 || channels == 0 || channels != m_channels) { return; }

    // A StreamedMatrix input carries no sampling rate; derive it from the first chunk's geometry.
    if (m_sampling <= 0.0)
    {
        const double duration = OVTimeToSeconds(endTime) - OVTimeToSeconds(startTime);
        if (duration <= 0.0) { return; }
        m_sampling = double(samples) / duration;
        reallocate();
    }
    if (m_capacity == 0) { return; }

    if (m_total == 0) { m_firstSampleSeconds = OVTimeToSeconds(startTime); }

    // A chunk longer than the whole ring: only its tail is kept, as the rest would be overwritten at once.
    // UG fork: the skipped samples still count, and the tail goes to the slots it would have reached. Upstream
    // counted only the tail, so every later sample, and so the time ruler and the markers, was dated too early by
    // the samples skipped.
    const size_t skipped = samples > m_capacity ? samples - m_capacity : 0;
    const size_t count   = samples - skipped;
    const size_t first   = (m_writePos + skipped) % m_capacity;

    // The write splits into at most two contiguous spans: up to the ring end, then from the start.
    const size_t head = std::min(count, m_capacity - first);
    const size_t tail = count - head;

    for (size_t c = 0; c < m_channels; ++c)
    {
        const double* src = buffer + c * samples + skipped;
        float*        dst = m_samples.data() + c * m_capacity;
        for (size_t i = 0; i < head; ++i) { dst[first + i] = float(src[i]); }
        for (size_t i = 0; i < tail; ++i) { dst[i] = float(src[head + i]); }
    }

    m_writePos = (first + count) % m_capacity;
    m_total += samples;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
