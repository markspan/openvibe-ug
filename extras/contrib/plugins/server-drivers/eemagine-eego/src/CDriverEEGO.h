/**
 * SPDX-FileCopyrightText: Inria / eemagine GmbH
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the eego acquisition driver originally written by Steffen Heimes (eemagine GmbH).
 */
#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

#include <openvibe/parameters/parameterInt.h>
#include <openvibe/parameters/parameterListString.h>
#include <openvibe/parameters/parameterPath.h>
#include <openvibe/parameters/parameterString.h>

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

// The SDK headers are heavy and pull in Windows.h, so keep them out of this header.
namespace eemagine {
namespace sdk {
class amplifier;
class factory;
class stream;
}
}

/**
 * \brief Acquisition driver for eemagine eego amplifiers.
 *
 * The amplifier's channels are called EEG1..EEGn (referential) and BIP1..BIPm (bipolar). The first
 * "EEG channel count" referential channels are recorded, or the channels a Charmeleon montage names
 * (HardwareChannel counts the referential channels first, then the bipolar ones). Further channels are
 * added by name, optionally relabelled ("BIP1=Photodiode"). The stream always carries a trigger and a
 * sample-counter channel on top.
 * The trigger channel is published as a signal channel *and* flank-decoded into stimulations, as it
 * was before the port.
 *
 * The SDK allows at most one stream alive per amplifier, which shapes the lifecycle: init() only
 * acquires the amplifier, and the EEG or impedance stream is opened by start() / startImpedanceCheck().
 */
class CDriverEEGO final : public AbstractDriver
{
public:
    CDriverEEGO();
    /** Out of line: the unique_ptr members hold types that are incomplete in this header. */
    ~CDriverEEGO() override;

    bool init() override;
    bool unInit() override;
    bool start() override;
    bool stop() override;

    int read(sampleType* data, uint64_t size) override;
    int readStimulations(ovStimulation* buff, uint32_t maxCount) override;
    void getData(samplesBuffer* buff) override;

    bool supportsImpedanceCheck() const override { return true; }
    bool usesCharmeleonForImpedance() const override { return true; }
    bool wantsDriftCorrection() const override { return true; }
    bool startImpedanceCheck() override;
    int  readImpedances(double* values, size_t count) override;
    bool stopImpedanceCheck() override;

    void setChunkSize(uint32_t chunkSize) override;

private:
    /** Number of status channels the stream appends to the selected EEG/BIP ones: trigger + counter. */
    static const int STATUS_CHANNEL_COUNT = 2;

    /** Absolute path to eego-SDK.dll next to the host executable, or the bare name as a fallback. */
    static std::string sdkLibraryPath();

    /** Lazily builds the SDK factory. Throws whatever the SDK throws when the library is missing. */
    eemagine::sdk::factory& factory();

    /**
     * Picks the channels to record from the amplifier's channel list, into m_selectedChannels, and
     * fills m_channelCount / m_channelNames. No stream needed.
     */
    bool selectChannels();

    /** Copies one stream buffer into m_pending. Returns the number of samples fetched. */
    size_t fetchPending();

    // Configurable parameters
    OpenViBE::Parameters::ParameterListString m_paramSamplingRate;
    OpenViBE::Parameters::ParameterListString m_paramEEGRange; ///< [mV]
    OpenViBE::Parameters::ParameterListString m_paramBIPRange; ///< [mV]
    OpenViBE::Parameters::ParameterInt        m_paramEEGChannelCount;
    OpenViBE::Parameters::ParameterPath       m_paramMontage;
    OpenViBE::Parameters::ParameterString     m_paramAdditionalChannels;

    /// Channels to record, as SDK channel index and type, in amplifier order.
    std::vector<std::pair<unsigned int, int>> m_selectedChannels;

    std::unique_ptr<eemagine::sdk::factory>   m_factory;
    std::unique_ptr<eemagine::sdk::amplifier> m_amplifier;
    std::unique_ptr<eemagine::sdk::stream>    m_stream;

    uint32_t m_chunkSize = 32;

    /**
     * Bipolar range actually handed to OpenEegStream, in volt. Differs from m_paramBIPRange only
     * on EE-511 amplifiers, which constrain it to 2.5x the EEG range.
     */
    double m_effectiveBipRange = 0.0;

    // Samples fetched from the stream but not yet handed over, sample-major.
    std::vector<double> m_pending;
    size_t              m_pendingChannels = 0;
    size_t              m_pendingSamples  = 0;
    size_t              m_pendingOffset   = 0; ///< Index of the next sample to hand out.

    int      m_triggerChannel   = -1; ///< Index of the trigger channel in a scan, -1 if absent.
    uint32_t m_lastTriggerValue = 0;  ///< Previous trigger word, for rising-edge detection.
    uint64_t m_lastSampleTime   = 0;  ///< Timestamp given to the last sample handed over, see getData().
    /// Triggers found by the current read(): stimulation id and index of their sample in the block.
    std::vector<std::pair<uint64_t, size_t>> m_pendingTriggers;
    std::deque<ovStimulation> m_stimQueue;
};

// REGISTER_DRIVER lives in CDriverEEGO.cpp: in plugin builds it defines the exported
// getInstance() factory, which must exist in exactly one translation unit.
