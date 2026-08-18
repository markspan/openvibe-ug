/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "LSLOutputWriter.h"

#include <random>
#include <sstream>
#include <iomanip>

static std::string generateStreamID()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << dis(gen);
    return oss.str();
}

LSLOutputWriter::LSLOutputWriter(const std::string& writerName) :
    AbstractWriter(writerName),
    m_signalStreamName("LSL Signal StreamName", "LSL signal stream name", "OpenViBESignal"),
    m_markerStreamName("LSL Marker StreamName", "LSL marker stream name", "OpenViBEMarkers")
{
    m_signalStreamID = generateStreamID();
    do {
        m_markerStreamID = generateStreamID();
    } while (m_markerStreamID == m_signalStreamID);

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

void LSLOutputWriter::onAcquisitionSetup(uint32_t sampleRate, const std::vector<std::string>& channelNames)
{
    destroyOutlets();

    if (sampleRate != 0 && !channelNames.empty() && m_chunkSamplesCount != 0) {

        m_sampleRate   = sampleRate;
        m_channelNames = channelNames;

        lsl::stream_info signalInfo(m_signalStreamName, "signal", int(channelNames.size()),
                                    double(sampleRate), lsl::cf_float32, m_signalStreamID);
        lsl::xml_element channels = signalInfo.desc().append_child("channels");
        for (const auto& name : channelNames)
        {
            channels.append_child("channel")
                    .append_child_value("label", name)
                    .append_child_value("unit", "unknown")
                    .append_child_value("type", "signal");
        }
        m_signalOutlet = new lsl::stream_outlet(signalInfo, int(m_chunkSamplesCount));

        lsl::stream_info markerInfo(m_markerStreamName, "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_int32, m_markerStreamID);
        markerInfo.desc().append_child("channels")
                .append_child("channel")
                .append_child_value("label", "Stimulations")
                .append_child_value("type", "marker");
        m_stimulusOutlet = new lsl::stream_outlet(markerInfo);
    }
}

size_t LSLOutputWriter::send(ovMsg* msg, uint64_t /*clientId*/)
{
    if (msg && m_signalOutlet && msg->msgType == ovMsg::OV_MSG_SAMPLES) {

        const auto& s = msg->msgData.samplesStruct;

        if (m_signalOutlet->have_consumers() && s.count > 0)
        {
            const float* samples = s.samples;
            for (uint32_t i = 0; i < s.count; ++i)
            {
                // Timestamps are left to LSL (push_sample with no timestamp argument uses lsl::local_clock()).
                // Mapping acquisition-server clock to LSL clock is non-trivial and can be added later.
                m_signalOutlet->push_sample(samples + i * m_channelCount);
            }
        }

        if (m_stimulusOutlet && m_stimulusOutlet->have_consumers() && s.stimCount > 0)
        {
            for (uint32_t i = 0; i < s.stimCount; ++i)
            {
                int code = int(s.stimulations[i].id);
                m_stimulusOutlet->push_sample(&code);
            }
        }

        return size_t(s.count);
    }
}

void LSLOutputWriter::destroyOutlets()
{
    delete m_signalOutlet;
    m_signalOutlet = nullptr;
    delete m_stimulusOutlet;
    m_stimulusOutlet = nullptr;
}
