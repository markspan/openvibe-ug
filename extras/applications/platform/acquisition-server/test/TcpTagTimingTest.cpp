// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see speller.md

#include "../builtin/drivers/tagging/TagDate.h"
#include "AbstractWriter.h"
#include "samplesBuffer.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

constexpr uint64_t ONE_MS_FIXED = (1ULL << 32) / 1000;
constexpr uint64_t ONE_S_NS     = 1000000000ULL;

// What went out, chunk by chunk
class RecordingWriter final : public AbstractWriter
{
public:
    RecordingWriter() : AbstractWriter("test") { }

    struct Chunk
    {
        std::vector<timestampType> timestamps;
        std::vector<uint64_t> stimulations;
    };

    std::vector<Chunk> chunks;

protected:
    size_t send(ovMsg* msg, uint64_t /*clientId*/) override
    {
        const auto& samples = msg->msgData.samplesStruct;
        Chunk chunk;
        chunk.timestamps.assign(samples.timestamp, samples.timestamp + samples.count);
        for (uint32_t i = 0; i < samples.stimCount; ++i) { chunk.stimulations.push_back(samples.stimulations[i].id); }
        chunks.push_back(chunk);
        return samples.count;
    }
};

// A buffer of samples with these timestamps, and stimulations with these dates, whose ids are their dates
class Buffer
{
public:
    Buffer(const std::vector<timestampType>& timestamps, const std::vector<uint64_t>& dates)
        : m_timestamps(timestamps), m_samples(timestamps.size(), 0)
    {
        for (const uint64_t date : dates) { m_stimulations.push_back(ovStimulation{date, date, 0}); }
        m_buffer.count           = timestamps.size();
        m_buffer.stimCount       = int(dates.size());
        m_buffer.timestampsBuff  = m_timestamps.data();
        m_buffer.samplesBuff     = m_samples.data();
        m_buffer.stimulationsBuff = m_stimulations.data();
    }

    samplesBuffer* get() { return &m_buffer; }

private:
    std::vector<timestampType> m_timestamps;
    std::vector<sampleType> m_samples;
    std::vector<ovStimulation> m_stimulations;
    samplesBuffer m_buffer {};
};

}  // namespace

TEST(TcpTagDateTest, SubtractsTheAgeOfTheTag)
{
    const uint64_t now   = 1000ULL << 32;
    const uint64_t nowNs = 7 * ONE_S_NS;

    // 10 ms old, to within the rounding of fixed point time
    const uint64_t date = tagDate(now - 10 * ONE_MS_FIXED, now, nowNs);
    EXPECT_NEAR(double(nowNs - date), 10e6, 1e3);

    EXPECT_EQ(tagDate(now, now, nowNs), nowNs);
}

TEST(TcpTagDateTest, ATimeThatMeansNothingHereGivesThePresent)
{
    const uint64_t now   = 1000ULL << 32;
    const uint64_t nowNs = 7 * ONE_S_NS;

    EXPECT_EQ(tagDate(now + ONE_MS_FIXED, now, nowNs), nowNs);       // From the future: another PC's clock
    EXPECT_EQ(tagDate(now - (3ULL << 32), now, nowNs), nowNs);       // Older than a tag can be
    EXPECT_EQ(tagDate(now - (1ULL << 32), now, ONE_S_NS / 2), ONE_S_NS / 2);    // Before the driver's clock began
}

TEST(TcpTagWriterTest, AStimulationWaitsForItsSample)
{
    RecordingWriter writer;
    ASSERT_TRUE(writer.init(4, 1));

    // The tag is there before the samples of its moment
    Buffer tag({}, {160});
    writer.writeSamples(tag.get());

    Buffer before({100, 110, 120, 130}, {});
    writer.writeSamples(before.get());
    ASSERT_EQ(writer.chunks.size(), 1u);
    EXPECT_TRUE(writer.chunks[0].stimulations.empty());

    Buffer holding({140, 150, 160, 170}, {});
    writer.writeSamples(holding.get());
    ASSERT_EQ(writer.chunks.size(), 2u);
    ASSERT_EQ(writer.chunks[1].stimulations.size(), 1u);
    EXPECT_EQ(writer.chunks[1].stimulations[0], 160u);
}

TEST(TcpTagWriterTest, OnlyTheStimulationsThatAreDueGoOut)
{
    RecordingWriter writer;
    ASSERT_TRUE(writer.init(2, 1));

    Buffer tags({}, {105, 115, 125});
    writer.writeSamples(tags.get());

    // Sample by sample, which takes the other way through the writer: a chunk that is copied
    Buffer first({100}, {});
    Buffer second({110}, {});
    writer.writeSamples(first.get());
    writer.writeSamples(second.get());
    ASSERT_EQ(writer.chunks.size(), 1u);
    EXPECT_EQ(writer.chunks[0].stimulations, std::vector<uint64_t>({105}));

    Buffer third({120, 130}, {});
    writer.writeSamples(third.get());
    ASSERT_EQ(writer.chunks.size(), 2u);
    EXPECT_EQ(writer.chunks[1].stimulations, std::vector<uint64_t>({115, 125}));
}

TEST(TcpTagWriterTest, ALateStimulationGoesWithTheNextChunk)
{
    RecordingWriter writer;
    ASSERT_TRUE(writer.init(2, 1));

    Buffer tag({}, {50});
    writer.writeSamples(tag.get());
    Buffer samples({100, 110}, {});
    writer.writeSamples(samples.get());
    ASSERT_EQ(writer.chunks.size(), 1u);
    EXPECT_EQ(writer.chunks[0].stimulations, std::vector<uint64_t>({50}));
}

TEST(TcpTagWriterTest, AStimulationFromAnotherClockIsNotHeldBack)
{
    RecordingWriter writer;
    ASSERT_TRUE(writer.init(2, 1));

    // Its samples would never come
    const uint64_t farAway = 100 + 5 * ONE_S_NS;
    Buffer tag({}, {farAway});
    writer.writeSamples(tag.get());
    Buffer samples({100, 110}, {});
    writer.writeSamples(samples.get());
    ASSERT_EQ(writer.chunks.size(), 1u);
    EXPECT_EQ(writer.chunks[0].stimulations, std::vector<uint64_t>({farAway}));
}

TEST(TcpTagWriterTest, ATriggerStaysWithItsOwnSample)
{
    RecordingWriter writer;
    ASSERT_TRUE(writer.init(2, 1));

    Buffer samples({100, 110, 120, 130}, {110, 120});
    writer.writeSamples(samples.get());
    ASSERT_EQ(writer.chunks.size(), 2u);
    EXPECT_EQ(writer.chunks[0].stimulations, std::vector<uint64_t>({110}));
    EXPECT_EQ(writer.chunks[1].stimulations, std::vector<uint64_t>({120}));
}
