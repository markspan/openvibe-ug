// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/** Converts an OpenViBE 32:32 fixed-point time to seconds. */
inline double OVTimeToSeconds(const uint64_t t) { return double(t >> 32) + double(t & 0xFFFFFFFFULL) / 4294967296.0; }

/**
 * \brief A fixed-capacity, channel-major ring of samples shared between the player and the renderer.
 *
 * This replaces both halves of the old storage: the deque of heap-allocated chunks and the
 * per-channel QList of QPointF that duplicated it.
 *
 * Layout is channel-major (m_samples[c * m_capacity + slot]) so that it matches the incoming
 * CMatrix and so that the renderer's decimation pass streams linearly through one channel.
 * Samples are float: half the footprint of double and far beyond any amplifier's precision.
 *
 * There are no per-sample timestamps. The time of global sample g is
 * firstTime + g / sampling, which makes a per-channel time cursor -- and the drift bug it used
 * to cause -- structurally impossible: x is derived from the ring slot index alone.
 *
 * Threading: every public method locks. The player thread only ever calls appendChunk();
 * the GUI and render threads read through CReader.
 */
class CTraceBuffer
{
public:
    CTraceBuffer()                               = default;
    CTraceBuffer(const CTraceBuffer&)            = delete;
    CTraceBuffer& operator=(const CTraceBuffer&) = delete;

    /**
     * \brief Sets the geometry. Reallocates and drops history only if it actually changes.
     *
     * \param channels Number of channels.
     * \param samplingRate Sampling rate in Hz, or <= 0 to have it estimated from the first chunk.
     * \param windowSeconds Duration held by the ring.
     */
    void reconfigure(size_t channels, double samplingRate, double windowSeconds);

    /**
     * \brief Changes the displayed duration, keeping the channel count and sampling rate.
     *
     * History is discarded: resampling it would cost more than it is worth.
     */
    void setWindowSeconds(double windowSeconds);

    /**
     * \brief Appends one chunk. Called from the player thread and from nowhere else.
     *
     * If the sampling rate was unknown it is estimated here from the chunk duration, which is
     * what a StreamedMatrix input needs. A chunk longer than the ring keeps only its tail.
     *
     * \param buffer Channel-major [channels x samples], as delivered by CMatrix.
     */
    void appendChunk(const double* buffer, size_t channels, size_t samples, uint64_t startTime, uint64_t endTime);

    void clear();

    /** Cheap snapshot of the write counter, for change detection. */
    uint64_t totalSamples() const;

    /** RAII read accessor. Holds the lock for its whole lifetime, so keep it short-lived. */
    class CReader
    {
    public:
        explicit CReader(const CTraceBuffer& b) : m_b(b), m_lock(b.m_mutex) { }

        bool         valid() const { return m_b.m_channels != 0 && m_b.m_capacity != 0 && m_b.m_total != 0; }
        size_t       channels() const { return m_b.m_channels; }
        size_t       capacity() const { return m_b.m_capacity; }
        double       sampling() const { return m_b.m_sampling; }
        uint64_t     totalSamples() const { return m_b.m_total; }
        size_t       writePos() const { return m_b.m_writePos; }
        double       firstSampleSeconds() const { return m_b.m_firstSampleSeconds; }
        /** Number of slots currently holding valid data. */
        size_t       filled() const { return size_t(m_b.m_total < uint64_t(m_b.m_capacity) ? m_b.m_total : uint64_t(m_b.m_capacity)); }
        /** Contiguous base pointer of one channel, capacity() slots long. */
        const float* channel(const size_t c) const { return m_b.m_samples.data() + c * m_b.m_capacity; }

    private:
        const CTraceBuffer&          m_b;
        std::unique_lock<std::mutex> m_lock;
    };

private:
    friend class CReader;

    /** Reallocates m_samples from m_channels / m_sampling / m_windowSeconds. Caller holds the lock. */
    void reallocate();

    mutable std::mutex m_mutex;
    std::vector<float> m_samples;               ///< channel-major, m_channels * m_capacity entries
    size_t             m_channels      = 0;
    size_t             m_capacity      = 0;     ///< slots per channel
    size_t             m_writePos      = 0;     ///< next slot to write
    uint64_t           m_total         = 0;     ///< samples per channel written since the last clear()
    double             m_sampling      = 0.0;   ///< Hz, 0 until known
    double             m_windowSeconds = 10.0;
    double             m_firstSampleSeconds = 0.0;  ///< time of global sample 0
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
