// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

#include <openvibe/parameters/parameterListString.h>
#include <openvibe/parameters/parameterInt.h>

#include <lsl_cpp.h>

#include <vector>
#include <string>


/**
 * \class CDriverLabStreamingLayer
 * \author Thomas Prampart (INRIA)
 * \brief Acquires a signal stream (and optionally a marker stream) from LabStreamingLayer (LSL).
 *
 * Assumes the signal stream is float32 and the (optional) marker stream is int32, matching the
 * conventions used by the LSL Output writer and the historical LSL driver/box in OpenViBE.
 */
class CDriverLabStreamingLayer final : public AbstractDriver
{
public:
    CDriverLabStreamingLayer();

    bool init() override;
    bool unInit() override;

    bool start() override;
    bool stop() override;

    int read(sampleType* data, uint64_t size) override;
    int readStimulations(ovStimulation* buff, uint32_t maxCount) override;

    void setChunkSize(uint32_t chunkSize) override;

    OpenViBE::Parameters::Parameters getParameters() override;

private:
    static constexpr const char* NONE_MARKER_STREAM = "None";

    void resolveStreams();
    bool findStreamByLabel(const std::string& label, const std::vector<lsl::stream_info>& streams, lsl::stream_info& result) const;

    // Configurable parameters
    OpenViBE::Parameters::ParameterListString m_paramSignalStream;
    OpenViBE::Parameters::ParameterListString m_paramMarkerStream;
    OpenViBE::Parameters::ParameterInt m_paramFallbackSamplingRate;

    // Streams discovered by the last resolveStreams() call, parallel to the dropdown labels above
    std::vector<lsl::stream_info> m_availableSignalStreams;
    std::vector<lsl::stream_info> m_availableMarkerStreams;

    // Streams resolved (bound) by init(), used by start()/stop()/read()
    lsl::stream_info m_signalStreamInfo;
    lsl::stream_info m_markerStreamInfo;
    bool m_hasMarkerStream = false;

    lsl::stream_inlet* m_signalInlet = nullptr;
    lsl::stream_inlet* m_markerInlet = nullptr;

    std::vector<float> m_sampleBuffer; // scratch buffer, one sample's worth of channels (used by init()'s auto-detect)
    std::vector<int32_t> m_markerBuffer; // scratch buffer for readStimulations()'s pull_chunk_multiplexed()

    uint32_t m_chunkSize = 0;
};


REGISTER_DRIVER(CDriverLabStreamingLayer, "LabStreamingLayer (LSL)", false);
