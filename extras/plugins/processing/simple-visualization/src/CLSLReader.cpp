// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see CLSLReader.h.

#include "CLSLReader.h"

#include <lsl_cpp.h>

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {
/// How often the reader looks for streams on the network.
constexpr auto LOOKUP_INTERVAL = std::chrono::seconds(1);
/// How long the reader sleeps when nothing came in: a display needs no less.
constexpr auto IDLE_SLEEP = std::chrono::milliseconds(5);
/// How long opening a stream may take before it is given up, in seconds.
constexpr double OPEN_TIMEOUT = 3.0;

SLSLStreamInfo describe(const lsl::stream_info& info)
{
    SLSLStreamInfo out;
    out.uid      = info.uid();
    out.name     = info.name();
    out.type     = info.type();
    out.hostname = info.hostname();
    out.channels = info.channel_count();
    out.rate     = info.nominal_srate();
    out.text     = info.channel_format() == lsl::cf_string;
    return out;
}

/// A marker's text for a number: without decimals when it is whole, as a code is.
std::string numberText(const double value)
{
    if (std::isfinite(value) && value == std::floor(value) && std::fabs(value) < 1e15) { return std::to_string(int64_t(value)); }
    std::ostringstream text;
    text << value;
    return text.str();
}
}  // namespace

struct CLSLReader::SInlet
{
    SLSLStreamInfo info;
    std::unique_ptr<lsl::stream_inlet> inlet;
    std::shared_ptr<CTraceBuffer> ring;  ///< Data streams only
    std::vector<double> samples;         ///< Sample-major, as LSL gives them
    std::vector<double> stamps;
    std::vector<double> channelMajor;    ///< As the ring takes them
};

CLSLReader::CLSLReader(const double windowSeconds) : m_origin(lsl::local_clock()), m_windowSeconds(windowSeconds)
{
    m_thread = std::thread([this] { run(); });
}

CLSLReader::~CLSLReader()
{
    m_running = false;
    if (m_thread.joinable()) { m_thread.join(); }
}

std::vector<SLSLStreamInfo> CLSLReader::available() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_available;
}

void CLSLReader::select(const std::set<std::string>& uids)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_selected = uids;
}

std::vector<SLSLOpenedStream> CLSLReader::takeOpened()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::exchange(m_opened, {});
}

std::vector<std::string> CLSLReader::takeClosed()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::exchange(m_closed, {});
}

std::vector<SLSLMarker> CLSLReader::takeMarkers()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::exchange(m_markers, {});
}

void CLSLReader::run()
{
    // Streams that went away are forgotten after 5 s, so that a stream that restarts is found again
    lsl::continuous_resolver resolver(5.0);
    auto nextLookUp = std::chrono::steady_clock::now();
    std::map<std::string, lsl::stream_info> found;  // by uid, from the last look-up

    while (m_running)
    {
        if (std::chrono::steady_clock::now() >= nextLookUp)
        {
            nextLookUp = std::chrono::steady_clock::now() + LOOKUP_INTERVAL;
            found.clear();
            std::vector<SLSLStreamInfo> available;
            try
            {
                for (const lsl::stream_info& info : resolver.results())
                {
                    found.emplace(info.uid(), info);
                    available.push_back(describe(info));
                }
            }
            catch (const std::exception& ex) { std::cout << "[LSL display] Could not look up the streams: " << ex.what() << std::endl; }
            std::lock_guard<std::mutex> lock(m_mutex);
            m_available = std::move(available);
        }

        // Open the streams newly selected, close those no longer selected
        std::set<std::string> selected;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            selected = m_selected;
        }
        for (auto it = m_inlets.begin(); it != m_inlets.end();)
        {
            if (selected.count(it->first) != 0) { ++it; continue; }
            const bool data = it->second->ring != nullptr;
            const std::string uid = it->first;
            it = m_inlets.erase(it);
            if (data)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_closed.push_back(uid);
            }
        }
        for (const std::string& uid : selected)
        {
            if (m_inlets.count(uid) != 0 || found.count(uid) == 0) { continue; }
            auto inlet  = std::make_unique<SInlet>();
            inlet->info = describe(found.at(uid));
            try
            {
                inlet->inlet = std::make_unique<lsl::stream_inlet>(found.at(uid), 30);
                // On this computer's clock, smoothed, and never back in time: as LabRecorder's files are when loaded
                inlet->inlet->set_postprocessing(inlet->info.isMarkers() ? proc_clocksync : proc_clocksync | proc_dejitter | proc_monotonize);
                inlet->inlet->open_stream(OPEN_TIMEOUT);
            }
            catch (const std::exception& ex)
            {
                std::cout << "[LSL display] Could not open the stream " << inlet->info.name << ": " << ex.what() << std::endl;
                continue;  // tried again at the next pass
            }

            if (!inlet->info.isMarkers())
            {
                SLSLOpenedStream opened;
                opened.info = inlet->info;
                try
                {
                    lsl::stream_info full = inlet->inlet->info(OPEN_TIMEOUT);  // desc() is not const
                    lsl::xml_element channel    = full.desc().child("channels").child("channel");
                    for (int c = 0; c < inlet->info.channels; ++c)
                    {
                        const std::string label = channel.empty() ? std::string() : channel.child_value("label");
                        opened.labels.push_back(label.empty() ? "Ch" + std::to_string(c + 1) : label);
                        opened.units.push_back(channel.empty() ? std::string() : channel.child_value("unit"));
                        if (!channel.empty()) { channel = channel.next_sibling("channel"); }
                    }
                }
                catch (const std::exception&)
                {
                    opened.labels.clear();
                    opened.units.assign(size_t(inlet->info.channels), "");
                    for (int c = 0; c < inlet->info.channels; ++c) { opened.labels.push_back("Ch" + std::to_string(c + 1)); }
                }
                inlet->ring = std::make_shared<CTraceBuffer>();
                inlet->ring->setWindowSeconds(m_windowSeconds);
                inlet->ring->setTimeOrigin(m_origin);
                inlet->ring->reconfigure(size_t(inlet->info.channels), inlet->info.rate);
                opened.ring = inlet->ring;
                std::lock_guard<std::mutex> lock(m_mutex);
                m_opened.push_back(std::move(opened));
            }
            m_inlets.emplace(uid, std::move(inlet));
        }

        bool any = false;
        for (auto& [uid, inlet] : m_inlets)
        {
            try { any = (inlet->ring ? readData(*inlet) : readMarkers(*inlet)) || any; }
            catch (const std::exception& ex)
            {
                // A stream that went away: the inlet recovers by itself when it comes back
                std::cout << "[LSL display] " << inlet->info.name << ": " << ex.what() << std::endl;
            }
        }
        if (!any) { std::this_thread::sleep_for(IDLE_SLEEP); }
    }
}

bool CLSLReader::readData(SInlet& inlet)
{
    const size_t channels = size_t(inlet.info.channels);
    if (channels == 0) { return false; }
    // Room for half a second at a time; a longer backlog takes several pulls
    const size_t room = std::max<size_t>(64, size_t(inlet.info.rate / 2.0));
    inlet.samples.resize(room * channels);
    inlet.stamps.resize(room);

    const size_t elements = inlet.inlet->pull_chunk_multiplexed(inlet.samples.data(), inlet.stamps.data(), inlet.samples.size(), inlet.stamps.size(), 0.0);
    const size_t count    = elements / channels;
    if (count == 0) { return false; }

    inlet.channelMajor.resize(count * channels);
    for (size_t s = 0; s < count; ++s)
    {
        for (size_t c = 0; c < channels; ++c) { inlet.channelMajor[c * count + s] = inlet.samples[s * channels + c]; }
    }
    inlet.ring->appendTimed(inlet.channelMajor.data(), channels, count, inlet.stamps[0]);
    return true;
}

bool CLSLReader::readMarkers(SInlet& inlet)
{
    std::vector<SLSLMarker> markers;
    if (inlet.info.text)
    {
        std::vector<std::string> sample(size_t(std::max(1, inlet.info.channels)));
        for (double t = inlet.inlet->pull_sample(sample, 0.0); t != 0.0; t = inlet.inlet->pull_sample(sample, 0.0))
        {
            markers.push_back({t, sample[0], uint64_t(std::hash<std::string>{}(sample[0]))});
        }
    }
    else
    {
        // A numeric marker stream: its first channel is the code
        std::vector<double> sample(size_t(std::max(1, inlet.info.channels)));
        for (double t = inlet.inlet->pull_sample(sample, 0.0); t != 0.0; t = inlet.inlet->pull_sample(sample, 0.0))
        {
            markers.push_back({t, numberText(sample[0]), uint64_t(int64_t(sample[0]))});
        }
    }
    if (markers.empty()) { return false; }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_markers.insert(m_markers.end(), markers.begin(), markers.end());
    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
