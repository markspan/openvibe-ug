///-------------------------------------------------------------------------------------------------
///
/// \file CDriverBrainProductsBase.h
/// \author Thomas Prampart (Inria)
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
///
///-------------------------------------------------------------------------------------------------
#pragma once

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include <memory>
#include <vector>
#include <string>

#include "SDK.h"

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

class CAmplifier;

namespace OpenViBE {
namespace AcquisitionServer {

struct BrainProductsDevice {
    std::string id;
    std::vector<float> baseSampleRates;
    std::vector<float> subSampleDivisors;
    size_t availableEEGChannels;
    size_t availableAUXChannels;
    size_t availableACCChannels;
};

struct DeviceSelection {
    size_t selectionIndex = 0;  ///< index of selected device
    size_t baseSampleRateSelectionIndex = 0;  ///< index of sample rate selected for device
    size_t subSampleDivisorSelectionIndex = 0; ///< index of sub sample divisor selected for device
    std::vector<BrainProductsDevice> devices;  ///< List of devices to select from
};

class CDriverBrainProductsBase : public IDriver
{

public:
    explicit CDriverBrainProductsBase(IDriverContext &ctx);

    bool start() override;

    bool initialize(uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool stop() override;
    bool loop() override;

    const IHeader* getHeader() override { return &m_header; }

protected:

    /**
     * \brief Method to override by child class and called during initialize()
     * \return true on success, false otherwise
     */
    virtual bool initializeSpecific()
    {
        return true;
    };

    /**
     * \brief Method to override by child class and called when enough data was received to process.
     * \return true on success, false otherwise
     */
    virtual bool processData() {
        return true;
    };
    /**
     * \brief Get available amplifiers informations
     * \return True if info could be extracted, False otherwise
     */
    bool getAvailableDevices();

    /**
     * \Brief Opens selected amplifier
     * \return True if amplifier was opened successfully, false otherwise
     */
    bool openAmplifier();

    /**
     * \brief Configure LiveAmp, sampling rate and mode.
     * \return True if LiveAmp is successful configured.
     */
    bool configureAmplifier();

    /**
    * \brief Calcualtes the size of 'one' sample the amplifier delivers. It is actually a sum of the one sample per each channel that will be acquired.
       This information is needed to read data from buffer, in order to access to each sample of each channel.
    * \return Size of 'one' acquired sample.
    */
    uint32_t getAmplifierSampleSize();

    /**
     * \brief Reads and parses data from amplifier
     * \param extractedData [out] vector of samples as a vector of one sample per channel.
     * \return the amount of samples read.
     */
    size_t getData(std::vector<std::vector<float>>& extractedData);

    /**
     * brief Clears and re-initialises buffers storing data from the device
     */
    void resetBuffers();

    /**
     * \brief Retrieve from device the channels providing impedance data
     * \return True on success, false otherwise
     */
    bool configureImpedanceMeasure();

    /**
     * \brief Update acquisition server with impedance data
     */
    void checkImpedance();


    IDriverCallback* m_callback = nullptr;
    CHeader m_header;
    std::vector<uint16_t> m_dataTypes;
    uint32_t m_nSamplePerSentBlock = 0;

    std::unique_ptr<CAmplifier> m_amplifier = nullptr;
    AmplifierFamily m_ampFamily;
    DeviceSelection m_deviceSelection;
    bool m_devicesEnumerated = false;

    RecordingMode m_recordingMode = RM_STOPPED;

    std::vector<BYTE> m_sampleBuffer;  //< Stores data on each API call
    std::vector<std::vector<float>> m_sendBuffers;  //< Store data received from device after each read
    size_t m_sampleSize = 0;
    uint32_t m_nUsedChannels     = 0;
    std::vector<float> m_resolutions;
    std::vector<uint16_t> m_triggerIndices;
    std::vector<uint16_t> m_lastTriggerStates;

    // Data to send to acq. server
    std::vector<float> m_samples;
    CStimulationSet m_stimSet;

    std::vector<float> m_impedanceBuffer;
    uint32_t m_impedanceChannels = 0;
    uint32_t m_goodImpedanceLimit = 5000;
    uint32_t m_badImpedanceLimit  = 10000;
};

}  // AcquisitionServer
}  // OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
