// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.27. Tests what the BrainVision Format file writer (UG) puts in its files.

#include "BrainVisionUG.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <string>

using namespace OpenViBE::Plugins::FileIO::BrainVisionUG;

namespace {
/// OpenViBE's time of a sample, as CTime(sampling, sample) computes it: rounded down to the fixed point
uint64_t sampleTime(const uint64_t sampling, const uint64_t sample) { return (sample << 32) / sampling; }
}  // namespace

TEST(BrainVisionUG, theFirstSampleWrittenIsPositionOne)
{
    const uint64_t start = sampleTime(2048, 1000);  // the signal started late, as the Acquisition client's does
    EXPECT_EQ(markerPosition(start, start, 2048), 1U);
    EXPECT_EQ(markerPosition(start + sampleTime(2048, 37), start, 2048), 38U);
}

TEST(BrainVisionUG, aSignalThatStartsLateDoesNotShiftTheMarkers)
{
    // The client's signal starts when the server's configuration comes in, here 0.3 s into the scenario
    const uint64_t start = uint64_t(0.3 * 4294967296.0);
    EXPECT_EQ(markerPosition(start + sampleTime(500, 10), start, 500), 11U);
}

TEST(BrainVisionUG, chunkStartsRoundedDownStillGiveTheSample)
{
    // At 500 Hz a chunk of 32 samples lasts 274877906.944 units; the client adds the rounded-down duration chunk by
    // chunk, so after many chunks a sample's date is well short of its exact time
    const uint64_t sampling = 500, chunk = 32;
    const uint64_t chunkDuration = sampleTime(sampling, chunk);
    uint64_t chunkStart = 0;
    for (uint64_t c = 0; c < 10000; ++c) { chunkStart += chunkDuration; }
    const uint64_t date = chunkStart + sampleTime(sampling, 5);
    EXPECT_EQ(markerPosition(date, 0, sampling), 10000 * chunk + 5 + 1);
}

TEST(BrainVisionUG, aMarkerBeforeTheFirstSampleHasNoPosition)
{
    const uint64_t start = sampleTime(1000, 500);
    EXPECT_FALSE(markerPosition(start - sampleTime(1000, 3), start, 1000).has_value());
    // Within half a sample it is the first sample
    EXPECT_EQ(markerPosition(start - 1000, start, 1000), 1U);
    EXPECT_FALSE(markerPosition(start, start, 0).has_value());
}

TEST(BrainVisionUG, commasInNamesAreCoded)
{
    EXPECT_EQ(escapeName("Fp1"), "Fp1");
    EXPECT_EQ(escapeName("EOG,left"), "EOG\\1left");
}

TEST(BrainVisionUG, integersAreRoundedAndHeldAtTheirLimits)
{
    bool clipped = false;
    EXPECT_EQ(toInteger<int16_t>(12.6, clipped), 13);
    EXPECT_EQ(toInteger<int16_t>(-12.6, clipped), -13);
    EXPECT_FALSE(clipped);

    EXPECT_EQ(toInteger<int16_t>(40000.0, clipped), 32767);
    EXPECT_TRUE(clipped);
    clipped = false;
    EXPECT_EQ(toInteger<int16_t>(-40000.0, clipped), -32768);
    EXPECT_TRUE(clipped);
    clipped = false;
    EXPECT_EQ(toInteger<uint16_t>(-1.0, clipped), 0);
    EXPECT_TRUE(clipped);
    clipped = false;
    EXPECT_EQ(toInteger<int16_t>(std::nan(""), clipped), 0);
    EXPECT_TRUE(clipped);
}

// UG fork: the impedances under [Comment], as BrainVision Recorder writes them and MNE reads them (raw.impedances)
TEST(BrainVisionUG, impedancesAsTheRecorderWritesThem)
{
    const std::string block = impedanceBlock("2026-11-22 10:42:17", {{"Fp1", 4.4}, {"Cz", 12.6}, {"EOG", std::nan("")}});
    EXPECT_EQ(block, "Impedance [kOhm] at 10:42:17 :\n"
                     "Fp1:          4\n"
                     "Cz:           13\n"
                     "\n"
                     "Impedances measured by the acquisition server on 2026-11-22 at 10:42:17, as the recording started\n");
}

TEST(BrainVisionUG, namesMNECanNotReadGoLast)
{
    // MNE stops at the first line whose name has another character than letters, digits, spaces and "_+-"
    const std::string block = impedanceBlock("2026-11-22 10:42:17", {{"A1.ref", 5.0}, {"Fp1", 4.0}});
    EXPECT_LT(block.find("Fp1:"), block.find("A1.ref:"));
    EXPECT_TRUE(impedanceNameReadable("Fp1"));
    EXPECT_TRUE(impedanceNameReadable("BIP-1"));
    EXPECT_FALSE(impedanceNameReadable("A1.ref"));
}

TEST(BrainVisionUG, noImpedancesNoBlock)
{
    EXPECT_TRUE(impedanceBlock("2026-11-22 10:42:17", {}).empty());
    EXPECT_TRUE(impedanceBlock("2026-11-22 10:42:17", {{"Fp1", std::nan("")}}).empty());
}
