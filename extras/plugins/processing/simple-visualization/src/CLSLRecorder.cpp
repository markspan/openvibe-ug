// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see CLSLRecorder.h.

#include "CLSLRecorder.h"

#include <lsl_cpp.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {
using Clock = std::chrono::steady_clock;

/// As LabRecorder: a clock offset every 5 s, a boundary chunk every 10 s.
constexpr auto OFFSET_INTERVAL   = std::chrono::seconds(5);
constexpr auto BOUNDARY_INTERVAL = std::chrono::seconds(10);
constexpr auto LOOKUP_INTERVAL   = std::chrono::seconds(1);
constexpr auto IDLE_SLEEP        = std::chrono::milliseconds(5);
/// How long opening a stream, and measuring its clock offset, may take, in seconds.
constexpr double OPEN_TIMEOUT   = 3.0;
constexpr double OFFSET_TIMEOUT = 2.0;
/// How much the inlet buffers, in seconds, should the recording thread be held up: LabRecorder's default.
constexpr int MAX_BUFFER = 360;

size_t bytesPerValue(const lsl::channel_format_t format)
{
    switch (format)
    {
        case lsl::cf_float32: return 4;
        case lsl::cf_double64: return 8;
        case lsl::cf_int32: return 4;
        case lsl::cf_int16: return 2;
        case lsl::cf_int8: return 1;
        case lsl::cf_int64: return 8;
        default: return 0;  // text, or a format this does not know
    }
}
}  // namespace

struct CLSLRecorder::SStream
{
    uint32_t id = 0;
    std::string name;
    std::unique_ptr<lsl::stream_inlet> inlet;
    lsl::channel_format_t format = lsl::cf_undefined;
    size_t channels = 0;          ///< Of the stream
    std::vector<size_t> keep;     ///< The channels recorded, counted from 0; all of them, or those chosen
    std::vector<char> kept;       ///< The samples of a chunk, of the channels recorded only
    std::vector<std::string> keptTexts;
    size_t room     = 0;  ///< Samples pulled at most at a time
    double first = 0.0, last = 0.0;
    uint64_t count = 0;
    std::vector<std::pair<double, double>> offsets;
    Clock::time_point nextOffset;
    std::vector<char> values;
    std::vector<std::string> texts;
    std::vector<double> stamps;
};

CLSLRecorder::CLSLRecorder(const std::string& pathUtf8)
{
    if (!m_writer.open(pathUtf8)) { return; }
    m_thread = std::thread([this] { run(); });
}

CLSLRecorder::~CLSLRecorder()
{
    m_running = false;
    if (m_thread.joinable()) { m_thread.join(); }
    m_writer.close();
}

void CLSLRecorder::select(const std::map<std::string, Channels>& streams)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_selected = streams;
}

std::string CLSLRecorder::recordLabel(const std::string& uid, const Channels& channels)
{
    if (channels.empty()) { return uid; }
    std::string label = uid + "#";
    for (size_t i = 0; i < channels.size(); ++i) { label += (i == 0 ? "" : ",") + std::to_string(channels[i]); }
    return label;
}

bool CLSLRecorder::parseRecordLabel(const std::string& label, std::string& uid, Channels& channels)
{
    channels.clear();
    const size_t hash = label.find('#');
    uid               = label.substr(0, hash);
    if (hash != std::string::npos)
    {
        std::istringstream list(label.substr(hash + 1));
        std::string index;
        while (std::getline(list, index, ','))
        {
            try { channels.push_back(size_t(std::stoul(index))); }
            catch (const std::exception&) {}  // a stray entry is left out
        }
    }
    return !uid.empty();
}

std::string CLSLRecorder::headerForChannels(const std::string& xml, const Channels& channels)
{
    // Edit a stream info made from the XML, through LSL's own XML handling
    lsl::stream_info info = lsl::stream_info::from_xml(xml);
    const size_t total    = size_t(info.channel_count());
    lsl::xml_element list = info.desc().child("channels");
    if (!list.empty())
    {
        std::vector<lsl::xml_element> entries;
        for (lsl::xml_element entry = list.child("channel"); !entry.empty(); entry = entry.next_sibling("channel")) { entries.push_back(entry); }
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (std::find(channels.begin(), channels.end(), i) == channels.end()) { list.remove_child(entries[i]); }
        }
    }
    // Which channels of the stream these are, counted from 1, as people count channels
    std::string recorded;
    for (size_t i = 0; i < channels.size(); ++i) { recorded += (i == 0 ? "" : ",") + std::to_string(channels[i] + 1); }
    lsl::xml_element note = info.desc().append_child("openvibe_ug");
    note.append_child_value("recorded_channels", recorded);
    note.append_child_value("stream_channel_count", std::to_string(total));

    // The channel count is not editable through the API, and appears once, at the top
    std::string out          = info.as_xml();
    const std::string before = "<channel_count>" + std::to_string(total) + "</channel_count>";
    const size_t at          = out.find(before);
    if (at != std::string::npos) { out.replace(at, before.size(), "<channel_count>" + std::to_string(channels.size()) + "</channel_count>"); }
    return out;
}

void CLSLRecorder::run()
{
    lsl::continuous_resolver resolver(5.0);
    std::map<std::string, lsl::stream_info> found;
    auto nextLookUp   = Clock::now();
    auto nextBoundary = Clock::now() + BOUNDARY_INTERVAL;

    while (m_running)
    {
        if (Clock::now() >= nextLookUp)
        {
            nextLookUp = Clock::now() + LOOKUP_INTERVAL;
            found.clear();
            try
            {
                for (const lsl::stream_info& info : resolver.results()) { found.emplace(info.uid(), info); }
            }
            catch (const std::exception& ex) { std::cout << "[LSL recorder] Could not look up the streams: " << ex.what() << std::endl; }
        }

        std::map<std::string, Channels> selected;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            selected = m_selected;
        }
        // Streams no longer selected end here; those newly selected start
        for (auto it = m_streams.begin(); it != m_streams.end();)
        {
            if (selected.count(it->first) != 0) { ++it; continue; }
            end(*it->second);
            it = m_streams.erase(it);
        }
        for (const auto& [uid, chosen] : selected)
        {
            if (m_streams.count(uid) != 0 || found.count(uid) == 0) { continue; }
            auto stream = std::make_unique<SStream>();
            try
            {
                // No post-processing: the file keeps the sender's own time stamps, as LabRecorder's does
                stream->inlet = std::make_unique<lsl::stream_inlet>(found.at(uid), MAX_BUFFER);
                stream->inlet->open_stream(OPEN_TIMEOUT);
                const lsl::stream_info info = stream->inlet->info(OPEN_TIMEOUT);
                stream->name     = info.name();
                stream->format   = info.channel_format();
                stream->channels = size_t(info.channel_count());
                stream->room     = std::max<size_t>(64, size_t(info.nominal_srate() / 2.0));
                if (stream->format != lsl::cf_string && bytesPerValue(stream->format) == 0)
                {
                    std::cout << "[LSL recorder] " << stream->name << " has a sample format that can not be recorded; it is left out." << std::endl;
                    continue;
                }
                // The channels chosen, those the stream has; all of them when none were chosen
                for (const size_t c : chosen)
                {
                    if (c < stream->channels) { stream->keep.push_back(c); }
                }
                const bool all = stream->keep.empty() || stream->keep.size() == stream->channels;
                if (all)
                {
                    stream->keep.clear();
                    for (size_t c = 0; c < stream->channels; ++c) { stream->keep.push_back(c); }
                }
                stream->id = m_nextId++;
                m_writer.writeStreamHeader(stream->id, all ? info.as_xml() : headerForChannels(info.as_xml(), stream->keep));
            }
            catch (const std::exception& ex)
            {
                std::cout << "[LSL recorder] Could not open the stream " << found.at(uid).name() << ": " << ex.what() << std::endl;
                continue;  // tried again at the next pass
            }
            stream->nextOffset = Clock::now();  // the first offset at once, as LabRecorder
            std::cout << "[LSL recorder] Recording " << stream->name << " as stream " << stream->id << ", " << stream->keep.size() << " of its "
                      << stream->channels << " channels" << std::endl;
            m_streams.emplace(uid, std::move(stream));
        }
        m_recording = m_streams.size();

        bool any = false;
        for (auto& [uid, stream] : m_streams)
        {
            try
            {
                any = record(*stream) || any;
                if (Clock::now() >= stream->nextOffset)
                {
                    stream->nextOffset  = Clock::now() + OFFSET_INTERVAL;
                    const double now    = lsl::local_clock();
                    const double offset = stream->inlet->time_correction(OFFSET_TIMEOUT);
                    // When it was measured, on the sender's clock, as LabRecorder writes it
                    m_writer.writeClockOffset(stream->id, now - offset, offset);
                    stream->offsets.emplace_back(now - offset, offset);
                }
            }
            catch (const std::exception& ex)
            {
                // A stream that went away: its inlet recovers by itself when it comes back
                std::cout << "[LSL recorder] " << stream->name << ": " << ex.what() << std::endl;
            }
        }
        if (Clock::now() >= nextBoundary)
        {
            nextBoundary = Clock::now() + BOUNDARY_INTERVAL;
            m_writer.writeBoundary();
        }
        if (!any) { std::this_thread::sleep_for(IDLE_SLEEP); }
    }

    // What came in last, and every stream's footer
    for (auto& [uid, stream] : m_streams)
    {
        try { while (record(*stream)) {} }
        catch (const std::exception&) {}
        end(*stream);
    }
    m_streams.clear();
    m_recording = 0;
}

bool CLSLRecorder::record(SStream& stream)
{
    stream.stamps.resize(stream.room);
    size_t count = 0;
    if (stream.format == lsl::cf_string)
    {
        stream.texts.resize(stream.room * stream.channels);
        count = stream.inlet->pull_chunk_multiplexed(stream.texts.data(), stream.stamps.data(), stream.texts.size(), stream.stamps.size(), 0.0)
                / stream.channels;
        if (count == 0) { return false; }
        if (stream.keep.size() == stream.channels) { m_writer.writeTextSamples(stream.id, stream.stamps.data(), count, stream.texts.data(), stream.channels); }
        else
        {
            stream.keptTexts.resize(count * stream.keep.size());
            for (size_t s = 0; s < count; ++s)
            {
                for (size_t k = 0; k < stream.keep.size(); ++k) { stream.keptTexts[s * stream.keep.size() + k] = stream.texts[s * stream.channels + stream.keep[k]]; }
            }
            m_writer.writeTextSamples(stream.id, stream.stamps.data(), count, stream.keptTexts.data(), stream.keep.size());
        }
    }
    else
    {
        const size_t width = bytesPerValue(stream.format);
        stream.values.resize(stream.room * stream.channels * width);
        const size_t elements = stream.room * stream.channels;
        void* data            = stream.values.data();
        switch (stream.format)
        {
            case lsl::cf_float32: count = stream.inlet->pull_chunk_multiplexed(static_cast<float*>(data), stream.stamps.data(), elements, stream.room, 0.0); break;
            case lsl::cf_double64: count = stream.inlet->pull_chunk_multiplexed(static_cast<double*>(data), stream.stamps.data(), elements, stream.room, 0.0); break;
            case lsl::cf_int32: count = stream.inlet->pull_chunk_multiplexed(static_cast<int32_t*>(data), stream.stamps.data(), elements, stream.room, 0.0); break;
            case lsl::cf_int16: count = stream.inlet->pull_chunk_multiplexed(static_cast<int16_t*>(data), stream.stamps.data(), elements, stream.room, 0.0); break;
            case lsl::cf_int8: count = stream.inlet->pull_chunk_multiplexed(static_cast<char*>(data), stream.stamps.data(), elements, stream.room, 0.0); break;
            case lsl::cf_int64: count = stream.inlet->pull_chunk_multiplexed(static_cast<int64_t*>(data), stream.stamps.data(), elements, stream.room, 0.0); break;
            default: return false;
        }
        count /= stream.channels;
        if (count == 0) { return false; }
        if (stream.keep.size() == stream.channels) { m_writer.writeSamples(stream.id, stream.stamps.data(), count, stream.values.data(), stream.channels * width); }
        else
        {
            // Only the channels recorded, in their order
            const size_t keptWidth = stream.keep.size() * width;
            stream.kept.resize(count * keptWidth);
            for (size_t s = 0; s < count; ++s)
            {
                for (size_t k = 0; k < stream.keep.size(); ++k)
                {
                    std::memcpy(stream.kept.data() + s * keptWidth + k * width, stream.values.data() + (s * stream.channels + stream.keep[k]) * width, width);
                }
            }
            m_writer.writeSamples(stream.id, stream.stamps.data(), count, stream.kept.data(), keptWidth);
        }
    }

    if (stream.count == 0) { stream.first = stream.stamps[0]; }
    stream.last = stream.stamps[count - 1];
    stream.count += count;
    return true;
}

void CLSLRecorder::end(SStream& stream)
{
    m_writer.writeStreamFooter(stream.id, CXDFWriter::footerXml(stream.first, stream.last, stream.count, stream.offsets));
    std::cout << "[LSL recorder] " << stream.name << ": " << stream.count << " samples recorded" << std::endl;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
