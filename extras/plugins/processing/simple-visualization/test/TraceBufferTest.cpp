// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.26. Tests the Signal display's ring of samples.

#include "CTraceBuffer.h"

#include <gtest/gtest.h>

#include <cmath>
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

// UG fork: the time-locked ring of the LSL display

TEST(TraceBuffer, ringsOfDifferentRatesDrawATimeAtTheSameX)
{
    // Two streams of 500 and 256 Hz in a 2 s window, starting at different times on a shared clock
    CTraceBuffer fast, slow;
    for (CTraceBuffer* ring : { &fast, &slow }) {
        ring->setWindowSeconds(2.0);
        ring->setTimeOrigin(100.0);
    }
    fast.reconfigure(1, 500.0);
    slow.reconfigure(1, 256.0);

    // Both written up to t = 101.0
    const std::vector<double> fastSamples = ramp(0, 350), slowSamples = ramp(0, 64);
    fast.appendTimed(fastSamples.data(), 1, 350, 100.3);
    slow.appendTimed(slowSamples.data(), 1, 64, 100.75);

    CTraceBuffer::CReader f(fast);
    const double fastX = double(f.writePos()) / double(f.capacity());
    CTraceBuffer::CReader s(slow);
    const double slowX = double(s.writePos()) / double(s.capacity());
    EXPECT_DOUBLE_EQ(fastX, 0.5);  // 1 s into a 2 s window
    EXPECT_DOUBLE_EQ(slowX, 0.5);
}

TEST(TraceBuffer, aGapInTimeIsBridgedWithTheLastValue)
{
    CTraceBuffer ring;
    ring.setWindowSeconds(1.0);
    ring.setTimeOrigin(0.0);
    ring.reconfigure(1, 100.0);

    const std::vector<double> first = ramp(0, 10), second = ramp(100, 10);
    ring.appendTimed(first.data(), 1, 10, 0.0);
    ring.appendTimed(second.data(), 1, 10, 0.2);  // 10 samples later than counted

    CTraceBuffer::CReader r(ring);
    EXPECT_EQ(r.totalSamples(), 30U);
    EXPECT_FLOAT_EQ(r.channel(0)[15], 9.0F);   // the gap holds the last value
    EXPECT_FLOAT_EQ(r.channel(0)[20], 100.0F); // the second chunk at the slot of its time
}

TEST(TraceBuffer, samplesThatComeEarlyOverwriteTheLastOnes)
{
    CTraceBuffer ring;
    ring.setWindowSeconds(1.0);
    ring.setTimeOrigin(0.0);
    ring.reconfigure(1, 100.0);

    const std::vector<double> first = ramp(0, 10), second = ramp(100, 10);
    ring.appendTimed(first.data(), 1, 10, 0.0);
    ring.appendTimed(second.data(), 1, 10, 0.07);  // 3 samples earlier than counted

    CTraceBuffer::CReader r(ring);
    EXPECT_EQ(r.totalSamples(), 17U);
    EXPECT_FLOAT_EQ(r.channel(0)[7], 100.0F);
    EXPECT_EQ(r.writePos(), 17U);
}

TEST(TraceBuffer, aSampleOfJitterIsLeftAlone)
{
    CTraceBuffer ring;
    ring.setWindowSeconds(1.0);
    ring.setTimeOrigin(0.0);
    ring.reconfigure(1, 100.0);

    const std::vector<double> chunk = ramp(0, 10);
    ring.appendTimed(chunk.data(), 1, 10, 0.0);
    ring.appendTimed(chunk.data(), 1, 10, 0.111);  // one sample late: under RESYNC_SAMPLES

    EXPECT_EQ(ring.totalSamples(), 20U);
}

namespace {
/**
 * Sends a simulated LSL stream into a ring for \p seconds: samples at \p actualRate, in chunks of \p chunk samples, each
 * chunk dated by its first sample, as LSL dates it, plus \p firstError on the first chunk only. Each sample's value is its
 * true time, so that a slot shows where the sample of that time landed.
 */
void simulate(CTraceBuffer& ring, const double start, const double actualRate, const size_t chunk, const double seconds, const double firstError)
{
    std::vector<double> values(chunk);
    const size_t total = size_t(seconds * actualRate);
    for (size_t s = 0; s + chunk <= total; s += chunk)
    {
        for (size_t i = 0; i < chunk; ++i) { values[i] = start + double(s + i) / actualRate; }
        ring.appendTimed(values.data(), 1, chunk, values[0] + (s == 0 ? firstError : 0.0));
    }
}

/// The true time of the sample in the slot where \p ring draws time \p t, as xAtTime() maps a time to a slot.
double sampleTimeAt(CTraceBuffer& ring, const double t)
{
    CTraceBuffer::CReader r(ring);
    const double index = (t - r.firstSampleSeconds()) * r.sampling();
    const size_t slot  = size_t(std::llround(std::fmod(index + double(r.slotOffset()), double(r.capacity()))));
    return r.channel(0)[slot % r.capacity()];
}
}  // namespace

TEST(TraceBuffer, streamsOfDifferentRatesAndClocksStayLinedUpForAnHour)
{
    // Two streams on one clock: one at 500 Hz whose amplifier runs 500 ppm fast, in chunks of 20, its first date 15 ms
    // off, as LSL's first dates are before the clock correction settles; one at 128 Hz running 0.1 % slow, in chunks of 4
    CTraceBuffer fast, slow;
    for (CTraceBuffer* ring : { &fast, &slow }) {
        ring->setWindowSeconds(10.0);
        ring->setTimeOrigin(1000.0);
    }
    fast.reconfigure(1, 500.0);
    slow.reconfigure(1, 128.0);
    simulate(fast, 1000.3, 500.25, 20, 3600.0, 0.015);
    simulate(slow, 1000.7, 127.872, 4, 3600.0, 0.0);

    // An hour on, a time a few seconds back is drawn on the sample of that time in both, within RESYNC_SAMPLES and the
    // half sample of the grid
    for (const double t : { 4595.0, 4597.25, 4598.5 })
    {
        EXPECT_NEAR(sampleTimeAt(fast, t), t, (CTraceBuffer::RESYNC_SAMPLES + 0.5) / 500.0) << "at " << t;
        EXPECT_NEAR(sampleTimeAt(slow, t), t, (CTraceBuffer::RESYNC_SAMPLES + 0.5) / 128.0) << "at " << t;
    }
}
