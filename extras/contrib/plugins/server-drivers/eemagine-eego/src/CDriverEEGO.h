/**
 * SPDX-FileCopyrightText: Inria / eemagine GmbH
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the eego acquisition driver originally written by Steffen Heimes (eemagine GmbH).
 *
 * UG fork: upstream ships this driver's source but does not build it; this fork builds it, and reworked it: the
 * channel list is read from the amplifier instead of the EEGMask/BIPMask settings, Charmeleon montages name and
 * select the channels, Charmeleon checks impedances, every sample is timestamped, and errors are shown in the server.
 */
#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>
#include <CharmeleonMontage.h>

#include <openvibe/parameters/parameterListString.h>
#include <openvibe/parameters/parameterPath.h>

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
 * The amplifier's channels are called EEG1..EEGn (referential) and BIP1..BIPm (bipolar). The channel
 * names list has one entry per amplifier channel, read from the amplifier, and only channels with a name
 * are recorded, in amplifier order. EEG channels start out with their name (EEG1, ...). A Charmeleon
 * montage, applied by the rules in CharmeleonMontage, names them instead: its HardwareChannel n is the n-th
 * referential channel, and EEG channels it leaves out are not recorded; it can also name bipolar channels
 * ("Channel": "BIP1"). On eego the cap fixes which electrode is on which referential channel, so a montage
 * mostly mirrors the cap. Bipolar channels start out empty: giving one a name records it under that name.
 * The stream always carries a trigger and a sample-counter channel on top.
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
    /// Makes Charmeleon open an eego amplifier, as this driver does.
    std::vector<std::string> charmeleonArguments() const override;
    /// The montage file, found as locateCharmeleonMontage() finds it, so that Charmeleon opens the file the driver reads.
    std::string charmeleonMontage() const override;
    /// Reads the montage again, under its new name if Charmeleon saved it under another one.
    void takeOverCharmeleonMontage(const std::string& path) override;
    bool wantsDriftCorrection() const override { return true; }
    bool startImpedanceCheck() override;
    int  readImpedances(double* values, size_t count) override;
    bool stopImpedanceCheck() override;

    void setChunkSize(uint32_t chunkSize) override;

    /// BIP1, BIP2, ... for the bipolar channels.
    std::vector<std::string> channelHints() const override;

    OpenViBE::Parameters::Parameters getParameters() override;

private:
    /** Number of status channels the stream appends to the selected EEG/BIP ones: trigger + counter. */
    static const int STATUS_CHANNEL_COUNT = 2;

    /** Absolute path to eego-SDK.dll next to the host executable, or the bare name as a fallback. */
    static std::string sdkLibraryPath();

    /** Lazily builds the SDK factory. Throws whatever the SDK throws when the library is missing. */
    eemagine::sdk::factory& factory();

    struct DeviceChannel
    {
        unsigned int index; ///< SDK channel index.
        int type;           ///< SDK channel type, reference or bipolar.
        std::string name;   ///< EEG<n> or BIP<n>: eego channels carry a type and an index but no name.
    };

    /** Reads the referential and bipolar channels of \p amplifier into m_layout. No stream needed. */
    void readLayout(eemagine::sdk::amplifier& amplifier);
    /** Reads m_layout from the amplifier, acquiring it briefly, unless it is known already. */
    bool queryLayout();
    /** Gives the channel names list one entry per amplifier channel, see the class description. */
    bool fitNamesToLayout();
    /** The montage kind of each channel in m_layout. */
    std::vector<MontageChannelKind> layoutKinds() const;
    /**
     * Applies the montage, if one is set, to \p names, and shows the outcome: a note and the locked entries.
     * Returns false when the montage is set but can not be used; \p names is then left as it was.
     */
    bool applyMontage(std::vector<std::string>& names);
    /** Applies a newly chosen montage to the channel names list, or restores the amplifier names when it is cleared. */
    void onMontageChanged();
    /** Prints \p message and keeps it as the reason the last attempt to start failed. */
    void report(const std::string& message);
    /** Ends a failed init(): reports \p message, if any, and shows why Play failed under the settings. Returns false. */
    bool failInit(const std::string& message = {});
    /**
     * Reads the channel list from the acquired amplifier and picks the channels with a name, into
     * m_selectedChannels; m_channelCount / m_channelNames then describe those, plus the status channels.
     */
    bool selectChannels();

    /** Copies one stream buffer into m_pending. Returns the number of samples fetched. */
    size_t fetchPending();

    // Configurable parameters
    OpenViBE::Parameters::ParameterListString m_paramSamplingRate;
    OpenViBE::Parameters::ParameterListString m_paramEEGRange; ///< [mV]
    OpenViBE::Parameters::ParameterListString m_paramBIPRange; ///< [mV]
    OpenViBE::Parameters::ParameterPath       m_paramMontage;

    /// Referential then bipolar channels of the amplifier, as it lists them.
    std::vector<DeviceChannel> m_layout;
    /// The full channel names list, one entry per amplifier channel, while only the recorded ones are shown. Restored in unInit().
    std::vector<std::string> m_configuredNames;
    /// The channel locks that go with m_configuredNames.
    std::vector<bool> m_configuredLocks;
    /// What the montage did, shown again once an acquisition has started.
    Note m_montageNote;
    /// Why the last attempt to start failed, see report().
    std::string m_lastError;
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
