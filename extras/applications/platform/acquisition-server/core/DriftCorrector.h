#pragma once
/**
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file DriftCorrector.h
 * @brief Keeps the number of samples sent in line with the elapsed time, as the 3.x server did.
 *
 * UG fork: new file. Upstream OpenViBE 4.0 measures drift (DriftCalculator) but does not correct it.
 */

#include "samplesBuffer.h"

#include <cstdint>
#include <deque>

/**
 * @brief Adds or removes samples so that the stream keeps pace with the computer's clock.
 *
 * An amplifier's clock never runs at exactly the declared rate, so over a session it delivers a few samples more
 * or fewer than the elapsed time calls for. For every buffer this class measures how far the samples received so
 * far lag behind or run ahead of the time, and averages that over the last buffers to smooth out the jitter of
 * the delivery. Once the average exceeds the tolerance, it corrects the difference at the end of the buffer at
 * hand: by repeating the last sample when samples are missing, by dropping the last samples when there are too
 * many. Stimulations on dropped samples move to the new last sample, so none is lost.
 *
 * Added samples share the timestamp of the sample they repeat, so that a stimulation dated at that sample stays
 * on it, and timestamps never go backwards.
 */
class DriftCorrector
{
public:
    /**
     * @param samplingRate     Declared sampling rate, in Hz.
     * @param channelCount     Number of channels in a sample.
     * @param toleranceMs      Drift that is left alone, in ms.
     * @param estimationCount  Number of buffers the drift is averaged over.
     */
    DriftCorrector(uint32_t samplingRate, uint32_t channelCount, uint32_t toleranceMs, size_t estimationCount = 128);

    /**
     * Measures the drift with a buffer whose timestamps are all set and corrects it in that buffer if needed.
     * @return The number of samples added, negative when samples were removed.
     */
    int64_t process(samplesBuffer* buff);

    uint64_t addedCount() const { return m_added; }
    uint64_t removedCount() const { return m_removed; }
    uint64_t correctionCount() const { return m_corrections; }

private:
    void addSamples(samplesBuffer* buff, uint64_t count) const;
    static void removeSamples(samplesBuffer* buff, uint64_t count);

    int64_t  m_periodNs;
    uint32_t m_channelCount;
    int64_t  m_toleranceNs;
    size_t   m_estimationCount;

    bool    m_anchored = false;
    int64_t m_anchor   = 0;  ///< Time the first sample was due, derived from the first buffer.
    int64_t m_sent     = 0;  ///< Samples handed on so far, corrections included.

    std::deque<int64_t> m_lateness;  ///< How late the last samples of the recent buffers came, in ns.
    int64_t m_latenessSum = 0;

    uint64_t m_added       = 0;
    uint64_t m_removed     = 0;
    uint64_t m_corrections = 0;
};
