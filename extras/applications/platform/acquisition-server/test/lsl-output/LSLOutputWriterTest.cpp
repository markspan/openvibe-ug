/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file. The LSL Output writer as a client sees it, through real LSL streams on this PC. See UG.md, 2.21.
 */
#include "LSLOutputWriter.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr uint32_t SAMPLE_RATE  = 256;
constexpr uint32_t CHUNK        = 4;
constexpr uint32_t CHANNELS     = 2;
constexpr uint64_t PERIOD_NS    = 1000000000ULL / SAMPLE_RATE;
constexpr double   TIMEOUT      = 10.0;
constexpr double   SAME_TIME    = 1e-6;  // Seconds. Times that both come from the server's clock agree to this.
constexpr int      PRIME_MARKER = 999;

uint64_t serverClockNs()
{
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

// A real test stream, of a name nothing else on the network has
std::string uniqueName(const std::string& prefix)
{
    std::random_device random;
    return prefix + "_" + std::to_string(random());
}

ovChannelInfo withMontage()
{
    ovChannelInfo info;
    info.names     = {"Cz", "EOG"};
    info.montage   = "Refa-64.json";
    info.positions = {{true, 0.0, 0.0}, {}};
    return info;
}

class LSLOutputWriterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_signalName = uniqueName("OVTestSignal");
        m_markerName = uniqueName("OVTestMarkers");
        m_writer     = create();
    }

    std::unique_ptr<LSLOutputWriter> create(const ovChannelInfo& channels = {{"C3", "C4"}, "", {}}) const
    {
        auto writer = std::make_unique<LSLOutputWriter>("LSL Output");
        writer->getParameters()[0]->setValue(std::any(m_signalName));
        writer->getParameters()[1]->setValue(std::any(m_markerName));
        writer->init(CHUNK, CHANNELS);
        writer->onAcquisitionSetup(SAMPLE_RATE, channels);
        return writer;
    }

    static lsl::stream_info find(const std::string& name)
    {
        const auto found = lsl::resolve_stream("name", name, 1, TIMEOUT);
        if (found.empty()) { throw std::runtime_error("Stream not found: " + name); }
        return found[0];
    }

    // A chunk of samples, the last one stamped lastStamp, with these stimulations
    void write(const uint64_t lastStamp, std::vector<ovStimulation> stims = {}) const
    {
        std::vector<timestampType> stamps;
        std::vector<sampleType> samples;
        for (uint32_t i = 0; i < CHUNK; ++i) {
            stamps.push_back(lastStamp - (CHUNK - 1 - i) * PERIOD_NS);
            for (uint32_t c = 0; c < CHANNELS; ++c) { samples.push_back(sampleType(i * 10 + c)); }
        }
        samplesBuffer buffer{stamps.data(), samples.data(), stims.data(), CHUNK, uint32_t(stims.size()), uint32_t(stims.size()), CHUNK};
        m_writer->writeSamples(&buffer);
    }

    // The writer sends nothing until an outlet knows of its client, which comes some time after the client has opened the stream.
    // Sends chunks until one arrives, and leaves the client with nothing to read.
    void connect(lsl::stream_inlet& signal, lsl::stream_inlet& markers) const
    {
        signal.open_stream(TIMEOUT);
        markers.open_stream(TIMEOUT);
        std::vector<float> sample(CHANNELS);
        int marker = 0;
        bool gotSignal = false, gotMarker = false;
        for (int i = 0; i < 200 && !(gotSignal && gotMarker); ++i) {
            const uint64_t now = serverClockNs();
            write(now, {{uint64_t(PRIME_MARKER), now, 0}});
            gotSignal = gotSignal || signal.pull_sample(sample.data(), CHANNELS, 0.05) != 0.0;
            gotMarker = gotMarker || markers.pull_sample(&marker, 1, 0.05) != 0.0;
        }
        ASSERT_TRUE(gotSignal && gotMarker);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        signal.flush();
        markers.flush();
    }

    std::string m_signalName, m_markerName;
    std::unique_ptr<LSLOutputWriter> m_writer;
};

TEST_F(LSLOutputWriterTest, samplesKeepTheirTimesInLslsClock)
{
    lsl::stream_inlet signal(find(m_signalName)), markers(find(m_markerName));
    connect(signal, markers);

    const uint64_t lastStamp = serverClockNs() - 100000000ULL;  // Read from the amplifier 100 ms ago
    write(lastStamp);

    std::vector<float> sample(CHANNELS);
    std::vector<double> times;
    for (uint32_t i = 0; i < CHUNK; ++i) {
        const double time = signal.pull_sample(sample.data(), CHANNELS, TIMEOUT);
        ASSERT_NE(time, 0.0);
        EXPECT_FLOAT_EQ(sample[1], float(i * 10 + 1));
        times.push_back(time);
    }

    for (uint32_t i = 1; i < CHUNK; ++i) { EXPECT_NEAR(times[i] - times[i - 1], 1e-9 * double(PERIOD_NS), SAME_TIME); }

    // Against the real clock, so with some room for this thread to be interrupted
    const double expected = lsl::local_clock() - 1e-9 * double(serverClockNs() - lastStamp);
    EXPECT_NEAR(times.back(), expected, 0.005);
}

// This is what the export is for: on the client's timeline a marker is on its own sample, wherever in the chunk that is
TEST_F(LSLOutputWriterTest, aMarkerStaysOnItsOwnSample)
{
    lsl::stream_inlet signal(find(m_signalName)), markers(find(m_markerName));
    connect(signal, markers);

    const uint64_t lastStamp = serverClockNs();
    const uint64_t sample1   = lastStamp - 2 * PERIOD_NS;
    const uint64_t late      = lastStamp - 300000000ULL;  // Belongs to a chunk that went long ago
    write(lastStamp, {{7, late, 0}, {8, sample1, 0}});

    std::vector<float> sample(CHANNELS);
    std::vector<double> times;
    for (uint32_t i = 0; i < CHUNK; ++i) {
        times.push_back(signal.pull_sample(sample.data(), CHANNELS, TIMEOUT));
        ASSERT_NE(times.back(), 0.0);
    }

    int marker = 0;
    const double lateTime = markers.pull_sample(&marker, 1, TIMEOUT);
    EXPECT_EQ(marker, 7);
    const double ownTime = markers.pull_sample(&marker, 1, TIMEOUT);
    EXPECT_EQ(marker, 8);

    EXPECT_NEAR(ownTime, times[1], SAME_TIME);
    EXPECT_NEAR(times[3] - lateTime, 0.3, SAME_TIME);
}

TEST_F(LSLOutputWriterTest, otherMessagesSendNothing)
{
    const ovChannelInfo channels{{"C3", "C4"}, "", {}};
    EXPECT_EQ(m_writer->writeAcquisitionConfig(CHANNELS, SAMPLE_RATE, CHUNK, channels), 0u);
    EXPECT_EQ(m_writer->writeDrifts(drift_st{}), 0u);
}

TEST_F(LSLOutputWriterTest, withoutAMontageAChannelSaysOnlyItsName)
{
    lsl::stream_inlet signal(find(m_signalName));
    lsl::stream_info info = signal.info(TIMEOUT);

    EXPECT_EQ(info.type(), "EEG");
    lsl::xml_element channel = info.desc().child("channels").child("channel");
    EXPECT_STREQ(channel.child_value("label"), "C3");
    EXPECT_TRUE(channel.child("type").empty());
    EXPECT_TRUE(channel.child("unit").empty());
    EXPECT_TRUE(channel.child("location").empty());
    EXPECT_TRUE(info.desc().child("cap").empty());
}

TEST_F(LSLOutputWriterTest, aMontageGivesTypesAndPositions)
{
    m_writer = create(withMontage());
    lsl::stream_inlet signal(find(m_signalName));
    lsl::stream_info info = signal.info(TIMEOUT);

    EXPECT_STREQ(info.desc().child("cap").child_value("name"), "Refa-64.json");

    lsl::xml_element cz = info.desc().child("channels").child("channel");
    EXPECT_STREQ(cz.child_value("label"), "Cz");
    EXPECT_STREQ(cz.child_value("type"), "EEG");
    EXPECT_NEAR(std::atof(cz.child("location").child_value("X")), 0.0, 1e-3);
    EXPECT_NEAR(std::atof(cz.child("location").child_value("Y")), 0.0, 1e-3);
    EXPECT_NEAR(std::atof(cz.child("location").child_value("Z")), 85.0, 1e-3);

    lsl::xml_element eog = cz.next_sibling();
    EXPECT_STREQ(eog.child_value("label"), "EOG");
    EXPECT_STREQ(eog.child_value("type"), "AUX");
    EXPECT_TRUE(eog.child("location").empty());
}

TEST_F(LSLOutputWriterTest, positionsFollowXdfsAxes)
{
    ovChannelInfo info = withMontage();
    info.positions     = {{true, 90.0, 0.0}, {true, 90.0, 90.0}};  // Right ear, nose
    m_writer           = create(info);
    lsl::stream_inlet signal(find(m_signalName));
    lsl::stream_info full = signal.info(TIMEOUT);

    lsl::xml_element right = full.desc().child("channels").child("channel");
    EXPECT_NEAR(std::atof(right.child("location").child_value("X")), 85.0, 1e-3);
    EXPECT_NEAR(std::atof(right.child("location").child_value("Y")), 0.0, 1e-3);
    lsl::xml_element front = right.next_sibling();
    EXPECT_NEAR(std::atof(front.child("location").child_value("X")), 0.0, 1e-3);
    EXPECT_NEAR(std::atof(front.child("location").child_value("Y")), 85.0, 1e-3);
}

// A restart of the server has to give the same ID, for a client to recover the stream
TEST_F(LSLOutputWriterTest, theStreamIdDoesNotChange)
{
    const auto second = create();
    const auto found  = lsl::resolve_stream("name", m_signalName, 2, TIMEOUT);
    ASSERT_EQ(found.size(), 2u);
    EXPECT_FALSE(found[0].source_id().empty());
    EXPECT_EQ(found[0].source_id(), found[1].source_id());
    EXPECT_NE(found[0].source_id(), find(m_markerName).source_id());
}

}  // namespace
