/**
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "DriftCorrector.h"

namespace {

constexpr uint32_t samplingRate = 100;          // 100 Hz
constexpr int64_t periodNs      = 10'000'000;   // 1e9 / samplingRate
constexpr uint32_t channels     = 2;
constexpr uint64_t capacity     = 64;

/// A buffer of @p count samples stamped at the declared period, the last one at @p lastStamp. Sample i holds i.
struct TestBuffer
{
    std::vector<timestampType> stamps   = std::vector<timestampType>(capacity);
    std::vector<sampleType> samples     = std::vector<sampleType>(capacity * channels);
    std::vector<ovStimulation> stims    = std::vector<ovStimulation>(8);
    samplesBuffer buff{};

    TestBuffer(const uint64_t count, const uint64_t lastStamp)
    {
        buff = {stamps.data(), samples.data(), stims.data(), count, 0, uint32_t(stims.size()), capacity};
        for (uint64_t i = 0; i < count; ++i) {
            stamps[i] = lastStamp - (count - 1 - i) * periodNs;
            for (uint32_t c = 0; c < channels; ++c) { samples[i * channels + c] = sampleType(i); }
        }
    }
};

}  // namespace

TEST(DriftCorrectorTest, LeavesOnScheduleSamplesAlone)
{
    DriftCorrector corrector(samplingRate, channels, 5, 4);
    uint64_t last = 1'000'000'000;
    for (int i = 0; i < 20; ++i) {
        TestBuffer b(10, last);
        EXPECT_EQ(corrector.process(&b.buff), 0);
        EXPECT_EQ(b.buff.count, 10u);
        last += 10 * periodNs;
    }
    EXPECT_EQ(corrector.correctionCount(), 0u);
}

TEST(DriftCorrectorTest, RepeatsTheLastSampleWhenSamplesAreMissing)
{
    DriftCorrector corrector(samplingRate, channels, 5, 4);
    uint64_t last = 1'000'000'000;
    TestBuffer first(10, last);
    corrector.process(&first.buff);

    // From now on every buffer comes 30 ms later than its samples account for: 3 samples are missing
    last += 10 * periodNs + 3 * periodNs;
    int64_t total = 0;
    for (int i = 0; i < 4; ++i) {
        TestBuffer b(10, last);
        const int64_t correction = corrector.process(&b.buff);
        total += correction;
        if (correction > 0) {
            ASSERT_EQ(b.buff.count, 10u + uint64_t(correction));
            for (uint64_t s = 10; s < b.buff.count; ++s) {
                EXPECT_EQ(b.stamps[s], b.stamps[9]);
                EXPECT_EQ(b.samples[s * channels], b.samples[9 * channels]);
            }
        }
        last += 10 * periodNs;
    }
    EXPECT_EQ(total, 3);
    EXPECT_EQ(corrector.addedCount(), 3u);
}

TEST(DriftCorrectorTest, DropsSamplesAndKeepsTheirStimulationsWhenThereAreTooMany)
{
    DriftCorrector corrector(samplingRate, channels, 5, 4);
    uint64_t last = 1'000'000'000;
    TestBuffer first(10, last);
    corrector.process(&first.buff);

    // Every buffer now carries 13 samples for 10 sample periods of time
    int64_t total = 0;
    for (int i = 0; i < 4; ++i) {
        last += 10 * periodNs;
        TestBuffer b(13, last);
        b.stims[0]       = ovStimulation{0x8105, b.stamps[12], 0};
        b.buff.stimCount = 1;
        const int64_t correction = corrector.process(&b.buff);
        total += correction;
        if (correction < 0) {
            EXPECT_EQ(b.buff.count, uint64_t(13 + correction));
            EXPECT_EQ(b.stims[0].date, b.stamps[b.buff.count - 1]);
        }
    }
    EXPECT_LT(total, 0);
    EXPECT_EQ(corrector.removedCount(), uint64_t(-total));
}

TEST(DriftCorrectorTest, ToleratesDriftWithinTheTolerance)
{
    DriftCorrector corrector(samplingRate, channels, 50, 4);
    uint64_t last = 1'000'000'000;
    TestBuffer first(10, last);
    corrector.process(&first.buff);

    last += 10 * periodNs + 3 * periodNs;  // 30 ms late, within 50 ms
    for (int i = 0; i < 10; ++i) {
        TestBuffer b(10, last);
        EXPECT_EQ(corrector.process(&b.buff), 0);
        last += 10 * periodNs;
    }
}
