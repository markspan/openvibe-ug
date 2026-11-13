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
 * Threading: every public method locks. The player thread calls reconfigure() and appendChunk();
 * the GUI and render threads read through CReader.
 *
 * UG fork: the slot of global sample g is always (g + slotOffset()) % capacity, also after a chunk longer than the ring,
 * and writePos() is always (totalSamples() + slotOffset()) % capacity(). The time ruler and the markers rely on both.
 * slotOffset() is 0 unless the ring is time-locked, see setTimeOrigin().
 */
class CTraceBuffer
{
public:
    CTraceBuffer()                               = default;
    CTraceBuffer(const CTraceBuffer&)            = delete;
    CTraceBuffer& operator=(const CTraceBuffer&) = delete;

    /**
     * \brief Sets the stream's geometry. Reallocates and drops history only if it actually changes.
     *
     * UG fork: the duration held is kept, see setWindowSeconds(), so that the player thread can call this on a
     * header without reading the display settings, which the GUI thread owns.
     *
     * \param channels Number of channels.
     * \param samplingRate Sampling rate in Hz, or <= 0 to have it estimated from the first chunk.
     */
    void reconfigure(size_t channels, double samplingRate);

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

    /**
     * \brief Locks the ring to a clock shared with other rings, so that rings of different rates line up.
     *
     * UG fork, for the LSL display. Sample slots then lie on a grid of that clock: the sample dated t sits in slot
     * round((t - origin) * rate) % capacity, and the time of a slot is the grid's. So in Scan mode every ring with the same
     * window draws a time at the same x, whatever its rate or when it started. Call before the first sample, with the same
     * origin for every ring that is to line up; samples come through appendTimed().
     */
    void setTimeOrigin(double originSeconds);

    /**
     * \brief Appends one chunk dated by its first sample, keeping each sample at the slot of its time.
     *
     * UG fork, for the LSL display. Every chunk is placed by its own date on the grid of setTimeOrigin(): the ring starts at
     * the grid point of its first sample, so that its times are the grid's, in every ring alike. A stream's samples do not
     * come exactly at its nominal rate, and may have gaps. When a chunk's date and the samples counted so far disagree by
     * RESYNC_SAMPLES or more, the count follows the date: a gap is filled with each channel's last value, and samples come in
     * early overwrite the last ones. So every sample lies less than RESYNC_SAMPLES from its date, plus half a sample of
     * grid, for hours, at the cost of a sample skipped or repeated now and then.
     *
     * \param buffer             Channel-major [channels x samples].
     * \param firstSampleSeconds The date of the chunk's first sample, on the clock of setTimeOrigin().
     */
    void appendTimed(const double* buffer, size_t channels, size_t samples, double firstSampleSeconds);

    /// How far a chunk's date may be from the samples counted before appendTimed() corrects the count. UG fork.
    static constexpr int64_t RESYNC_SAMPLES = 2;

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
        /// UG fork: the slot of global sample 0; 0 unless the ring is time-locked
        size_t       slotOffset() const { return m_b.m_slotOffset; }
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

    /**
     * \brief Writes a chunk at m_writePos and moves on by all its samples. UG fork: shared by both appends.
     * \param buffer Channel-major [channels x samples]; of a chunk longer than the ring, only the tail is written.
     */
    void write(const double* buffer, size_t samples);

    /// Moves the count by \p samples: forward over a gap, filled with the last values, or back over samples to overwrite. UG fork.
    void shift(int64_t samples);

    mutable std::mutex m_mutex;
    std::vector<float> m_samples;               ///< channel-major, m_channels * m_capacity entries
    size_t             m_channels      = 0;
    size_t             m_capacity      = 0;     ///< slots per channel
    size_t             m_writePos      = 0;     ///< next slot to write
    uint64_t           m_total         = 0;     ///< samples per channel received since the last reallocation
    double             m_sampling      = 0.0;   ///< Hz, 0 until known
    double             m_windowSeconds = 10.0;
    double             m_firstSampleSeconds = 0.0;  ///< time of global sample 0
    bool               m_timeLocked    = false; ///< UG fork: see setTimeOrigin()
    double             m_originSeconds = 0.0;   ///< UG fork: see setTimeOrigin()
    size_t             m_slotOffset    = 0;     ///< UG fork: the slot of global sample 0
    int64_t            m_startIndex    = 0;     ///< UG fork: the grid index of global sample 0, see setTimeOrigin()
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
