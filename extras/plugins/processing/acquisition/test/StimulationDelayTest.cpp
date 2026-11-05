/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, see speller.md, 3.3
 */
#include "StimulationDelay.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

// Chunks of 4 samples, stamped 100 apart in the server's clock from 1000, and 10 apart in the scenario's time
constexpr size_t CHUNK_SAMPLES = 4;

class StimulationDelayTest : public ::testing::Test
{
protected:
    void configure(const size_t delayChunks)
    {
        m_delay.configure(delayChunks, [](const size_t sample) { return uint64_t(sample) * 10; });
    }

    // The next chunk of signal, with these stimulations. Returns what the delay lets out, as the box would ask for it.
    std::vector<StimulationDelay::Chunk> add(const std::vector<ovStimulation>& stims = {})
    {
        std::vector<uint64_t> stamps;
        for (size_t i = 0; i < CHUNK_SAMPLES; ++i) { stamps.push_back(1000 + (m_chunks * CHUNK_SAMPLES + i) * 100); }
        m_delay.addChunk(m_chunks * CHUNK_SAMPLES * 10, std::move(stamps), stims);
        ++m_chunks;

        std::vector<StimulationDelay::Chunk> due;
        while (m_delay.hasDueChunk()) { due.push_back(m_delay.takeDueChunk()); }
        return due;
    }

    StimulationDelay m_delay;
    uint64_t m_chunks = 0;
};

TEST_F(StimulationDelayTest, nothingComesOutBeforeTheDelayHasPassed)
{
    configure(2);
    EXPECT_TRUE(add().empty());
    EXPECT_TRUE(add().empty());
    const auto due = add();
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].startTime, 0u);
}

TEST_F(StimulationDelayTest, aStimulationOnTimeGoesOutWithItsChunkLater)
{
    configure(1);
    EXPECT_TRUE(add({{7, 1250, 0}}).empty());  // Between samples 2 and 3 of chunk 0
    const auto due = add();
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].startTime, 0u);
    ASSERT_EQ(due[0].stims.size(), 1u);
    EXPECT_EQ(due[0].stims[0].id, 7u);
    EXPECT_EQ(due[0].stims[0].date, 20u);
}

// This is what the delay is for: the chunk has gone from the server, but its stimulation goes where it belongs.
TEST_F(StimulationDelayTest, aLateStimulationGetsItsOwnSample)
{
    configure(1);
    EXPECT_TRUE(add().empty());
    const auto due = add({{7, 1300, 0}});  // Sample 3 of chunk 0, coming with chunk 1
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].startTime, 0u);
    ASSERT_EQ(due[0].stims.size(), 1u);
    EXPECT_EQ(due[0].stims[0].date, 30u);

    EXPECT_TRUE(add().at(0).stims.empty());
}

TEST_F(StimulationDelayTest, aStimulationCanMissSeveralShortChunks)
{
    configure(3);
    add();
    add();
    add();
    const auto due = add({{7, 1100, 0}});  // Sample 1 of chunk 0, coming with chunk 3
    ASSERT_EQ(due.size(), 1u);
    ASSERT_EQ(due[0].stims.size(), 1u);
    EXPECT_EQ(due[0].stims[0].date, 10u);
}

TEST_F(StimulationDelayTest, aStimulationLaterThanTheDelayGoesToTheFirstSampleStillOpen)
{
    configure(1);
    add();
    add();  // Chunk 0 has gone
    const auto due = add({{7, 1100, 0}});  // Sample 1 of chunk 0
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].startTime, 40u);
    ASSERT_EQ(due[0].stims.size(), 1u);
    EXPECT_EQ(due[0].stims[0].date, 40u);
}

TEST_F(StimulationDelayTest, aStimulationStampedBeforeAllSamplesGoesToTheFirst)
{
    configure(1);
    add({{7, 5, 0}});
    const auto due = add();
    ASSERT_EQ(due.size(), 1u);
    ASSERT_EQ(due[0].stims.size(), 1u);
    EXPECT_EQ(due[0].stims[0].date, 0u);
}

TEST_F(StimulationDelayTest, stimulationsGoOutInDateOrder)
{
    configure(1);
    add({{1, 1300, 0}});
    const auto due = add({{2, 1100, 0}, {3, 1200, 0}});  // Two programs send tags: these came later, with earlier dates
    ASSERT_EQ(due.size(), 1u);
    ASSERT_EQ(due[0].stims.size(), 3u);
    EXPECT_EQ(due[0].stims[0].id, 2u);
    EXPECT_EQ(due[0].stims[1].id, 3u);
    EXPECT_EQ(due[0].stims[2].id, 1u);
}

// Drift correction repeats samples with the same stamp, and the stimulation was dated with the original.
TEST_F(StimulationDelayTest, ofSamplesSharingAStampTheFirst)
{
    configure(1);
    m_delay.addChunk(0, {1000, 1100, 1100, 1100}, {});
    m_delay.addChunk(40, {1100, 1200, 1300, 1400}, {{7, 1100, 0}});
    ASSERT_TRUE(m_delay.hasDueChunk());
    const StimulationDelay::Chunk due = m_delay.takeDueChunk();
    ASSERT_EQ(due.stims.size(), 1u);
    EXPECT_EQ(due.stims[0].date, 10u);
}

TEST_F(StimulationDelayTest, everyDateIsWithinItsChunk)
{
    configure(2);
    std::vector<StimulationDelay::Chunk> out;
    for (uint64_t chunk = 0; chunk < 50; ++chunk) {
        // Each chunk brings stimulations from 0 to 3 chunks back
        std::vector<ovStimulation> stims;
        for (uint64_t back = 0; back < 4; ++back) {
            if (chunk >= back) { stims.push_back({back, 1000 + (chunk - back) * CHUNK_SAMPLES * 100 + 150, 0}); }
        }
        for (auto& due : add(stims)) { out.push_back(std::move(due)); }
    }

    size_t count = 0;
    for (const auto& due : out) {
        for (const auto& stim : due.stims) {
            EXPECT_GE(stim.date, due.startTime);
            EXPECT_LT(stim.date, due.startTime + CHUNK_SAMPLES * 10);
            ++count;
        }
    }
    EXPECT_GT(count, 100u);
}

TEST(StimulationDelayChunks, coverTheWindow)
{
    EXPECT_EQ(StimulationDelay::chunksFor(32, 256), 1u);   // 125 ms
    EXPECT_EQ(StimulationDelay::chunksFor(8, 256), 4u);    // 31 ms
    EXPECT_EQ(StimulationDelay::chunksFor(32, 2048), 7u);  // 15.6 ms
    EXPECT_EQ(StimulationDelay::chunksFor(512, 256), 1u);  // 2 s
    EXPECT_EQ(StimulationDelay::chunksFor(0, 256), 1u);
    EXPECT_EQ(StimulationDelay::chunksFor(32, 0), 1u);
}

}  // namespace
