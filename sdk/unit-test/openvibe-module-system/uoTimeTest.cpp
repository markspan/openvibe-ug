///-------------------------------------------------------------------------------------------------
/// 
/// \file uoTimeTest.cpp
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include <iostream>
#include <limits>
#include <vector>
#include <cmath>
#include <tuple>
#include <chrono>

#include "openvibe/CTime.hpp"
#include "system/ovCTime.h"

#include "ovtAssert.h"

//
// \note This test should be improved. Some improvements could be:
//       - Compare clock results to gold standard
//       - True testing of monotonic state
//       - Stress tests on longer period
//

// \brief Calibrate sleep function to estimate the extra time not spent at sleeping
uint64_t calibrateSleep(const size_t nSample, bool (*sleepFunction)(uint64_t), uint64_t (*timeFunction)())
{
    uint64_t maxTime = 0;
    for (size_t i = 0; i < nSample; ++i) {
        const uint64_t preTime = timeFunction();
        sleepFunction(0);
        const uint64_t processingTime = timeFunction() - preTime;

        if (processingTime > maxTime) { maxTime = processingTime; }
    }

    return maxTime;
}

// \brief Record sleep function precision
std::vector<uint64_t> testSleep(const std::vector<uint64_t>& sleepTimes, bool (*sleepFunction)(uint64_t), uint64_t (*timeFunction)())
{
    std::vector<uint64_t> effectiveSleepTimes;
    for (const auto& time : sleepTimes) {
        const uint64_t preTime = timeFunction();
        sleepFunction(time);
        effectiveSleepTimes.push_back(timeFunction() - preTime);
    }

    return effectiveSleepTimes;
}

// \brief Return a warning count that is incremented when sleep function did not meet following requirements:
//       - sleep enough time
//       - sleep less than the expected time + delta
size_t assessSleepTestResult(const std::vector<uint64_t>& expected, const std::vector<uint64_t>& result, const uint64_t delta, const uint64_t epsilon)
{
    size_t warningCount = 0;
    for (size_t i = 0; i < expected.size(); ++i) {
        if (result[i] + epsilon < expected[i] || result[i] > (expected[i] + delta + epsilon)) {
            std::cerr << "WARNING: Failure to sleep the right amount of time: [expected|result] = "
                    << OpenViBE::CTime(expected[i]) << "|" << OpenViBE::CTime(result[i]) << std::endl;
            warningCount++;
        }
    }

    return warningCount;
}

// \brief Record clock function data (spin test taken from OpenViBE). Return a tuple with:
//       - bool = monotonic state
//       - std::vector<uint64_t> = all the cumulative steps
std::tuple<bool, std::vector<uint64_t>> testClock(const uint64_t samplePeriod, const unsigned sampleCountGuess, uint64_t (*timeFunction)())
{
    std::vector<uint64_t> cumulativeSteps;
    cumulativeSteps.reserve(sampleCountGuess);

    bool monotonic           = true;
    const uint64_t startTime = timeFunction();
    uint64_t nowTime         = startTime;
    uint64_t previousTime    = nowTime;

    while (nowTime - startTime < samplePeriod) {
        nowTime = timeFunction();
        if (nowTime > previousTime) { cumulativeSteps.push_back(nowTime - previousTime); }
        else if (nowTime < previousTime) {
            monotonic = false;
            break;
        }
        previousTime = nowTime;
    }

    return std::make_tuple(monotonic, cumulativeSteps);
}

// \brief Compute jitter measurements for 32:32 time. Return a tuple with:
//       - double = mean
//       - double = max deviation from mean
//       - double = RMSE
std::tuple<double, double, double> assessTimeClock(const std::vector<uint64_t>& measurements)
{
    double jitterMax = 0.0;
    double jitterMSE = 0.0;
    double mean      = 0.0;

    // compute mean
    for (auto& data : measurements) {
        // convert data
        auto seconds                         = data >> 32;
        auto microseconds                    = ((data & 0xFFFFFFFFLL) * 1000000LL) >> 32;
        std::chrono::microseconds chronoData = std::chrono::seconds(seconds) + std::chrono::microseconds(microseconds);

        mean += double(chronoData.count()) / double(1000 * measurements.size());
    }

    // compute deviations
    for (auto& data : measurements) {
        // convert data
        auto seconds                         = data >> 32;
        auto microseconds                    = ((data & 0xFFFFFFFFLL) * 1000000LL) >> 32;
        std::chrono::microseconds chronoData = std::chrono::seconds(seconds) + std::chrono::microseconds(microseconds);

        const double deviation = std::abs(double(chronoData.count()) / 1000 - mean);
        jitterMSE += std::pow(deviation, 2) / double(measurements.size());

        if (deviation - jitterMax > std::numeric_limits<double>::epsilon()) { jitterMax = deviation; }
    }

    return std::make_tuple(mean, jitterMax, std::sqrt(jitterMSE));
}

int uoTimeTest(int /*argc*/, char* /*argv*/[])
{
    // This is a very theoretical test. But if it returns false, we can
    // assume that a steady clock is not available on the test
    // platform. If it returns true, it just means that the internal clock
    // says it is steady...
    OVT_ASSERT(System::Time::isClockSteady(), "Failure to retrieve a steady clock");

    // Same as above.
    // The test is set to 1ms at it is a requirement for OpenViBE clocks
    OVT_ASSERT(System::Time::checkResolution(1), "Failure to check for resolution");

    // A stress test to check no overflow happens
    OVT_ASSERT(System::Time::checkResolution(std::numeric_limits<size_t >::max()), "Failure to check for resolution");

    //
    // zSleep() function test
    //

    const std::vector<uint64_t> expectedSleepData = {
        0x80000000LL, 0x40000000LL, 0x20000000LL, 0x10000000LL,
        0x80000000LL, 0x40000000LL, 0x20000000LL, 0x10000000LL,
        0x08000000LL, 0x04000000LL, 0x02000000LL, 0x01000000LL,
        0x08000000LL, 0x04000000LL, 0x02000000LL, 0x01000000LL
    };

    // calibrate sleep function
    const auto deltaTime = calibrateSleep(1000, System::Time::zsleep, System::Time::zgetTime);

    std::cout << "INFO: Delta time for zsleep calibration = " << OpenViBE::CTime(deltaTime) << std::endl;

    const auto resultSleepData = testSleep(expectedSleepData, System::Time::zsleep, System::Time::zgetTime);

    OVT_ASSERT(resultSleepData.size() == expectedSleepData.size(), "Failure to run zsleep tests");

    const size_t warningCount = assessSleepTestResult(expectedSleepData, resultSleepData, deltaTime, OpenViBE::CTime(0.005).time());

    // relax this threshold in case there is some recurrent problems
    // according to the runtime environment
    OVT_ASSERT(warningCount <= 2, "Failure to zsleep the right amount of time");

    //
    // zGetTime() function test
    //

    // the sample count guess was found in an empiric way
    const auto resultGetTimeData = testClock(OpenViBE::CTime(0.5).time(), 500000, System::Time::zgetTime);

    OVT_ASSERT(std::get<0>(resultGetTimeData), "Failure in zgetTime() test: the clock is not monotonic");

    const auto clockMetrics = assessTimeClock(std::get<1>(resultGetTimeData));

    std::cout << "INFO: Sample count for getTime() = " << std::get<1>(resultGetTimeData).size() << std::endl;
    std::cout << "INFO: Mean step in ms for getTime() = " << std::get<0>(clockMetrics) << std::endl;
    std::cout << "INFO: Max deviation in ms for getTime() = " << std::get<1>(clockMetrics) << std::endl;
    std::cout << "INFO: RMSE in ms for getTime() = " << std::get<2>(clockMetrics) << std::endl;

    // We expect at least 1ms resolution
    const double resolutionDelta = std::get<0>(clockMetrics) - 1;
    OVT_ASSERT(resolutionDelta <= std::numeric_limits<double>::epsilon(), "Failure in zgetTime() test: the clock resolution does not match requirements");

    return EXIT_SUCCESS;
}
