/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <cstdint>
#include <stdexcept>

#include <gtest/gtest.h>

#include "SampleBufferHandler.h"

namespace {

/// Chunks buffered are those needed to cover one 20 ms eye-refresh period, rounded up.
/// maxSampleCount, which createBuffer() publishes, is sampleChunk * that count.
uint64_t expectedMaxSampleCount(const unsigned int sampleRate, const unsigned int sampleChunk)
{
    const uint64_t periodeChunkNs = (UINT64_C(1000000000) / sampleRate) * sampleChunk;
    constexpr uint64_t eyesRefreshNs = UINT64_C(1000000000) / 50;

    uint64_t chunkCount = eyesRefreshNs / periodeChunkNs;
    if (eyesRefreshNs % periodeChunkNs) { chunkCount++; }

    return chunkCount * sampleChunk;
}

/// Builds a buffer and hands back its capacity, then releases it.
uint64_t maxSampleCountOf(const unsigned int channelCount, const unsigned int sampleRate, const unsigned int sampleChunk)
{
    SampleBufferHandler handler(channelCount, sampleRate, sampleChunk);

    samplesBuffer* buff = handler.createBuffer();
    EXPECT_NE(buff, nullptr);
    EXPECT_NE(buff->samplesBuff, nullptr);
    EXPECT_NE(buff->timestampsBuff, nullptr);
    EXPECT_NE(buff->stimulationsBuff, nullptr);

    const uint64_t maxSampleCount = buff->maxSampleCount;
    handler.removeBuffer(buff);

    return maxSampleCount;
}

} // namespace

///
/// The regression this file exists for.
///
/// determineSizes() used to replace the chunk count with "periodeChunk_ns * 3" -- a duration in
/// nanoseconds -- whenever three or more chunks fitted in the refresh period. At the BioSemi
/// ActiveThree rate that asked for 5859360 chunks and a 24 GB sample buffer, the allocation
/// returned null and the page-warming loop wrote to it.
///
TEST(SampleBufferHandlerTest, HighSamplingRateKeepsBufferSmall)
{
    EXPECT_EQ(maxSampleCountOf(32, 16384, 32), 352u);
    EXPECT_EQ(maxSampleCountOf(32, 16384, 32), expectedMaxSampleCount(16384, 32));
}

/// Every chunk size the UI offers has to survive the BioSemi rate; 4..64 all took the bad branch.
TEST(SampleBufferHandlerTest, EveryChunkSizeSurvivesTheHighestSamplingRate)
{
    for (const unsigned int chunk : { 4u, 8u, 16u, 32u, 64u, 128u, 256u, 512u }) {
        EXPECT_EQ(maxSampleCountOf(32, 16384, chunk), expectedMaxSampleCount(16384, chunk)) << "chunk size " << chunk;
    }
}

/// A channel count whose chunk is not a multiple of the 4096-byte alignment: 40 x 4 x 32 = 5120 B.
/// C11 aligned_alloc, used on the Linux build, rejects a size that is not a multiple of it.
TEST(SampleBufferHandlerTest, ChunkSizeNeedNotBeAMultipleOfTheAlignment)
{
    EXPECT_EQ(maxSampleCountOf(40, 16384, 32), 352u);
}

/// Rates that already worked must keep exactly the buffer geometry they had.
TEST(SampleBufferHandlerTest, LowSamplingRatesAreUnchanged)
{
    EXPECT_EQ(maxSampleCountOf(32, 256, 32), 32u);
    EXPECT_EQ(maxSampleCountOf(32, 512, 32), 32u);
    EXPECT_EQ(maxSampleCountOf(4, 128, 16), 16u);
}

/// The bad branch was reachable at sampleRate >= 150 * sampleChunk, so a mid-range rate with a
/// small chunk hit it too -- this is not specific to BioSemi.
TEST(SampleBufferHandlerTest, ModerateRateWithSmallChunkIsSane)
{
    EXPECT_EQ(maxSampleCountOf(32, 1000, 4), 20u);
    EXPECT_EQ(maxSampleCountOf(32, 1000, 8), 24u);
}

/// determineSizes() divides by both, and the LabStreamingLayer driver takes its rate from the
/// remote stream, so zero is reachable rather than merely theoretical.
TEST(SampleBufferHandlerTest, ZeroRateOrChunkIsRejected)
{
    EXPECT_THROW(SampleBufferHandler(32, 0, 32), std::invalid_argument);
    EXPECT_THROW(SampleBufferHandler(32, 16384, 0), std::invalid_argument);
}

/// Buffers are handed out from a pool of ten, so createBuffer() is called repeatedly on one
/// handler and every one of them has to be independently usable.
TEST(SampleBufferHandlerTest, RepeatedCreateAndRemoveRoundTrips)
{
    SampleBufferHandler handler(32, 16384, 32);

    samplesBuffer* buffers[10] = {};
    for (auto& buff : buffers) {
        buff = handler.createBuffer();
        ASSERT_NE(buff, nullptr);
        ASSERT_NE(buff->samplesBuff, nullptr);
        EXPECT_EQ(buff->count, 0u);
        EXPECT_EQ(buff->stimCount, 0u);
        EXPECT_EQ(buff->maxSampleCount, 352u);
    }

    // Writing the full advertised capacity must stay inside the allocation.
    for (auto& buff : buffers) {
        for (uint64_t i = 0; i < buff->maxSampleCount * 32; ++i) { buff->samplesBuff[i] = float(i); }
        for (uint64_t i = 0; i < buff->maxSampleCount; ++i) { buff->timestampsBuff[i] = i; }
    }

    for (auto& buff : buffers) { handler.removeBuffer(buff); }
}
