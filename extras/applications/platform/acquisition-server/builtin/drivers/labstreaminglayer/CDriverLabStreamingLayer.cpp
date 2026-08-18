// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CDriverLabStreamingLayer.h"

#include <cmath>
#include <cstdint>
#include <iostream>

namespace {
// In seconds
constexpr double LSL_SAMPLING_ESTIMATION_DURATION = 2.0;
constexpr double LSL_RESOLVE_TIMEOUT               = 2.0;
constexpr double LSL_OPEN_TIMEOUT                  = 2.0;
constexpr double LSL_READ_TIMEOUT                  = 2.0;

std::string streamLabel(const lsl::stream_info& stream) { return stream.name() + " / " + stream.source_id(); }

// Recovers the "name" part of a "name / source_id" dropdown label, e.g. to re-resolve a stream
// that fell out of the last discovery pass.
std::string streamNameFromLabel(const std::string& label)
{
    const size_t pos = label.rfind(" / ");
    return pos == std::string::npos ? label : label.substr(0, pos);
}

uint32_t getClosestMultipleOf25(const uint32_t value) { return uint32_t(((value + 12) / 25) * 25); }

uint32_t getClosestPowerOf2(const uint32_t value)
{
    uint32_t bitShiftCount = 0;
    while ((1LL << bitShiftCount) < value) { bitShiftCount++; }

    if (bitShiftCount > 0) {
        const uint32_t greater = uint32_t(1LL << bitShiftCount);
        const uint32_t lesser  = greater >> 1;
        if (greater - value < value - lesser) { return greater; }
        return lesser;
    }

    return uint32_t(1LL << bitShiftCount);
}

// Rounds a roughly-measured sampling rate to whichever of "nearest multiple of 25" or "nearest
// power of 2" is closer, since those cover the vast majority of real EEG amplifier rates.
uint32_t getSmartFallbackSamplingRateEstimate(const uint32_t lslSampling)
{
    const uint32_t closestMultipleOf25     = getClosestMultipleOf25(lslSampling);
    const uint32_t closestPowerOfTwo       = getClosestPowerOf2(lslSampling);
    const uint32_t closestMultipleOf25Diff = uint32_t(std::abs(int(closestMultipleOf25) - int(lslSampling)));
    const uint32_t closestPowerOfTwoDiff   = uint32_t(std::abs(int(closestPowerOfTwo) - int(lslSampling)));
    return closestMultipleOf25Diff < closestPowerOfTwoDiff ? closestMultipleOf25 : closestPowerOfTwo;
}

// Captures ~LSL_SAMPLING_ESTIMATION_DURATION seconds of samples on an already-open inlet and
// estimates the sampling rate from the elapsed LSL time. Returns 0 on timeout/failure.
uint32_t autoDetectSamplingRate(lsl::stream_inlet& inlet, size_t channelCount, std::vector<float>& scratch)
{
    try {
        // Drop anything already buffered so we measure a fresh window.
        while (inlet.samples_available() != 0) { inlet.pull_sample(scratch.data(), int(channelCount), LSL_READ_TIMEOUT); }

        const double startTime = inlet.pull_sample(scratch.data(), int(channelCount), LSL_READ_TIMEOUT);
        if (startTime == 0.0) { return 0; }

        uint32_t nSamples    = 0;
        double currentTime = startTime;
        while (currentTime - startTime < LSL_SAMPLING_ESTIMATION_DURATION) {
            const double t = inlet.pull_sample(scratch.data(), int(channelCount), LSL_READ_TIMEOUT);
            if (t == 0.0) { return 0; }
            ++nSamples;
            currentTime = t;
        }

        const double duration = currentTime - startTime;
        return duration > 0.0 ? getSmartFallbackSamplingRateEstimate(uint32_t(nSamples / duration)) : 0;
    }
    catch (...) {
        std::cout << "[LSL] Failed while auto-detecting sampling rate\n";
        return 0;
    }
}
}  // namespace

CDriverLabStreamingLayer::CDriverLabStreamingLayer()
    : m_paramSignalStream("SignalStream", "Signal stream", {}, ""),
      m_paramMarkerStream("MarkerStream", "Marker stream", {NONE_MARKER_STREAM}, NONE_MARKER_STREAM),
      m_paramFallbackSamplingRate("FallbackSamplingRate", "Fallback sampling rate (0 = auto-detect)", 0, 0, 100000)
{
    m_parameters.push_back(&m_paramSignalStream);
    m_parameters.push_back(&m_paramMarkerStream);
    m_parameters.push_back(&m_paramFallbackSamplingRate);
}

//___________________________________________________________________//
//                                                                   //

OpenViBE::Parameters::Parameters CDriverLabStreamingLayer::getParameters()
{
    resolveStreams();
    return AbstractDriver::getParameters();
}

void CDriverLabStreamingLayer::resolveStreams()
{
    std::vector<lsl::stream_info> streams;
    try { streams = lsl::resolve_streams(1.0); }
    catch (...) {
        std::cout << "[LSL] Failed to resolve streams\n";
        return;
    }

    m_availableSignalStreams.clear();
    m_availableMarkerStreams.clear();

    std::vector<std::string> signalLabels;
    std::vector<std::string> markerLabels{NONE_MARKER_STREAM};

    for (const auto& stream : streams) {
        if (stream.channel_format() == lsl::cf_float32) {
            m_availableSignalStreams.push_back(stream);
            signalLabels.push_back(streamLabel(stream));
        }
        else if (stream.channel_format() == lsl::cf_int32) {
            m_availableMarkerStreams.push_back(stream);
            markerLabels.push_back(streamLabel(stream));
        }
    }

    m_paramSignalStream.setAvailableValues(signalLabels);
    m_paramMarkerStream.setAvailableValues(markerLabels);
}

bool CDriverLabStreamingLayer::findStreamByLabel(const std::string& label, const std::vector<lsl::stream_info>& streams, lsl::stream_info& result) const
{
    for (const auto& stream : streams) {
        if (streamLabel(stream) == label) {
            result = stream;
            return true;
        }
    }
    return false;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverLabStreamingLayer::init()
{
    if (m_paramSignalStream.value().empty()) {
        std::cout << "[LSL] No signal stream selected\n";
        return false;
    }

    if (!findStreamByLabel(m_paramSignalStream.value(), m_availableSignalStreams, m_signalStreamInfo)) {
        // Cached discovery is stale/empty: try a fresh, direct resolve by name.
        const std::string name = streamNameFromLabel(m_paramSignalStream.value());
        const std::vector<lsl::stream_info> found = lsl::resolve_stream("name", name, 1, LSL_RESOLVE_TIMEOUT);
        if (found.empty()) {
            std::cout << "[LSL] Error resolving signal stream [" << name << "]\n";
            return false;
        }
        m_signalStreamInfo = found[0];
    }

    m_hasMarkerStream = m_paramMarkerStream.value() != NONE_MARKER_STREAM;
    if (m_hasMarkerStream) {
        if (!findStreamByLabel(m_paramMarkerStream.value(), m_availableMarkerStreams, m_markerStreamInfo)) {
            const std::string name = streamNameFromLabel(m_paramMarkerStream.value());
            const std::vector<lsl::stream_info> found = lsl::resolve_stream("name", name, 1, LSL_RESOLVE_TIMEOUT);
            if (found.empty()) {
                std::cout << "[LSL] Error resolving marker stream [" << name << "]\n";
                return false;
            }
            m_markerStreamInfo = found[0];
        }
    }

    // Open a temporary inlet to poll channel labels and (if needed) auto-detect the sampling rate.
    // It is closed by going out of scope; the "real" inlet is opened in start().
    lsl::stream_inlet tmpInlet(m_signalStreamInfo, 360, 0, false);
    try { tmpInlet.open_stream(LSL_OPEN_TIMEOUT); }
    catch (...) {
        std::cout << "[LSL] Failed to open signal stream [" << m_signalStreamInfo.name() << "]\n";
        return false;
    }

    lsl::stream_info fullInfo;
    try { fullInfo = tmpInlet.info(LSL_READ_TIMEOUT); }
    catch (...) {
        std::cout << "[LSL] Timed out reading full stream info for [" << m_signalStreamInfo.name() << "]\n";
        return false;
    }

    m_channelCount.setValue(int(fullInfo.channel_count()));
    m_sampleBuffer.resize(m_channelCount.value());

    lsl::xml_element channel = fullInfo.desc().child("channels").child("channel");
    for (int i = 0; i < m_channelCount.value(); ++i) {
        const char* label = channel.child_value("label");
        m_channelNames.setAt(i, (label && label[0]) ? label : "LSL " + std::to_string(i + 1));
        channel = channel.next_sibling("channel");
    }

    uint32_t sampling = 0;
    if (m_paramFallbackSamplingRate.value() != 0) {
        sampling = uint32_t(m_paramFallbackSamplingRate.value());
        std::cout << "[LSL] Using fallback sampling rate [" << sampling << "] hz\n";
    }
    else if (m_signalStreamInfo.nominal_srate() != 0) { sampling = uint32_t(m_signalStreamInfo.nominal_srate()); }
    else {
        std::cout << "[LSL] Stream has no nominal sampling rate, auto-detecting...\n";
        sampling = autoDetectSamplingRate(tmpInlet, size_t(m_channelCount.value()), m_sampleBuffer);
        if (sampling == 0) {
            std::cout << "[LSL] Failed to auto-detect sampling rate\n";
            return false;
        }
        std::cout << "[LSL] Auto-detected sampling rate [" << sampling << "] hz\n";
    }

    m_samplingRate.setValue(int(sampling));

    return true;
}

bool CDriverLabStreamingLayer::unInit()
{
    // Deleting the inlets here (rather than in stop()) matters: Acquisition::stop() calls
    // driver->stop() *before* joining the reader thread, which may still be mid-pull_sample()
    // on m_signalInlet/m_markerInlet at that point. unInit() is only called after the reader
    // thread has been joined, so it is the first point at which it's safe to free them.
    if (m_signalInlet) {
        m_signalInlet->close_stream();
        delete m_signalInlet;
        m_signalInlet = nullptr;
    }

    if (m_markerInlet) {
        m_markerInlet->close_stream();
        delete m_markerInlet;
        m_markerInlet = nullptr;
    }

    m_sampleBuffer.clear();
    return true;
}

bool CDriverLabStreamingLayer::start()
{
    m_signalInlet = new lsl::stream_inlet(m_signalStreamInfo, 360, 0, false);
    try { m_signalInlet->open_stream(LSL_OPEN_TIMEOUT); }
    catch (...) {
        std::cout << "[LSL] Failed to open signal stream [" << m_signalStreamInfo.name() << "]\n";
        delete m_signalInlet;
        m_signalInlet = nullptr;
        return false;
    }

    if (m_hasMarkerStream) {
        m_markerInlet = new lsl::stream_inlet(m_markerStreamInfo, 360, 0, false);
        try { m_markerInlet->open_stream(LSL_OPEN_TIMEOUT); }
        catch (...) {
            std::cout << "[LSL] Failed to open marker stream [" << m_markerStreamInfo.name() << "]\n";
            delete m_markerInlet;
            m_markerInlet = nullptr;
            delete m_signalInlet;
            m_signalInlet = nullptr;
            return false;
        }
    }

    return true;
}

bool CDriverLabStreamingLayer::stop()
{
    // Actual inlet teardown happens in unInit(), which the framework only calls after the
    // reader thread (still calling read()/readStimulations() at this point) has been joined.
    return true;
}

int CDriverLabStreamingLayer::read(sampleType* data, uint64_t size)
{
    if (!m_signalInlet) { return 0; }

    const std::size_t channelCount = std::size_t(m_channelCount.value());
    std::size_t elementsWritten = 0;
    // pull_chunk_multiplexed() writes directly into `data` in the same sample-major layout
    // AbstractDriver expects, and drains whatever's already buffered in one native call
    // instead of one pull_sample() call (and its internal locking) per sample.
    try { elementsWritten = m_signalInlet->pull_chunk_multiplexed(data, nullptr, size * channelCount, 0, 0.0); }
    catch (...) { std::cout << "[LSL] Failed to pull signal chunk\n"; }

    return int(elementsWritten / channelCount);
}

int CDriverLabStreamingLayer::readStimulations(ovStimulation* buff, uint32_t maxCount)
{
    if (!m_markerInlet) { return 0; }

    if (m_markerBuffer.size() < maxCount) { m_markerBuffer.resize(maxCount); }

    std::size_t count = 0;
    try { count = m_markerInlet->pull_chunk_multiplexed(m_markerBuffer.data(), nullptr, maxCount, 0, 0.0); }
    catch (...) { std::cout << "[LSL] Failed to pull marker chunk\n"; }

    for (std::size_t i = 0; i < count; ++i) {
        // date left at 0: auto-stamped by AbstractDriver::getData() using the driver's own clock
        // domain. Mapping the LSL clock domain to the driver's is not attempted, same simplification
        // used by TaggingDriver and LSLOutputWriter for their own clock-domain mismatches.
        buff[i] = ovStimulation{uint64_t(m_markerBuffer[i]), 0, 0};
    }

    return int(count);
}

void CDriverLabStreamingLayer::setChunkSize(uint32_t chunkSize) { m_chunkSize = chunkSize; }
