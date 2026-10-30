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
 * The channel names list has one entry per device channel, read from the amplifier, and only channels with a name
 * are recorded, in device order. EXG channels start out with their device name, or, when a Charmeleon montage is
 * set, with the name it gives their HardwareChannel, which counts EXG channels only (electrodes beyond the device's
 * EXG channels are ignored); EXG channels the montage leaves out are not recorded. All other
 * channels (BIP, AUX, DIG, SAW, ...) start out empty: giving one a name records it under that name.
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
    std::string charmeleonMontage() const override { return m_paramMontage.value(); }
    bool wantsDriftCorrection() const override { return true; }

    void setChunkSize(uint32_t chunkSize) override { m_nSamplePerSentBlock = chunkSize; }

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
    /// Names the EXG channels after the montage, if one is set, and clears those it leaves out.
    bool applyMontage(std::vector<std::string>& names) const;
    void onMontageChanged();
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
    OpenViBE::Parameters::ParameterString m_paramTriggerChannel;
    OpenViBE::Parameters::ParameterBool m_paramCommonModeRejection;
    OpenViBE::Parameters::ParameterPath m_paramMontage;

    std::vector<std::string> m_devices;
    bool m_opened  = false;
    bool m_started = false;

    uint32_t m_nSamplePerSentBlock = 0;
    /// The full channel names list, one entry per device channel, while only the recorded ones are shown. Restored in unInit().
    std::vector<std::string> m_configuredNames;

    // Device signal format
    std::vector<DeviceChannel> m_layout;
    std::string m_layoutDevice; ///< Device m_layout was read from.
    uint32_t m_nDeviceChannel = 0;
    std::vector<bool> m_isChannelActivated;
    int m_triggerChannel        = -1; ///< Device channel carrying the triggers, -1 if none.
    uint32_t m_lastTriggerValue = 0;

    // Raw device buffer, holding what GetSamples returned and read() has not consumed yet
    std::vector<unsigned long> m_rawBuffer;
    uint32_t m_rawSampleCount = 0;
    uint32_t m_rawSampleIndex = 0;

    uint64_t m_lastSampleTime = 0; ///< Timestamp given to the last sample handed over, see getData().
    std::vector<PendingTrigger> m_pendingTriggers;
    std::deque<ovStimulation> m_stimQueue;
};


REGISTER_DRIVER(CDriverTMSi, "TMSi amplifiers", false);
