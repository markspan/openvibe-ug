// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.29. Records LSL streams this test sends itself, and reads the XDF file back.

#include "CLSLRecorder.h"
#include "XDFWriter.h"

#include <lsl_cpp.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <thread>

using namespace OpenViBE::Plugins::SimpleVisualization;

namespace {
/// An XDF file as read back by this test's own parser, which follows the specification, not the writer.
struct SParsedStream
{
    std::string header, footer;
    std::vector<double> stamps;
    std::vector<std::vector<char>> samples;  ///< The raw bytes of each numeric sample
    std::vector<std::vector<std::string>> texts;
    size_t clockOffsets = 0;
};

uint64_t readVarLen(const std::string& data, size_t& at)
{
    const uint8_t bytes = uint8_t(data.at(at++));
    uint64_t value      = 0;
    if (bytes == 1) { value = uint8_t(data.at(at)); }
    else if (bytes == 4) { uint32_t v; std::memcpy(&v, data.data() + at, 4); value = v; }
    else if (bytes == 8) { std::memcpy(&value, data.data() + at, 8); }
    else { throw std::runtime_error("bad variable-length integer"); }
    at += bytes;
    return value;
}

template <class T>
T readRaw(const std::string& data, size_t& at)
{
    T value;
    std::memcpy(&value, data.data() + at, sizeof(T));
    at += sizeof(T);
    return value;
}

/// The value of an XML element, e.g. <name>, in \p xml.
std::string xmlValue(const std::string& xml, const std::string& element)
{
    const size_t from = xml.find("<" + element + ">");
    if (from == std::string::npos) { return {}; }
    const size_t start = from + element.size() + 2;
    return xml.substr(start, xml.find("</" + element + ">", start) - start);
}

std::map<uint32_t, SParsedStream> parse(const std::string& path, size_t& boundaries, std::string& fileHeader)
{
    std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
    const std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_EQ(data.substr(0, 4), "XDF:");
    std::map<uint32_t, SParsedStream> streams;
    boundaries = 0;
    size_t at  = 4;
    while (at < data.size())
    {
        const uint64_t length = readVarLen(data, at);
        const size_t end      = at + length;
        const uint16_t tag    = readRaw<uint16_t>(data, at);
        if (tag == 1) { fileHeader = data.substr(at, end - at); }
        else if (tag == 5) { ++boundaries; }
        else
        {
            const uint32_t id      = readRaw<uint32_t>(data, at);
            SParsedStream& stream = streams[id];
            if (tag == 2) { stream.header = data.substr(at, end - at); }
            else if (tag == 6) { stream.footer = data.substr(at, end - at); }
            else if (tag == 4) { ++stream.clockOffsets; }
            else if (tag == 3)
            {
                const bool text       = xmlValue(stream.header, "channel_format") == "string";
                const size_t channels = size_t(std::stoul(xmlValue(stream.header, "channel_count")));
                const std::string fmt = xmlValue(stream.header, "channel_format");
                const size_t width    = fmt == "float32" ? 4 : fmt == "double64" ? 8 : fmt == "int16" ? 2 : fmt == "int32" ? 4 : fmt == "int8" ? 1 : 8;
                const uint64_t count  = readVarLen(data, at);
                for (uint64_t s = 0; s < count; ++s)
                {
                    EXPECT_EQ(uint8_t(data.at(at)), 8U) << "every sample has its time stamp";
                    ++at;
                    stream.stamps.push_back(readRaw<double>(data, at));
                    if (text)
                    {
                        std::vector<std::string> values;
                        for (size_t c = 0; c < channels; ++c)
                        {
                            const uint64_t size = readVarLen(data, at);
                            values.push_back(data.substr(at, size));
                            at += size;
                        }
                        stream.texts.push_back(values);
                    }
                    else
                    {
                        stream.samples.emplace_back(data.begin() + at, data.begin() + at + channels * width);
                        at += channels * width;
                    }
                }
                EXPECT_EQ(at, end) << "a Samples chunk is as long as it says";
            }
        }
        at = end;
    }
    return streams;
}

const SParsedStream* byName(const std::map<uint32_t, SParsedStream>& streams, const std::string& name)
{
    for (const auto& [id, stream] : streams)
    {
        if (xmlValue(stream.header, "name") == name) { return &stream; }
    }
    return nullptr;
}

std::string findUid(const std::string& name)
{
    const std::vector<lsl::stream_info> found = lsl::resolve_stream("name", name, 1, 10.0);
    return found.empty() ? std::string() : found[0].uid();
}
}  // namespace

TEST(XDFWriter, varLenIntegersTakeOneFourOrEightBytes)
{
    std::string out;
    CXDFWriter::appendVarLen(out, 200);
    CXDFWriter::appendVarLen(out, 70000);
    CXDFWriter::appendVarLen(out, 0x100000000ULL);
    size_t at = 0;
    EXPECT_EQ(readVarLen(out, at), 200U);
    EXPECT_EQ(readVarLen(out, at), 70000U);
    EXPECT_EQ(readVarLen(out, at), 0x100000000ULL);
    EXPECT_EQ(out.size(), 2U + 5U + 9U);
}

TEST(LSLRecorder, labelsCarryAStreamAndItsChannels)
{
    EXPECT_EQ(CLSLRecorder::recordLabel("abc-123", {}), "abc-123");
    EXPECT_EQ(CLSLRecorder::recordLabel("abc-123", { 0, 2, 5 }), "abc-123#0,2,5");

    std::string uid;
    CLSLRecorder::Channels channels;
    ASSERT_TRUE(CLSLRecorder::parseRecordLabel("abc-123#0,2,5", uid, channels));
    EXPECT_EQ(uid, "abc-123");
    EXPECT_EQ(channels, (CLSLRecorder::Channels{ 0, 2, 5 }));
    ASSERT_TRUE(CLSLRecorder::parseRecordLabel("abc-123", uid, channels));
    EXPECT_TRUE(channels.empty()) << "a plain uid: all channels";
    EXPECT_FALSE(CLSLRecorder::parseRecordLabel("", uid, channels)) << "an empty label: nothing to record";
}

TEST(LSLRecorder, recordsDataAndMarkersAsLabRecorderWouldWithEverySampleAndTimeStamp)
{
    lsl::stream_info eegInfo("UGRecEEG", "EEG", 3, 200.0, lsl::cf_float32, "ug-recorder-test-eeg");
    lsl::xml_element channels = eegInfo.desc().append_child("channels");
    for (const char* label : { "Fp1", "Cz", "Pz" }) { channels.append_child("channel").append_child_value("label", label); }
    lsl::stream_outlet eeg(eegInfo);
    lsl::stream_outlet counter(lsl::stream_info("UGRecCounter", "Misc", 1, 100.0, lsl::cf_int16, "ug-recorder-test-counter"));
    lsl::stream_outlet markers(lsl::stream_info("UGRecMarkers", "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_string, "ug-recorder-test-markers"));

    // Of the EEG, only Fp1 and Pz, as when only they are shown; the others whole
    const std::map<std::string, CLSLRecorder::Channels> uids = {
        { findUid("UGRecEEG"), { 0, 2 } }, { findUid("UGRecCounter"), {} }, { findUid("UGRecMarkers"), {} }
    };
    ASSERT_EQ(uids.size(), 3U);
    ASSERT_EQ(uids.count(""), 0U);

    const std::string path = (std::filesystem::temp_directory_path() / "ug-lsl-recorder-test.xdf").u8string();
    {
        CLSLRecorder recorder(path);
        ASSERT_TRUE(recorder.ok());
        recorder.select(uids);
        for (int i = 0; i < 500 && recorder.recording() < 3; ++i) { std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
        ASSERT_EQ(recorder.recording(), 3U);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // the inlets subscribed

        // 400 samples at 200 Hz, 100 at 100 Hz, 3 markers, all with time stamps of the test's choosing
        const double start = lsl::local_clock();
        for (int s = 0; s < 400; ++s) { eeg.push_sample(std::vector<float>{ float(s), float(-s), 0.5F }, start + s / 200.0); }
        for (int s = 0; s < 100; ++s) { counter.push_sample(std::vector<int16_t>{ int16_t(s) }, start + s / 100.0); }
        for (const char* text : { "start", "S 12", "end" }) { markers.push_sample(std::vector<std::string>{ text }, start + 0.25); }
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        EXPECT_TRUE(recorder.ok());
    }  // the footers, and the file closed

    size_t boundaries = 0;
    std::string fileHeader;
    const auto streams = parse(path, boundaries, fileHeader);
    EXPECT_EQ(xmlValue(fileHeader, "version"), "1.0");
    ASSERT_EQ(streams.size(), 3U);

    const SParsedStream* recordedEEG = byName(streams, "UGRecEEG");
    ASSERT_NE(recordedEEG, nullptr);
    EXPECT_EQ(xmlValue(recordedEEG->header, "channel_format"), "float32");
    EXPECT_EQ(xmlValue(recordedEEG->header, "channel_count"), "2") << "only the channels shown";
    EXPECT_NE(recordedEEG->header.find("<label>Fp1</label>"), std::string::npos) << "their descriptions are in the header";
    EXPECT_NE(recordedEEG->header.find("<label>Pz</label>"), std::string::npos);
    EXPECT_EQ(recordedEEG->header.find("<label>Cz</label>"), std::string::npos) << "the channel left out is not described";
    EXPECT_EQ(xmlValue(recordedEEG->header, "recorded_channels"), "1,3") << "which channels of the stream, counted from 1";
    EXPECT_EQ(xmlValue(recordedEEG->header, "stream_channel_count"), "3");
    ASSERT_EQ(recordedEEG->stamps.size(), 400U);
    const double start = recordedEEG->stamps[0];
    for (size_t s = 0; s < 400; ++s)
    {
        EXPECT_DOUBLE_EQ(recordedEEG->stamps[s], start + double(s) / 200.0) << "the sender's own time stamp, uncorrected";
        ASSERT_EQ(recordedEEG->samples[s].size(), 2 * sizeof(float));
        float values[2];
        std::memcpy(values, recordedEEG->samples[s].data(), sizeof(values));
        EXPECT_FLOAT_EQ(values[0], float(s)) << "Fp1";
        EXPECT_FLOAT_EQ(values[1], 0.5F) << "Pz";
    }
    EXPECT_EQ(xmlValue(recordedEEG->footer, "sample_count"), "400");
    EXPECT_GE(recordedEEG->clockOffsets, 1U) << "a clock offset at the start, as LabRecorder";

    const SParsedStream* recordedCounter = byName(streams, "UGRecCounter");
    ASSERT_NE(recordedCounter, nullptr);
    ASSERT_EQ(recordedCounter->samples.size(), 100U);
    int16_t last = 0;
    std::memcpy(&last, recordedCounter->samples.back().data(), 2);
    EXPECT_EQ(last, 99);

    const SParsedStream* recordedMarkers = byName(streams, "UGRecMarkers");
    ASSERT_NE(recordedMarkers, nullptr);
    ASSERT_EQ(recordedMarkers->texts.size(), 3U);
    EXPECT_EQ(recordedMarkers->texts[1][0], "S 12");
    EXPECT_DOUBLE_EQ(recordedMarkers->stamps[0], start + 0.25);
}
