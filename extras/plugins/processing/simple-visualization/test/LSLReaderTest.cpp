// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.28. Tests the LSL display's reader against streams this test sends itself.

#include "CLSLReader.h"

#include <lsl_cpp.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <thread>

using namespace OpenViBE::Plugins::SimpleVisualization;

namespace {
using Clock = std::chrono::steady_clock;

/// Waits up to \p seconds for \p done, checking every 20 ms.
template <class Done>
bool waitFor(const double seconds, Done done)
{
    const auto until = Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
    while (Clock::now() < until)
    {
        if (done()) { return true; }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return done();
}

/// The uid of the stream named \p name, once the reader lists it; empty if it does not within 10 s.
std::string findStream(CLSLReader& reader, const std::string& name)
{
    std::string uid;
    waitFor(10.0, [&] {
        for (const SLSLStreamInfo& info : reader.available())
        {
            if (info.name == name) { uid = info.uid; }
        }
        return !uid.empty();
    });
    return uid;
}
}  // namespace

TEST(LSLReader, readsADataStreamIntoARingWithItsLabelsAndTimes)
{
    lsl::stream_info info("UGTestData", "EEG", 3, 100.0, lsl::cf_float32, "ug-lsl-display-test-data");
    lsl::xml_element channels = info.desc().append_child("channels");
    for (const char* label : { "Fp1", "Cz", "Trigger" }) { channels.append_child("channel").append_child_value("label", label).append_child_value("unit", "microvolts"); }
    lsl::stream_outlet outlet(info);

    CLSLReader reader(10.0);
    const std::string uid = findStream(reader, "UGTestData");
    ASSERT_FALSE(uid.empty()) << "the test's stream was not found";
    for (const SLSLStreamInfo& stream : reader.available())
    {
        if (stream.uid == uid) { EXPECT_FALSE(stream.isMarkers()); }
    }
    reader.select({uid});

    std::vector<SLSLOpenedStream> opened;
    ASSERT_TRUE(waitFor(10.0, [&] {
        for (SLSLOpenedStream& stream : reader.takeOpened()) { opened.push_back(std::move(stream)); }
        return !opened.empty();
    }));
    ASSERT_EQ(opened.size(), 1U);
    EXPECT_EQ(opened[0].labels, (std::vector<std::string>{ "Fp1", "Cz", "Trigger" }));
    EXPECT_EQ(opened[0].units[0], "microvolts");

    // A second of samples, dated as LSL dates them
    const double start = lsl::local_clock();
    for (int s = 0; s < 100; ++s)
    {
        const std::vector<float> sample = { float(s), 2.0F * float(s), 0.0F };
        outlet.push_sample(sample, start + s / 100.0);
    }
    ASSERT_TRUE(waitFor(5.0, [&] { return opened[0].ring->totalSamples() >= 100; }));

    CTraceBuffer::CReader r(*opened[0].ring);
    EXPECT_EQ(r.channels(), 3U);
    EXPECT_DOUBLE_EQ(r.sampling(), 100.0);
    EXPECT_NEAR(r.firstSampleSeconds(), start, 0.01);  // the stream's own time, on this computer's clock
    const size_t last = (r.writePos() + r.capacity() - 1) % r.capacity();
    EXPECT_FLOAT_EQ(r.channel(0)[last], 99.0F);
    EXPECT_FLOAT_EQ(r.channel(1)[last], 198.0F);
}

TEST(LSLReader, readsNumberAndTextMarkersWithTheirTimes)
{
    lsl::stream_outlet numbers(lsl::stream_info("UGTestCodes", "Markers", 2, lsl::IRREGULAR_RATE, lsl::cf_int32, "ug-lsl-display-test-codes"));
    lsl::stream_outlet texts(lsl::stream_info("UGTestTexts", "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_string, "ug-lsl-display-test-texts"));

    CLSLReader reader(10.0);
    const std::string codes = findStream(reader, "UGTestCodes"), words = findStream(reader, "UGTestTexts");
    ASSERT_FALSE(codes.empty());
    ASSERT_FALSE(words.empty());
    reader.select({codes, words});
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));  // time to open both

    const double t = lsl::local_clock();
    numbers.push_sample(std::vector<int32_t>{ 42, 1234 }, t);  // a code, and e.g. a call's duration: the code is the first
    texts.push_sample(std::vector<std::string>{ "fixation" }, t + 0.5);

    std::vector<SLSLMarker> markers;
    ASSERT_TRUE(waitFor(5.0, [&] {
        for (SLSLMarker& marker : reader.takeMarkers()) { markers.push_back(std::move(marker)); }
        return markers.size() >= 2;
    }));
    ASSERT_EQ(markers.size(), 2U);
    std::sort(markers.begin(), markers.end(), [](const SLSLMarker& a, const SLSLMarker& b) { return a.time < b.time; });
    EXPECT_EQ(markers[0].text, "42");
    EXPECT_EQ(markers[0].id, 42U);
    EXPECT_NEAR(markers[0].time, t, 0.01);
    EXPECT_EQ(markers[1].text, "fixation");
    EXPECT_NEAR(markers[1].time, t + 0.5, 0.01);
}

TEST(LSLReader, aStreamNoLongerSelectedIsClosed)
{
    lsl::stream_outlet outlet(lsl::stream_info("UGTestClose", "EEG", 1, 50.0, lsl::cf_double64, "ug-lsl-display-test-close"));

    CLSLReader reader(10.0);
    const std::string uid = findStream(reader, "UGTestClose");
    ASSERT_FALSE(uid.empty());
    reader.select({uid});
    ASSERT_TRUE(waitFor(10.0, [&] { return !reader.takeOpened().empty(); }));

    reader.select({});
    std::vector<std::string> closed;
    ASSERT_TRUE(waitFor(5.0, [&] {
        for (const std::string& c : reader.takeClosed()) { closed.push_back(c); }
        return !closed.empty();
    }));
    EXPECT_EQ(closed, std::vector<std::string>{ uid });
}
