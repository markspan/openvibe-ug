/**-------------------------------------------------------------------------------------------------
 * \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
 * Rights transferred to Inria, contract signed 21.11.2014
 *-------------------------------------------------------------------------------------------------
 */

#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>
#include <CharmeleonMontage.h>

#include <openvibe/acquisition-messaging/samples.h>
#include <openvibe/parameters/parameterBool.h>
#include <openvibe/parameters/parameterListString.h>
#include <openvibe/parameters/parameterPath.h>
#include <openvibe/parameters/parameterString.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <vector>

struct TMSiApi;

/**
 * \class CDriverTMSi
 * \author Mensia Technologies
 *
 * TMSi amplifiers (Refa, Porti, Mobita...) through TMSiSDK.dll, ported from the legacy ovasIDriver/GTK driver.
 *
 * UG fork: new in this fork; upstream OpenViBE 4.0 has no TMSi driver. Compared with the 3.x driver it was ported
 * from: no channel count or masks to set, the channel model below, Charmeleon montages and impedances, one trigger
 * channel with per-sample trigger times, and errors shown in the server (AbstractDriver::note()).
 *
 * The channel names list has one entry per device channel, read from the amplifier, and only channels with a name
 * are recorded, in device order. EXG channels start out with their device name. A Charmeleon montage, applied by
 * the rules in CharmeleonMontage, names them instead: its HardwareChannel n is the n-th EXG channel, and EXG
 * channels it leaves out are not recorded; it can also name BIP and AUX channels ("Channel": "BIP1"). The entries
 * a montage decides are shown read-only (channelLocks()), and what it did is shown under the settings (note()).
 * Without a device connected the montage still names the list, provisionally, on the layout a Refa has: EXG channels,
 * then 4 BIP and 4 AUX channels (assumedLayout()).
 * All other channels (BIP, AUX, DIG, SAW, ...) start out empty: giving one a name records it under that name.
 *
 * Triggers come from one channel only, the trigger channel, never from the SAW ramp channel, which the SDK may
 * also type as digital. They are its low 8 bits, sent as OVTK_StimulationId_Label(value), i.e. 0x8101 to 0x81FF,
 * without inversion. Nothing else is sent as a stimulation.
 */
class CDriverTMSi final : public AbstractDriver
{
public:
    CDriverTMSi();
    ~CDriverTMSi() override;

    bool init() override;
    bool unInit() override;

    bool start() override;
    bool stop() override;

    int read(sampleType* data, uint64_t size) override;
    int readStimulations(ovStimulation* buff, uint32_t maxCount) override;
    void getData(samplesBuffer* buff) override;

    bool usesCharmeleonForImpedance() const override { return true; }
    /// Makes Charmeleon open this driver's TMSi device, on its protocol.
    std::vector<std::string> charmeleonArguments() const override;
    /// The montage file, found as locateCharmeleonMontage() finds it, so that Charmeleon opens the file the driver reads.
    std::string charmeleonMontage() const override;
    std::string montageSetting() const override { return m_paramMontage.value(); }
    /// Reads the montage again, under its new name if Charmeleon saved it under another one.
    void takeOverCharmeleonMontage(const std::string& path) override;

    void setChunkSize(uint32_t chunkSize) override { m_nSamplePerSentBlock = chunkSize; }

    /// The fill level of the SDK's sample buffer (GetBufferInfo), and the samples lost when it overflowed.
    DeviceBuffer takeDeviceBuffer() override;

    /// BIP1, BIP2, AUX1, ... for the bipolar and auxiliary channels, the device name for the other non-EEG channels.
    std::vector<std::string> channelHints() const override;

    OpenViBE::Parameters::Parameters getParameters() override;

private:
    struct PendingTrigger
    {
        uint64_t id;
        uint64_t sampleIndex; ///< Index of the sample in the block returned by the current read().
    };

    struct DeviceChannel
    {
        std::string name;
        uint32_t type; ///< One of the SDK's CHANNELTYPE_ values.
    };

    bool loadLibrary();
    /// (Re)initializes the SDK on the selected protocol and lists the devices it finds.
    bool initLibrary();
    void exitLibrary();
    /// The selected device, or the first one found when it is not connected; empty if there is none.
    std::string selectedDevice() const;
    /// Reads the selected device's channels into m_layout, opening it briefly, unless they are known already.
    bool queryLayout();
    /// Gives the channel names list one entry per device channel, see the class description.
    bool fitNamesToLayout();
    /// The montage kind of each channel in m_layout.
    std::vector<MontageChannelKind> layoutKinds() const;
    /**
     * @brief The channel layout assumed while no device is connected: EXG channels, then 4 BIP and 4 AUX
     *        channels, as a Refa lists them.
     *
     * UG fork: so that a montage's BIP and AUX channels, and the names typed for them, have a place before the
     * device is seen. A list of 8 channels or fewer is taken to be EXG only.
     */
    static std::vector<MontageChannelKind> assumedLayout(size_t channelCount);
    /**
     * @brief Applies the montage, if one is set, to @p names, and shows the outcome: a note and the locked entries.
     * @return False when the montage is set but can not be used; @p names is then left as it was.
     */
    bool applyMontage(std::vector<std::string>& names);
    /// Applies a newly chosen montage to the channel names list, or restores the device names when it is cleared.
    void onMontageChanged();
    bool openFrontEnd(const std::string& deviceID);
    void closeFrontEnd();
    bool readSignalFormat();
    bool setSignalBuffer(uint32_t sampling);
    /// Reports an SDK failure, with the SDK's error code and message, see report().
    void printError(const std::string& what) const;
    /// Prints @p message and keeps it as the reason the last attempt to start failed.
    void report(const std::string& message) const;
    /**
     * @brief Ends a failed init(): reports @p message, if any, and shows why Play failed under the settings.
     * @return False, for init() to return.
     */
    bool failInit(const std::string& message = {});

    std::string channelName(size_t index) const;
    std::string channelType(size_t index) const;

    std::unique_ptr<TMSiApi> m_api;

    OpenViBE::Parameters::ParameterListString m_paramProtocol;
    OpenViBE::Parameters::ParameterListString m_paramDevice;
    OpenViBE::Parameters::ParameterString m_paramTriggerChannel;
    OpenViBE::Parameters::ParameterBool m_paramCommonModeRejection;
    OpenViBE::Parameters::ParameterPath m_paramMontage;

    std::vector<std::string> m_devices;
    bool m_opened  = false;
    bool m_started = false;

    uint32_t m_nSamplePerSentBlock = 0;
    /// The full channel names list, one entry per device channel, while only the recorded ones are shown. Restored in unInit().
    std::vector<std::string> m_configuredNames;
    std::vector<bool> m_configuredLocks; ///< The channel locks that go with m_configuredNames.
    Note m_montageNote;                  ///< What the montage did, shown again once an acquisition has started.
    mutable std::string m_lastError;     ///< Why the last attempt to start failed, see report().

    // Device signal format
    std::vector<DeviceChannel> m_layout;
    std::string m_layoutDevice; ///< Device m_layout was read from.
    uint32_t m_nDeviceChannel = 0;
    std::vector<bool> m_isChannelActivated;
    int m_triggerChannel        = -1; ///< Device channel carrying the triggers, -1 if none.
    uint32_t m_lastTriggerValue = 0;

    /**
     * @brief Records how full the SDK's buffer is, at most every 20 ms, just before GetSamples empties it.
     *
     * UG fork: for takeDeviceBuffer(). Runs on the reader thread.
     */
    void noteBufferLevel();

    // UG fork: the SDK buffer state, written on the reader thread and read by takeDeviceBuffer()
    std::atomic<bool> m_bufferReported{false};     ///< Set once the acquisition runs
    std::atomic<uint32_t> m_bufferSamples{0};       ///< Size of the SDK's buffer, in samples (SetSignalBuffer)
    std::atomic<uint32_t> m_bufferRate{0};          ///< Sampling rate, in Hz
    std::atomic<uint32_t> m_bufferPeakPercent{0};   ///< Fullest the buffer was since the previous takeDeviceBuffer()
    std::atomic<uint32_t> m_bufferOverflows{0};     ///< Times the buffer overflowed, losing its contents, since Start
    std::atomic<int64_t> m_lastReadNs{0};           ///< When GetSamples was last called, on the steady clock
    std::chrono::steady_clock::time_point m_lastBufferInfo;  ///< When GetBufferInfo was last called

    // Raw device buffer, holding what GetSamples returned and read() has not consumed yet
    std::vector<unsigned long> m_rawBuffer;
    uint32_t m_rawSampleCount = 0;
    uint32_t m_rawSampleIndex = 0;

    uint64_t m_lastSampleTime = 0; ///< Timestamp given to the last sample handed over, see getData().
    std::vector<PendingTrigger> m_pendingTriggers;
    std::deque<ovStimulation> m_stimQueue;
};


REGISTER_DRIVER(CDriverTMSi, "TMSi amplifiers", false);
