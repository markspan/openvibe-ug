/**-------------------------------------------------------------------------------------------------
 * \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
 * Rights transferred to Inria, contract signed 21.11.2014
 *-------------------------------------------------------------------------------------------------
 */

#pragma once

#include <CharmeleonDriver.h>  // UG fork
#include <AbstractDriverBuilder.h>
#include <CharmeleonMontage.h>
#include <SteadyTrigger.h>  // UG fork

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
 * channel with per-sample trigger times, and errors shown in the server (AbstractDriver::note()). Channels in volt are sent
 * in microvolt, whatever exponent the device gives them; samples out of range are counted and reported.
 *
 * The channel names list and the montage follow the rules of CharmeleonDriver, which this driver shares with the eego
 * driver: one entry per device channel, only channels with a name recorded. EXG channels are the montage's EEG
 * channels and start out with their device name; BIP and AUX channels are the montage's "Channel": "BIP1", ...; all
 * other channels (DIG, SAW, ...) start out empty, and giving one a name records it under that name. Without a device
 * connected, the list is taken to be laid out as a Refa's: EXG channels, then 4 BIP and 4 AUX channels (assumedKinds()).
 *
 * Triggers come from one channel only, the trigger channel, never from the SAW ramp channel, which the SDK may
 * also type as digital. They are its low 8 bits, sent as OVTK_StimulationId_Label(value), i.e. 0x8101 to 0x81FF,
 * without inversion. Nothing else is sent as a stimulation. A value counts once it has held for two samples, and is dated at the
 * first sample where the channel changed, so a value caught while the trigger lines switch is no trigger (SteadyTrigger).
 */
class CDriverTMSi final : public CharmeleonDriver
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

    // UG fork: the impedances, in the SDK's impedance mode, as Charmeleon reads them (Acquisition::measureImpedances())
    bool supportsImpedanceCheck() const override { return true; }
    bool startImpedanceCheck() override;
    int readImpedances(double* values, size_t count) override;
    bool stopImpedanceCheck() override;

    /// Makes Charmeleon open this driver's TMSi device, on its protocol.
    std::vector<std::string> charmeleonArguments() const override;

    void setChunkSize(uint32_t chunkSize) override { m_nSamplePerSentBlock = chunkSize; }

    /// The fill level of the SDK's sample buffer (GetBufferInfo), and the samples lost when it overflowed.
    DeviceBuffer takeDeviceBuffer() override;

    OpenViBE::Parameters::Parameters getParameters() override;

private:
    /// A channel as the SDK describes it.
    struct SdkChannel
    {
        std::string name;
        uint32_t type; ///< One of the SDK's CHANNELTYPE_ values.
    };

    // UG fork: what CharmeleonDriver needs to know of the device
    std::vector<DeviceChannel> deviceChannels() const override;
    /// EXG channels, then 4 BIP and 4 AUX channels, as a Refa lists them; a list of 8 channels or fewer is taken to be EXG only.
    std::vector<MontageChannelKind> assumedKinds(size_t count) const override;
    bool isAcquiring() const override { return m_opened; }

    bool loadLibrary();
    /// (Re)initializes the SDK on the selected protocol and lists the devices it finds.
    bool initLibrary();
    void exitLibrary();
    /// The selected device, or the first one found when it is not connected; empty if there is none.
    std::string selectedDevice() const;
    /// Reads the selected device's channels into m_layout, opening it briefly, unless they are known already.
    bool queryLayout();
    bool openFrontEnd(const std::string& deviceID);
    void closeFrontEnd();
    bool readSignalFormat();
    bool setSignalBuffer(uint32_t sampling);
    /// Reports an SDK failure, with the SDK's error code and message, see report().
    void printError(const std::string& what) const;

    std::string channelName(size_t index) const;
    std::string channelType(size_t index) const;

    std::unique_ptr<TMSiApi> m_api;

    OpenViBE::Parameters::ParameterListString m_paramProtocol;
    OpenViBE::Parameters::ParameterListString m_paramDevice;
    OpenViBE::Parameters::ParameterString m_paramTriggerChannel;
    OpenViBE::Parameters::ParameterBool m_paramCommonModeRejection;

    std::vector<std::string> m_devices;
    bool m_opened  = false;
    bool m_started = false;

    uint32_t m_nSamplePerSentBlock = 0;

    // Device signal format
    std::vector<SdkChannel> m_layout;
    std::string m_layoutDevice; ///< Device m_layout was read from.
    uint32_t m_nDeviceChannel = 0;
    std::vector<bool> m_isChannelActivated;
    std::vector<double> m_channelScale;  ///< UG fork: per device channel, what turns its values into microvolt (1 if not in volt), see init()
    uint64_t m_overflowSamples  = 0;     ///< UG fork: analog samples out of range since Play, recorded as 0
    uint64_t m_overflowReported = 0;     ///< UG fork: how many of those were reported
    std::vector<uint64_t> m_overflowByChannel;  ///< UG fork: per recorded channel, out of range since the last report
    std::chrono::steady_clock::time_point m_lastOverflowReport;  ///< UG fork: when they were last reported, see read()
    /// UG fork: an electrode out of range is reported at most this often
    static constexpr std::chrono::seconds OVERFLOW_REPORT_INTERVAL{10};
    int m_triggerChannel        = -1; ///< Device channel carrying the triggers, -1 if none.
    SteadyTrigger m_trigger;          ///< UG fork: turns the trigger channel into triggers, see getData()

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
    std::atomic<uint32_t> m_bufferPeakPercent{0};   ///< Fullest the buffer was since the previous takeDeviceBuffer(), as GetBufferInfo says
    /// Most samples one GetSamples returned since the previous takeDeviceBuffer(). GetSamples empties the buffer, so this is what
    /// waited in it, to the sample; GetBufferInfo gives whole percents, 100 ms of the 10 s buffer.
    std::atomic<uint32_t> m_peakFetched{0};
    std::atomic<uint32_t> m_bufferOverflows{0};     ///< Times the buffer overflowed, losing its contents, since Start
    std::atomic<int64_t> m_lastReadNs{0};           ///< When GetSamples was last called, on the steady clock
    std::chrono::steady_clock::time_point m_lastBufferInfo;  ///< When GetBufferInfo was last called

    // Raw device buffer, holding what GetSamples returned and read() has not consumed yet
    std::vector<unsigned long> m_rawBuffer;
    uint32_t m_rawSampleCount = 0;
    uint32_t m_rawSampleIndex = 0;
    uint64_t m_blockFetchTime = 0;  ///< UG fork: when GetSamples returned the samples in m_rawBuffer, see getData()
    uint32_t m_readFirst      = 0;  ///< UG fork: index in m_rawBuffer of the first sample the last read() handed over

    uint64_t m_lastSampleTime = 0; ///< Timestamp given to the last sample handed over, see getData().
    std::vector<uint32_t> m_triggerValues;  ///< UG fork: the trigger channel's value at each sample of the current read()
    std::deque<ovStimulation> m_stimQueue;

    bool m_impedanceMode = false;              ///< UG fork: between startImpedanceCheck() and stopImpedanceCheck()
    std::vector<unsigned long> m_impedanceBuffer;  ///< UG fork: what GetSamples gives in impedance mode
};


REGISTER_DRIVER(CDriverTMSi, "TMSi amplifiers", false);
