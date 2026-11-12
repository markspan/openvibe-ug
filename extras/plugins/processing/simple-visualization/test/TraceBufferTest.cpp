// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.26. Tests the Signal display's ring of samples.

#include "CTraceBuffer.h"

#include <gtest/gtest.h>

#include <vector>

using namespace OpenViBE::Plugins::SimpleVisualization;

namespace {
/** OpenViBE time of a number of seconds. */
uint64_t ovTime(const double seconds) { return uint64_t(seconds * 4294967296.0); }

/** One channel, sample values 0, 1, 2, ... from first on. */
std::vector<double> ramp(const size_t first, const size_t count)
{
    std::vector<double> values(count);
    for (size_t i = 0; i < count; ++i) { values[i] = double(first + i); }
    return values;
}
}  // namespace

TEST(TraceBuffer, eachSampleGoesToTheSlotOfItsNumber)
{
    CTraceBuffer buffer;
    buffer.setWindowSeconds(1.0);
    buffer.reconfigure(1, 10.0);  // 10 slots

    const std::vector<double> samples = ramp(0, 14);
    buffer.appendChunk(samples.data(), 1, 14, ovTime(0.0), ovTime(1.4));

    CTraceBuffer::CReader r(buffer);
    EXPECT_EQ(r.totalSamples(), 14U);
    EXPECT_EQ(r.writePos(), 4U);
    for (size_t g = 4; g < 14; ++g) { EXPECT_FLOAT_EQ(r.channel(0)[g % 10], float(g)) << "sample " << g; }
}

TEST(TraceBuffer, aChunkLongerThanTheRingStillCountsAllItsSamples)
{
    CTraceBuffer buffer;
    buffer.setWindowSeconds(1.0);
    buffer.reconfigure(1, 10.0);

    const std::vector<double> first = ramp(0, 3);
    buffer.appendChunk(first.data(), 1, 3, ovTime(0.0), ovTime(0.3));
    const std::vector<double> longChunk = ramp(3, 25);
    buffer.appendChunk(longChunk.data(), 1, 25, ovTime(0.3), ovTime(2.8));

    // Sample 27 was the last: the time of the next is 2.8 s, and every kept sample sits at the slot of its number
    CTraceBuffer::CReader r(buffer);
    EXPECT_EQ(r.totalSamples(), 28U);
    EXPECT_EQ(r.writePos(), 8U);
    EXPECT_DOUBLE_EQ(r.firstSampleSeconds() + double(r.totalSamples()) / r.sampling(), 2.8);
    for (size_t g = 18; g < 28; ++g) { EXPECT_FLOAT_EQ(r.channel(0)[g % 10], float(g)) << "sample " << g; }
}

TEST(TraceBuffer, aChunkOfTheWrongChannelCountIsLeftOut)
{
    CTraceBuffer buffer;
    buffer.setWindowSeconds(1.0);
    buffer.reconfigure(2, 10.0);

    const std::vector<double> samples = ramp(0, 4);
    buffer.appendChunk(samples.data(), 1, 4, ovTime(0.0), ovTime(0.4));

    EXPECT_EQ(buffer.totalSamples(), 0U);
}

TEST(TraceBuffer, theRateOfAMatrixStreamComesFromItsFirstChunk)
{
    CTraceBuffer buffer;
    buffer.setWindowSeconds(2.0);
    buffer.reconfigure(1, 0.0);  // a StreamedMatrix carries no rate

    const std::vector<double> samples = ramp(0, 8);
    buffer.appendChunk(samples.data(), 1, 8, ovTime(1.0), ovTime(1.5));

    CTraceBuffer::CReader r(buffer);
    EXPECT_DOUBLE_EQ(r.sampling(), 16.0);
    EXPECT_EQ(r.capacity(), 32U);
    EXPECT_DOUBLE_EQ(r.firstSampleSeconds(), 1.0);
}
