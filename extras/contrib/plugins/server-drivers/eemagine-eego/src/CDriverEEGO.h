/**
 * SPDX-FileCopyrightText: Inria / eemagine GmbH
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the eego acquisition driver originally written by Steffen Heimes (eemagine GmbH).
 */
#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

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
 * Channels are picked with two bit masks -- one over the referential (EEG) channels, one over the
 * bipolar (BIP) ones -- and the stream always carries a trigger and a sample-counter channel on top.
 * Alternatively a Charmeleon montage picks and names them: HardwareChannel counts the referential
 * channels first, then the bipolar ones, and channels the montage does not name are not recorded.
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
    bool startImpedanceCheck() override;
    int  readImpedances(double* values, size_t count) override;
    bool stopImpedanceCheck() override;

    void setChunkSize(uint32_t chunkSize) override;

private:
    /** Number of status channels the stream appends to the selected EEG/BIP ones: trigger + counter. */
    static const int STATUS_CHANNEL_COUNT = 2;

    /**
     * \brief Parses a channel mask written as "0x..", "0b..", "0.." (octal) or plain decimal.
     *
     * Strict on purpose: a single character the base does not accept is an error rather than a
     * silently truncated mask, which is what strtoull on its own would give. Ported from the
     * retired CHeaderEEGO.
     *
     * \return true when \p str was fully parsed, false otherwise (\p out is then 0).
     */
    static bool convertMask(const std::string& str, uint64_t& out);

    /** Absolute path to eego-SDK.dll next to the host executable, or the bare name as a fallback. */
    static std::string sdkLibraryPath();

    /** Lazily builds the SDK factory. Throws whatever the SDK throws when the library is missing. */
    eemagine::sdk::factory& factory();

    uint64_t refMask() const;
    uint64_t bipMask() const;

    /**
     * Reads the montage parameter, if set, into m_montage and the masks it selects. Needs the
     * amplifier, whose channel list tells how many referential channels precede the bipolar ones.
     */
    bool loadMontage();

    /** Fills m_channelCount / m_channelNames from the amplifier's channel list. No stream needed. */
    bool buildChannelNames();

    /** Copies one stream buffer into m_pending. Returns the number of samples fetched. */
    size_t fetchPending();

    // Configurable parameters
    OpenViBE::Parameters::ParameterListString m_paramSamplingRate;
    OpenViBE::Parameters::ParameterListString m_paramEEGRange; ///< [mV]
    OpenViBE::Parameters::ParameterListString m_paramBIPRange; ///< [mV]
    OpenViBE::Parameters::ParameterString     m_paramEEGMask;
    OpenViBE::Parameters::ParameterString     m_paramBIPMask;
    OpenViBE::Parameters::ParameterPath       m_paramMontage;

    std::map<int, std::string> m_montage; ///< HardwareChannel (1-based) -> name, empty without a montage.
    uint64_t m_montageRefMask = 0;
    uint64_t m_montageBipMask = 0;

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
    /// Triggers found by the current read(): stimulation id and index of their sample in the block.
    std::vector<std::pair<uint64_t, size_t>> m_pendingTriggers;
    std::deque<ovStimulation> m_stimQueue;
};

// REGISTER_DRIVER lives in CDriverEEGO.cpp: in plugin builds it defines the exported
// getInstance() factory, which must exist in exactly one translation unit.
