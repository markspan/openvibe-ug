#pragma once
/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file DriftCalculator.h
 * @brief Header file for the DriftCalculator class.
 *
 * This file declares the `DriftCalculator` class, which is responsible for
 * calculating various latency and drift metrics during data acquisition.
 */

#include <cstdint>
#include <deque>
#include <vector>
#include <atomic>
#include <mutex>

#include <openvibe/acquisition-messaging/drift.h>

 /**
  * @brief Calculates drift and latency metrics during data acquisition.
  *
  * The `DriftCalculator` class monitors the timestamps of acquired data and
  * calculates metrics such as start latency, global latency, current latency,
  * and end latency. It uses a sliding window of timestamps to compute these values.
  */
class DriftCalculator
{
public:
    DriftCalculator(unsigned int sampleFrequency, unsigned int timestampsWindowSize);
    ~DriftCalculator() = default;

    void setStartTimeAcq(uint64_t startTimeAcq) { m_startTimeAcq = startTimeAcq; }
    void setStopTimeAcq(uint64_t stopTimeAcq) { m_stopTimeAcq = stopTimeAcq; }
    void setFirstTimeStamp(uint64_t firstTimestamp) { m_firstTimestamp = firstTimestamp; }
    void setSampleCount(uint64_t sampleCount) { m_sampleCount = sampleCount; }
    
    void fillNewTimestamps(std::vector< uint64_t > const& newTimestamps);
    void readDrifts(drift_st & drift) const;
    
    void run();

private:
    void computeStartLatency() { m_startLatency = m_firstTimestamp - m_startTimeAcq; }
    void computeGlobalLatency();
    void computeCurrentLatency();
    void computeEndLatency();
    
    void treatment();

private:
    std::atomic<int64_t> m_startTimeAcq; ///< Atomic variable to store the start time of acquisition.
    std::atomic<int64_t> m_stopTimeAcq; ///< Atomic variable to store the stop time of acquisition.

    std::atomic<int64_t> m_sampleCount; ///< Atomic variable to store the total number of acquired samples.
    std::atomic<int64_t> m_sampleFrequency; ///< Atomic variable to store the sampling frequency.
    std::atomic<int64_t> m_period_ns; ///< Atomic variable to store the period of sampling in nanoseconds.

    std::atomic<int64_t> m_firstTimestamp; ///< Atomic variable to store the timestamp of the first acquired sample.
    std::atomic<int64_t> m_currentTimestamp; ///< Atomic variable to store the timestamp of the most recently processed sample.
    unsigned int m_timestampsWindowSize; ///< Size of the sliding window for current latency calculation.

    std::mutex m_timestampsWindowMutex; ///< Mutex to protect access to the timestamp window.
    std::deque< int64_t > m_timestampsWindow; ///< Deque to store the sliding window of timestamps.

    std::atomic<int64_t> m_startLatency; ///< Atomic variable to store the calculated start latency.
    std::atomic<int64_t> m_globalLatency; ///< Atomic variable to store the calculated global latency.
    std::atomic<int64_t> m_currentLatency; ///< Atomic variable to store the calculated current latency.
    std::atomic<int64_t> m_endLatency; ///< Atomic variable to store the calculated end latency.

    uint64_t m_loopDuration; ///< Stores the duration of the last run loop.
};
