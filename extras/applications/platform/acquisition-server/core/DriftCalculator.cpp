/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file DriftCalculator.cpp
 * @brief Implementation file for the DriftCalculator class.
 *
 * This file implements the methods of the `DriftCalculator` class, which calculates
 * various latency and drift metrics during data acquisition.
 */

#include "DriftCalculator.h"
#include <chrono>
#include <thread>

using Clock =std::chrono::steady_clock;
 /**
  * @brief Constructor for the DriftCalculator class.
  *
  * Initializes the DriftCalculator object with the provided sample frequency and
  * timestamp window size. Also initializes all latency and timestamp members to 0.
  *
  * @param sampleFrequency The sampling frequency of the acquired data in samples per second.
  * @param timestampsWindowSize The size of the sliding window used to calculate current latency.
  */
DriftCalculator::DriftCalculator(unsigned int sampleFrequency, unsigned int timestampsWindowSize) :
    m_sampleFrequency(sampleFrequency),
    m_timestampsWindowSize(timestampsWindowSize),
    m_startTimeAcq(0),
    m_stopTimeAcq(0),
    m_firstTimestamp(0),
    m_sampleCount(0),
    m_startLatency(0),
    m_globalLatency(0),
    m_currentLatency(0),
    m_currentTimestamp(0),
    m_endLatency(0),
    m_loopDuration(0)
{
    m_period_ns = 1000000000 / m_sampleFrequency; ///< Calculate the period in nanoseconds based on the sample frequency.
    //m_timestampsWindow.reserve(m_timestampsWindowSize); ///< Reserve space for the timestamp window to avoid frequent reallocations.
}

/**
 * @brief Fills the timestamp window with new timestamps.
 *
 * Acquires a lock on the timestamp window, adds the new timestamps to the back
 * of the window, and removes old timestamps from the front if the window size
 * is exceeded. Updates the `m_currentTimestamp` with the last timestamp added.
 *
 * @param newTimestamps A constant reference to a vector of new timestamps.
 */
void DriftCalculator::fillNewTimestamps(std::vector<uint64_t> const& newTimestamps)
{
    std::lock_guard<std::mutex> lock(m_timestampsWindowMutex); ///< Acquire a lock to protect the timestamp window.
    for (auto const& timestamp : newTimestamps) ///< Iterate through the new timestamps.
    {
        if (m_timestampsWindow.size() == m_timestampsWindowSize) ///< Check if the window is full.
        {
            m_timestampsWindow.pop_front(); ///< Remove the oldest timestamp from the front.
        }
        m_timestampsWindow.push_back(timestamp); ///< Add the new timestamp to the back of the window.
    }
    if (!newTimestamps.empty())
    {
        m_currentTimestamp = newTimestamps.back(); ///< Update the current timestamp.
        m_sampleCount += newTimestamps.size();
    }
}

/**
 * @brief Reads the calculated drift and latency values.
 *
 * Copies the current values of start latency, global latency, current latency,
 * and end latency to the provided output parameters.
 *
 * @param drift A reference to a `drift_st` structure where the calculated drift values will be stored.
 */
void DriftCalculator::readDrifts(drift_st & drift) const
{
    drift.startLatency = m_startLatency;
    drift.globalDrift = -m_globalLatency;
    drift.instantDrift = -m_currentLatency;
    drift.endLatency = m_endLatency;
}

/**
 * @brief Computes the global latency.
 *
 * Calculates the global latency by comparing the acquisition elapsed time (current timestamp - first timestamp)
 * with the total number of samples acquired (sample count * period).
 */
void DriftCalculator::computeGlobalLatency() {
    std::lock_guard<std::mutex> lock(m_timestampsWindowMutex); ///< Prevent observing m_currentTimestamp/m_sampleCount mid-update (see fillNewTimestamps()).
    m_globalLatency = (m_currentTimestamp - m_firstTimestamp) - (m_sampleCount * m_period_ns);
}

/**
 * @brief Computes the current latency based on the timestamp window.
 *
 * Acquires a lock on the timestamp window and calculates the average of the
 * timestamps within the window. This average is then stored as the current latency.
 * If the window is empty or its size doesn't match the expected size, the current
 * latency is set to 0.
 */
void DriftCalculator::computeCurrentLatency()
{
    std::lock_guard<std::mutex> lock(m_timestampsWindowMutex); ///< Acquire a lock to protect the timestamp window.
    if (m_timestampsWindowSize == m_timestampsWindow.size() && !m_timestampsWindow.empty()) ///< Check if the window has the expected size and is not empty.
    {
        int64_t avrg = 0;
        //for (auto const& sample : m_timestampsWindow) ///< Iterate through the timestamps in the window.
        
        for(uint32_t i=0; i<(m_timestampsWindowSize-1); ++i)
        {
            avrg += m_timestampsWindow[i + 1] - m_timestampsWindow[i];
        }
        avrg /= m_timestampsWindowSize-1; ///< Calculate the average timestamp.

        m_currentLatency = avrg - m_period_ns; ///< Set the current latency to the average timestamp.
    }
    else
    {
        m_currentLatency = 0; ///< Set current latency to 0 if the window conditions are not met.
    }
}

/**
 * @brief Computes the latency at the end of the acquisition.
 *
 * If the stop time of acquisition has been set, this method calculates the
 * difference between the current timestamp and the stop time and stores it as
 * the end latency.
 */
void DriftCalculator::computeEndLatency()
{
    if (m_stopTimeAcq) ///< Check if the stop time has been set.
    {
        m_endLatency = m_currentTimestamp - m_stopTimeAcq; ///< Calculate the end latency.
    }
}

/**
 * @brief Performs the sequence of latency computations.
 *
 * This method calls the individual compute methods to update the start latency,
 * global latency, current latency, and end latency.
 */
void DriftCalculator::treatment()
{
    computeStartLatency();
    computeGlobalLatency();
    computeCurrentLatency();
    computeEndLatency();
}

/**
 * @brief Runs the drift calculation logic in a loop.
 *
 * This method is intended to be run in a separate thread. It periodically calls
 * the `treatment()` method to update the latency values. It also includes a
 * mechanism to adjust the sleep duration to maintain a target refresh frequency.
 */
void DriftCalculator::run()
{
    uint64_t sleepDuration_ms = 20; // Ensure eye's refresh x2 (50 img per second), so a target of 100 updates per second.
    auto treatmentBeginTime = Clock::now(); ///< Record the start time of the treatment.

    treatment(); ///< Perform the latency calculations.

    auto treatmentEndTime = Clock::now(); ///< Record the end time of the treatment.
    // Adjust the sleep duration to try and maintain a consistent update frequency.
    uint64_t treatmentDuration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(treatmentEndTime - treatmentBeginTime).count();
    sleepDuration_ms = m_loopDuration == 0 ? sleepDuration_ms : (sleepDuration_ms > treatmentDuration_ms ? sleepDuration_ms - treatmentDuration_ms : 1); // Ensure a minimum sleep of 1 ms to avoid busy-waiting.

    std::this_thread::sleep_for(std::chrono::milliseconds(sleepDuration_ms)); ///< Sleep for the calculated duration.

    m_loopDuration = std::chrono::duration_cast<std::chrono::milliseconds>(treatmentEndTime - treatmentBeginTime).count(); ///< Store the duration of the last loop.
}
