/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file. The LSL driver keeps the times the sender gave its samples and markers, through real LSL streams on this PC.
 * See UG.md, 2.64.
 */
#include "CDriverLabStreamingLayer.h"
#include "LslClock.h"

#include "toolkit/ovtk_defines.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int RATE       = 256;
constexpr int CHANNELS   = 2;
constexpr int SAMPLES    = 64;
constexpr double AGE     = 5.0;     // The sender says its samples were taken this long ago, as an amplifier's driver can
constexpr double SAME_MS = 1.0;     // Times that both come from the sender agree to this, in ms

std::string uniqueName(const std::string& prefix)
{
    std::random_device random;
    return prefix + "_" + std::to_string(random());
}

void setParameter(AbstractDriver& driver, const std::string& name, const std::string& value)
{
    for (auto* parameter : driver.getParameters()) {
        if (parameter->name() == name) { parameter->fromString(value); }
    }
}

double ms(const uint64_t a, const uint64_t b) { return (double(a) - double(b)) / 1e6; }

}  // namespace

TEST(LSLDriverTest, SamplesAndMarkersKeepTheTimesTheirSenderGave)
{
    const std::string signalName = uniqueName("ovtest_lsl_in_signal");
    const std::string markerName = uniqueName("ovtest_lsl_in_markers");
    lsl::stream_outlet signal(lsl::stream_info(signalName, "EEG", CHANNELS, RATE, lsl::cf_float32, signalName + "_src"));
    lsl::stream_outlet markers(lsl::stream_info(markerName, "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_int32, markerName + "_src"));

    CDriverLabStreamingLayer driver;
    // The driver offers the streams it finds; give LSL a moment to see these
    for (int attempt = 0; attempt < 5; ++attempt) {
        setParameter(driver, "SignalStream", signalName + " / " + signalName + "_src");
        setParameter(driver, "MarkerStream", markerName + " / " + markerName + "_src");
        if (driver.init()) { break; }
    }
    ASSERT_EQ(driver.channelCount().value(), CHANNELS);
    ASSERT_TRUE(driver.start());
    ASSERT_TRUE(signal.wait_for_consumers(10.0));
    ASSERT_TRUE(markers.wait_for_consumers(10.0));

    // Samples taken AGE seconds ago, at the nominal rate, and two markers on samples 10 and 20: a trigger and another code
    const double first = lsl::local_clock() - AGE;
    std::vector<float> sample(CHANNELS);
    for (int i = 0; i < SAMPLES; ++i) {
        sample[0] = float(i);
        signal.push_sample(sample, first + double(i) / RATE);
    }
    const int trigger = 7, other = 33024;
    markers.push_sample(&trigger, first + 10.0 / RATE);
    markers.push_sample(&other, first + 20.0 / RATE);

    // Read as the server does
    std::vector<timestampType> stamps(128);
    std::vector<sampleType> values(128 * CHANNELS);
    std::vector<ovStimulation> stims(16);
    samplesBuffer buffer{stamps.data(), values.data(), stims.data(), 0, 0, uint32_t(stims.size()), stamps.size()};

    std::vector<timestampType> gotStamps;
    std::vector<ovStimulation> gotStims;
    for (const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
         (gotStamps.size() < SAMPLES || gotStims.size() < 2) && std::chrono::steady_clock::now() < end;) {
        driver.getData(&buffer);
        gotStamps.insert(gotStamps.end(), stamps.begin(), stamps.begin() + std::ptrdiff_t(buffer.count));
        gotStims.insert(gotStims.end(), stims.begin(), stims.begin() + buffer.stimCount);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    driver.stop();
    driver.unInit();

    ASSERT_EQ(gotStamps.size(), size_t(SAMPLES));
    ASSERT_EQ(gotStims.size(), 2U);

    // Each sample at its sender's time, on the server's clock: AGE seconds before it was read, which upstream dated it
    const double offset = LslClock::measureOffset();
    for (int i = 0; i < SAMPLES; ++i) {
        EXPECT_NEAR(ms(gotStamps[size_t(i)], LslClock::toServerStamp(first + double(i) / RATE, offset)), 0.0, SAME_MS) << "sample " << i;
    }
    const uint64_t now = LslClock::toServerStamp(lsl::local_clock(), offset);
    EXPECT_GT(ms(now, gotStamps.back()), AGE * 1000.0 - 100.0) << "not dated when read";

    // The markers on their samples; a value from 1 to 255 is a trigger, as an amplifier's are
    EXPECT_EQ(gotStims[0].id, uint64_t(OVTK_StimulationId_Label(trigger)));
    EXPECT_NEAR(ms(gotStims[0].date, gotStamps[10]), 0.0, SAME_MS);
    EXPECT_EQ(gotStims[1].id, uint64_t(other));
    EXPECT_NEAR(ms(gotStims[1].date, gotStamps[20]), 0.0, SAME_MS);
}

TEST(LSLDriverTest, MarkerValuesFromOneTo255AreTriggers)
{
    EXPECT_EQ(CDriverLabStreamingLayer::stimulationOfMarker(1), uint64_t(OVTK_StimulationId_Label(1)));
    EXPECT_EQ(CDriverLabStreamingLayer::stimulationOfMarker(255), uint64_t(OVTK_StimulationId_Label(255)));
    EXPECT_EQ(CDriverLabStreamingLayer::stimulationOfMarker(0), 0U);
    EXPECT_EQ(CDriverLabStreamingLayer::stimulationOfMarker(33025), 33025U) << "an OpenViBE stimulation ID stays one";
}
