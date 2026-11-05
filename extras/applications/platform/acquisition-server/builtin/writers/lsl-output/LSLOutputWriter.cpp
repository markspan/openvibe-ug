/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "LSLOutputWriter.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <locale>
#include <sstream>

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace {

// UG fork: the montage gives positions on a unit sphere, XDF asks for millimetres. No head was measured, so this is the usual
// assumption of a head with this radius; the directions are what the positions really say.
constexpr double HEAD_RADIUS_MM = 85.0;

std::string computerName()
{
#if defined(_WIN32)
    const char* name = std::getenv("COMPUTERNAME");
    return name ? name : "";
#else
    char name[256] = {};
    return gethostname(name, sizeof(name) - 1) == 0 ? name : "";
#endif
}

// UG fork: upstream draws a random ID at every start. LSL recovers a lost stream by its ID, so a client could not pick up the
// stream again after a restart of the server. The computer's name is in it, as two lab PCs can use the same stream names.
std::string streamId(const std::string& streamName) { return "openvibe-acquisition-server:" + computerName() + ":" + streamName; }

// The same clock as AbstractDriver::timestamp(), which dates the samples and the stimulations
double serverClockSeconds() { return 1e-9 * double(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }

// UG fork: what to add to the server's clock to get LSL's. Neither clock is adjusted while they run, so this holds for the whole
// session. Of a few readings the one with the shortest time between its reads, in case the thread is interrupted during one.
double measureClockOffset()
{
    double offset  = 0.0;
    double shortest = std::numeric_limits<double>::max();
    for (int i = 0; i < 10; ++i) {
        const double before = serverClockSeconds();
        const double lsl    = lsl::local_clock();
        const double after  = serverClockSeconds();
        if (after - before < shortest) {
            shortest = after - before;
            offset   = lsl - (before + after) / 2.0;
        }
    }
    return offset;
}

std::string formatNumber(const double value)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text.precision(6);
    text << std::fixed << value;
    return text.str();
}

// UG fork: XDF's frame is X to the right, Y to the front, Z to the top. Theta runs from the vertex, phi from the right ear towards
// the nose (ovChannelInfo.h), which is the same frame.
void appendLocation(lsl::xml_element channel, const ovChannelInfo::Position& position)
{
    const double degrees = std::acos(-1.0) / 180.0;
    const double theta   = position.theta * degrees;
    const double phi     = position.phi * degrees;
    channel.append_child("location")
           .append_child_value("X", formatNumber(HEAD_RADIUS_MM * std::sin(theta) * std::cos(phi)))
           .append_child_value("Y", formatNumber(HEAD_RADIUS_MM * std::sin(theta) * std::sin(phi)))
           .append_child_value("Z", formatNumber(HEAD_RADIUS_MM * std::cos(theta)));
}

}  // namespace

LSLOutputWriter::LSLOutputWriter(const std::string& writerName) :
    AbstractWriter(writerName),
    m_signalStreamName("LSL Signal StreamName", "LSL signal stream name", "OpenViBESignal"),
    m_markerStreamName("LSL Marker StreamName", "LSL marker stream name", "OpenViBEMarkers")
{
    m_parameters.push_back(&m_signalStreamName);
    m_parameters.push_back(&m_markerStreamName);
}

LSLOutputWriter::~LSLOutputWriter()
{
    destroyOutlets();
}

void LSLOutputWriter::uninit()
{
    destroyOutlets();
}

void LSLOutputWriter::onAcquisitionSetup(uint32_t sampleRate, const ovChannelInfo& channels)
{
    destroyOutlets();

    if (sampleRate != 0 && m_channelCount != 0 && m_chunkSamplesCount != 0) {

        m_clockOffset = measureClockOffset();

        // UG fork: the type is "EEG", which is what clients search for and XDF names, not "signal". A channel only says what is
        // known of it: its type comes from the montage, as a driver doesn't tell an EEG channel from a bipolar one. There is no
        // unit, as the drivers differ: eego's SDK delivers volts, and the others are not verified.
        lsl::stream_info signalInfo(m_signalStreamName, "EEG", int(m_channelCount), double(sampleRate), lsl::cf_float32, streamId(m_signalStreamName));
        lsl::xml_element channelList = signalInfo.desc().append_child("channels");
        const bool hasMontage = !channels.montage.empty();
        for (size_t i = 0; i < m_channelCount; ++i)
        {
            lsl::xml_element channel = channelList.append_child("channel");
            channel.append_child_value("label", i < channels.names.size() ? channels.names[i] : std::string());
            if (hasMontage) {
                const bool onHead = i < channels.positions.size() && channels.positions[i].known;
                channel.append_child_value("type", onHead ? "EEG" : "AUX");
                if (onHead) { appendLocation(channel, channels.positions[i]); }
            }
        }
        if (hasMontage) { signalInfo.desc().append_child("cap").append_child_value("name", channels.montage); }
        m_signalOutlet = new lsl::stream_outlet(signalInfo, int(m_chunkSamplesCount));

        lsl::stream_info markerInfo(m_markerStreamName, "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_int32, streamId(m_markerStreamName));
        markerInfo.desc().append_child("channels")
                .append_child("channel")
                .append_child_value("label", "Stimulations")
                .append_child_value("type", "marker");
        m_stimulusOutlet = new lsl::stream_outlet(markerInfo);
    }
}

size_t LSLOutputWriter::send(ovMsg* msg, uint64_t /*clientId*/)
{
    // UG fork: returns 0 for the acquisition configuration, which LSL carries in the stream's metadata. Upstream returns nothing.
    if (!msg || !m_signalOutlet || msg->msgType != ovMsg::OV_MSG_SAMPLES) { return 0; }

    const auto& s = msg->msgData.samplesStruct;

    // UG fork: samples and markers keep the times the server gave them, so that a marker stays on its own sample, however late
    // it came and whatever the size of a chunk. LSL carries these times with the data. Upstream uses the time of this call for
    // both, which puts all samples of a chunk at one time and a marker at the end of its chunk.
    if (m_signalOutlet->have_consumers() && s.count > 0)
    {
        m_timestamps.resize(s.count);
        for (uint32_t i = 0; i < s.count; ++i) { m_timestamps[i] = 1e-9 * double(s.timestamp[i]) + m_clockOffset; }
        m_signalOutlet->push_chunk_multiplexed(s.samples, m_timestamps.data(), s.count * m_channelCount);
    }

    if (m_stimulusOutlet && m_stimulusOutlet->have_consumers() && s.stimCount > 0)
    {
        for (uint32_t i = 0; i < s.stimCount; ++i)
        {
            const int code = int(s.stimulations[i].id);
            m_stimulusOutlet->push_sample(&code, 1e-9 * double(s.stimulations[i].date) + m_clockOffset);
        }
    }

    return size_t(s.count);
}

void LSLOutputWriter::destroyOutlets()
{
    delete m_signalOutlet;
    m_signalOutlet = nullptr;
    delete m_stimulusOutlet;
    m_stimulusOutlet = nullptr;
}

// UG fork: here and not in the header, as a second file that includes the header would register the writer again
REGISTER_WRITER(LSLOutputWriter, "LSL Output");
