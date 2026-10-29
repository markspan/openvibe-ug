/**
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file DriftCorrector.cpp
 * @brief Implementation of the DriftCorrector class.
 */

#include "DriftCorrector.h"

#include <algorithm>
#include <cstdlib>

DriftCorrector::DriftCorrector(const uint32_t samplingRate, const uint32_t channelCount, const uint32_t toleranceMs, const size_t estimationCount)
    : m_periodNs(1'000'000'000LL / int64_t(samplingRate)),
      m_channelCount(channelCount),
      m_toleranceNs(int64_t(toleranceMs) * 1'000'000LL),
      m_estimationCount(std::max<size_t>(estimationCount, 1))
{ }

int64_t DriftCorrector::process(samplesBuffer* buff)
{
    if (!buff || buff->count == 0) { return 0; }

    const int64_t lastStamp = int64_t(buff->timestampsBuff[buff->count - 1]);
    if (!m_anchored) {
        // The same anchor as the drift display: the first buffer's last sample, counted back at the declared rate
        m_anchor   = lastStamp - int64_t(buff->count) * m_periodNs;
        m_anchored = true;
    }
    m_sent += int64_t(buff->count);

    // Positive when the samples come later than the elapsed time says they should, i.e. when samples are missing
    const int64_t lateness = lastStamp - (m_anchor + m_sent * m_periodNs);
    m_lateness.push_back(lateness);
    m_latenessSum += lateness;
    if (m_lateness.size() > m_estimationCount) {
        m_latenessSum -= m_lateness.front();
        m_lateness.pop_front();
    }
    if (m_lateness.size() < m_estimationCount) { return 0; }

    const int64_t drift = m_latenessSum / int64_t(m_lateness.size());
    if (std::llabs(drift) <= m_toleranceNs) { return 0; }

    // Whole samples only, towards zero, so that a correction never overshoots
    int64_t correction = drift / m_periodNs;
    if (correction > 0) {
        correction = int64_t(std::min<uint64_t>(uint64_t(correction), buff->maxSampleCount - buff->count));
        addSamples(buff, uint64_t(correction));
        m_added += uint64_t(correction);
    }
    else if (correction < 0) {
        // Keep one sample, so the buffer keeps a timestamp to go by. What is left over is removed from the next buffers.
        correction = -int64_t(std::min<uint64_t>(uint64_t(-correction), buff->count - 1));
        removeSamples(buff, uint64_t(-correction));
        m_removed += uint64_t(-correction);
    }
    if (correction == 0) { return 0; }

    // The past measures now have to be read against the corrected sample count
    m_sent += correction;
    for (auto& value : m_lateness) { value -= correction * m_periodNs; }
    m_latenessSum -= correction * m_periodNs * int64_t(m_lateness.size());
    m_corrections++;
    return correction;
}

void DriftCorrector::addSamples(samplesBuffer* buff, const uint64_t count) const
{
    const sampleType* last    = buff->samplesBuff + (buff->count - 1) * m_channelCount;
    const timestampType stamp = buff->timestampsBuff[buff->count - 1];
    for (uint64_t i = 0; i < count; ++i) {
        std::copy(last, last + m_channelCount, buff->samplesBuff + (buff->count + i) * m_channelCount);
        buff->timestampsBuff[buff->count + i] = stamp;
    }
    buff->count += count;
}

void DriftCorrector::removeSamples(samplesBuffer* buff, const uint64_t count)
{
    buff->count -= count;
    const timestampType lastStamp = buff->timestampsBuff[buff->count - 1];
    for (uint32_t i = 0; i < buff->stimCount; ++i) {
        if (buff->stimulationsBuff[i].date > lastStamp) { buff->stimulationsBuff[i].date = lastStamp; }
    }
}
