/**-------------------------------------------------------------------------------------------------
 * \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
 * Rights transferred to Inria, contract signed 21.11.2014
 *-------------------------------------------------------------------------------------------------
 */

#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

#include <openvibe/acquisition-messaging/samples.h>
#include <openvibe/parameters/parameterBool.h>
#include <openvibe/parameters/parameterListString.h>
#include <openvibe/parameters/parameterPath.h>
#include <openvibe/parameters/parameterString.h>

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
 * m_channelCount is the number of EEG (EXG/BIP/AUX) channels, taken in device order, unless a Charmeleon
 * montage is set: it then picks and names the EEG channels by HardwareChannel. Additional channels
 * (DIG, SAW, ...) are added by name. Only additional DIG channels listed there produce stimulations, so the
 * SAW ramp channel, which the SDK also types as digital, never does.
 *
 * Triggers are the low 8 bits of a DIG channel, sent as OVTK_StimulationId_Label(value), i.e. 0x8101 to 0x81FF,
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

    void setChunkSize(uint32_t chunkSize) override { m_nSamplePerSentBlock = chunkSize; }

    OpenViBE::Parameters::Parameters getParameters() override;

private:
    struct PendingTrigger
    {
        uint64_t id;
        uint64_t sampleIndex; ///< Index of the sample in the block returned by the current read().
    };

    bool loadLibrary();
    /// (Re)initializes the SDK on the selected protocol and lists the devices it finds.
    bool initLibrary();
    void exitLibrary();
    bool openFrontEnd(const std::string& deviceID);
    void closeFrontEnd();
    bool readSignalFormat();
    bool setSignalBuffer(uint32_t sampling);
    void printError(const std::string& what) const;

    std::string channelName(size_t index) const;
    std::string channelType(size_t index) const;

    std::unique_ptr<TMSiApi> m_api;

    OpenViBE::Parameters::ParameterListString m_paramProtocol;
    OpenViBE::Parameters::ParameterListString m_paramDevice;
    OpenViBE::Parameters::ParameterString m_paramAdditionalChannels;
    OpenViBE::Parameters::ParameterBool m_paramCommonModeRejection;
    OpenViBE::Parameters::ParameterPath m_paramMontage;

    std::vector<std::string> m_devices;
    bool m_opened  = false;
    bool m_started = false;

    uint32_t m_nSamplePerSentBlock = 0;
    /// Channel count the user asked for, i.e. EEG channels only. Restored in unInit().
    uint32_t m_requestedChannelCount = 0;
    /// EEG channels actually recorded, from the channel count or the montage.
    uint32_t m_eegChannelCount = 0;

    // Device signal format
    uint32_t m_nDeviceChannel    = 0;
    uint32_t m_nDeviceEEGChannel = 0;
    std::vector<bool> m_isChannelActivated;
    std::vector<uint32_t> m_lastTriggerValue; ///< Per device channel, only meaningful for DIG channels.

    // Raw device buffer, holding what GetSamples returned and read() has not consumed yet
    std::vector<unsigned long> m_rawBuffer;
    uint32_t m_rawSampleCount = 0;
    uint32_t m_rawSampleIndex = 0;

    std::vector<PendingTrigger> m_pendingTriggers;
    std::deque<ovStimulation> m_stimQueue;
};


REGISTER_DRIVER(CDriverTMSi, "TMSi amplifiers", false);
