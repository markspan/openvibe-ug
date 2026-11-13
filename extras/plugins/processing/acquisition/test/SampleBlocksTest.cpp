/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, see UG.md 2.30
 */
#include "SampleBlocks.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

constexpr size_t CHANNELS = 2;

// A block of the server's, as it sends it: sample-major, channel 0 counting the samples from 0, channel 1 minus that, stamped 10
// apart from 1000
struct Block
{
    std::vector<sampleType> values;
    std::vector<uint64_t> stamps;
    std::vector<ovStimulation> stims;

    Block(const size_t first, const size_t count, std::vector<ovStimulation> stimulations = {}) : stims(std::move(stimulations))
    {
        for (size_t s = first; s < first + count; ++s) {
            values.push_back(sampleType(s));
            values.push_back(-sampleType(s));
            stamps.push_back(1000 + 10 * s);
        }
    }

    bool addTo(SampleBlocks& blocks) const
    {
        return blocks.add(values.data(), values.size(), stamps.data(), stamps.size(), stims.data(), stims.size());
    }
};

std::vector<SampleBlocks::Chunk> takeAll(SampleBlocks& blocks)
{
    std::vector<SampleBlocks::Chunk> chunks;
    while (blocks.hasChunk()) { chunks.push_back(blocks.takeChunk()); }
    return chunks;
}

}  // namespace

TEST(SampleBlocks, blocksLargerThanAChunkKeepEverySample)
{
    // The case that went wrong: a stream of chunks of 4, and the server restarted with blocks of 64
    SampleBlocks blocks;
    blocks.configure(CHANNELS, 4);
    ASSERT_TRUE(Block(0, 64).addTo(blocks));
    ASSERT_TRUE(Block(64, 64).addTo(blocks));

    const auto chunks = takeAll(blocks);
    ASSERT_EQ(chunks.size(), 32U) << "128 samples in chunks of 4";
    for (size_t c = 0; c < chunks.size(); ++c) {
        ASSERT_EQ(chunks[c].samples.size(), CHANNELS * 4);
        for (size_t i = 0; i < 4; ++i) {
            const size_t sample = c * 4 + i;
            EXPECT_EQ(chunks[c].samples[i], double(sample)) << "channel 0, channel-major";
            EXPECT_EQ(chunks[c].samples[4 + i], -double(sample)) << "channel 1";
            EXPECT_EQ(chunks[c].stamps[i], 1000 + 10 * sample);
        }
    }
    EXPECT_EQ(blocks.pendingSamples(), 0U);
}

TEST(SampleBlocks, blocksSmallerThanAChunkWaitForIt)
{
    SampleBlocks blocks;
    blocks.configure(CHANNELS, 32);
    for (size_t b = 0; b < 7; ++b) { ASSERT_TRUE(Block(b * 4, 4).addTo(blocks)); }
    EXPECT_FALSE(blocks.hasChunk()) << "28 samples do not fill a chunk of 32";
    EXPECT_EQ(blocks.pendingSamples(), 28U);

    ASSERT_TRUE(Block(28, 8).addTo(blocks));
    const auto chunks = takeAll(blocks);
    ASSERT_EQ(chunks.size(), 1U);
    EXPECT_EQ(chunks[0].samples[31], 31.0);
    EXPECT_EQ(blocks.pendingSamples(), 4U) << "the rest waits for the next chunk";
}

TEST(SampleBlocks, aStimulationGoesWithTheChunkThatHoldsItsBlocksLastSample)
{
    // Never with an earlier chunk, which StimulationDelay would place it in, before its own sample
    SampleBlocks blocks;
    blocks.configure(CHANNELS, 4);
    const ovStimulation stim{33, 1000 + 10 * 9, 0};
    ASSERT_TRUE(Block(0, 10, {stim}).addTo(blocks));

    auto chunks = takeAll(blocks);
    ASSERT_EQ(chunks.size(), 2U);
    EXPECT_TRUE(chunks[0].stims.empty());
    EXPECT_TRUE(chunks[1].stims.empty()) << "its sample, the 10th, is not in a chunk yet";

    ASSERT_TRUE(Block(10, 2).addTo(blocks));
    chunks = takeAll(blocks);
    ASSERT_EQ(chunks.size(), 1U);
    ASSERT_EQ(chunks[0].stims.size(), 1U);
    EXPECT_EQ(chunks[0].stims[0].id, 33U);
}

TEST(SampleBlocks, aBlockOfOtherChannelsIsRefusedNotMisread)
{
    SampleBlocks blocks;
    blocks.configure(3, 4);
    EXPECT_FALSE(Block(0, 4).addTo(blocks)) << "4 samples of 2 channels are not 4 samples of 3";
    EXPECT_FALSE(blocks.hasChunk());
    EXPECT_EQ(blocks.pendingSamples(), 0U);

    SampleBlocks unconfigured;
    EXPECT_FALSE(Block(0, 4).addTo(unconfigured)) << "no settings yet: nothing to place them by";
}
