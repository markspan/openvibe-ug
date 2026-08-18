/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "DriftCalculator.h"

namespace {

class DriftCalculatorTest : public ::testing::Test
{
protected:
    static constexpr unsigned int sampleFrequency = 100;    // 100 Hz
    static constexpr int64_t periodNs             = 10'000'000; // 1e9 / SampleFrequency
    static constexpr unsigned int windowSize       = 5;
};

} // namespace

TEST_F(DriftCalculatorTest, StartLatencyIsFirstTimestampMinusStartTime)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    calc.setStartTimeAcq(1000);
    calc.setFirstTimeStamp(1500);
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.startLatency, 500);
}

TEST_F(DriftCalculatorTest, GlobalDriftIsZeroWhenSamplesArriveOnSchedule)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    calc.setFirstTimeStamp(0);

    std::vector<uint64_t> timestamps;
    for (int i = 1; i <= 10; ++i) {
        timestamps.push_back(static_cast<uint64_t>(i) * periodNs);
    }
    calc.fillNewTimestamps(timestamps);
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.globalDrift, 0);
}

TEST_F(DriftCalculatorTest, GlobalDriftReflectsAccumulatedDelay)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    calc.setFirstTimeStamp(0);

    std::vector<uint64_t> timestamps;
    for (int i = 1; i <= 9; ++i) {
        timestamps.push_back(static_cast<uint64_t>(i) * periodNs);
    }
    timestamps.push_back(10 * periodNs + 5'000'000); // last sample arrives 5ms late
    calc.fillNewTimestamps(timestamps);
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.globalDrift, -5'000'000);
}

TEST_F(DriftCalculatorTest, GlobalDriftIsZeroWhenAnchorDerivedFromLastRealSample)
{
    // Mirrors Acquisition::processBuffer()'s current first-buffer handling: the anchor is
    // derived from the one real timestamp in the buffer (its last sample) minus count*period,
    // and the full buffer (including what would be "sample 0") is fed to fillNewTimestamps().
    DriftCalculator calc(sampleFrequency, windowSize);

    std::vector<uint64_t> firstBuffer;
    for (int i = 1; i <= 10; ++i) {
        firstBuffer.push_back(static_cast<uint64_t>(i) * periodNs);
    }
    uint64_t const anchor = firstBuffer.back() - firstBuffer.size() * periodNs;

    calc.setFirstTimeStamp(anchor);
    calc.fillNewTimestamps(firstBuffer);
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.globalDrift, 0);
}

TEST_F(DriftCalculatorTest, InstantDriftIsZeroWhenWindowIsNotFull)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    calc.fillNewTimestamps({periodNs, 2 * periodNs, 3 * periodNs}); // fewer than windowSize
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.instantDrift, 0);
}

TEST_F(DriftCalculatorTest, InstantDriftReflectsWindowAverageWhenFull)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    // 5 timestamps, 11ms apart (period + 1ms) -> avg delta 11ms, drift = -(11ms - 10ms)
    calc.fillNewTimestamps({0, 11'000'000, 22'000'000, 33'000'000, 44'000'000});
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.instantDrift, -1'000'000);
}

TEST_F(DriftCalculatorTest, InstantDriftReflectsOnlyMostRecentWindow)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    // Fill window with nominal cadence (10ms), then fully evict it with a new
    // batch of windowSize timestamps at 15ms cadence (period + 5ms).
    calc.fillNewTimestamps({0, 10'000'000, 20'000'000, 30'000'000, 40'000'000});
    calc.fillNewTimestamps({55'000'000, 70'000'000, 85'000'000, 100'000'000, 115'000'000});
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.instantDrift, -5'000'000);
}

TEST_F(DriftCalculatorTest, EndLatencyComputedWhenStopTimeIsSet)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    calc.setStopTimeAcq(1000);
    calc.fillNewTimestamps({1800});
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.endLatency, 800);
}

TEST_F(DriftCalculatorTest, EndLatencyStaysZeroWhenStopTimeNotSet)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    calc.fillNewTimestamps({1800});
    calc.run();

    drift_st drift{};
    calc.readDrifts(drift);
    EXPECT_EQ(drift.endLatency, 0);
}

TEST_F(DriftCalculatorTest, EmptyFillNewTimestampsIsANoOp)
{
    DriftCalculator calc(sampleFrequency, windowSize);
    calc.setFirstTimeStamp(0);
    calc.fillNewTimestamps({10'000'000, 20'000'000, 30'000'000});
    calc.run();

    drift_st before{};
    calc.readDrifts(before);

    calc.fillNewTimestamps({});
    calc.run();

    drift_st after{};
    calc.readDrifts(after);

    EXPECT_EQ(before.startLatency, after.startLatency);
    EXPECT_EQ(before.globalDrift, after.globalDrift);
    EXPECT_EQ(before.instantDrift, after.instantDrift);
    EXPECT_EQ(before.endLatency, after.endLatency);
}
