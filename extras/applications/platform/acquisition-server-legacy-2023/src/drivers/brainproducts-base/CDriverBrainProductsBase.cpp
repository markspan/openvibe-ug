///-------------------------------------------------------------------------------------------------
///
/// \file CDriverBrainProductsBase.cpp
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

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include "CDriverBrainProductsBase.h"


namespace OpenViBE {
namespace AcquisitionServer {

CDriverBrainProductsBase::CDriverBrainProductsBase(IDriverContext &ctx) :
        IDriver(ctx),
        m_amplifier(std::make_unique<CAmplifier>())
{

}

bool CDriverBrainProductsBase::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback &callback)
{
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    m_callback            = &callback;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "[INIT] Initialization has been started. " << "\n";

    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) {
        return false;
    }

    if (!openAmplifier()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Failed to open amplifier \n";
        return false;
    }

    if (!configureAmplifier()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Failed to configure amplifier \n";
        return false;
    }

    if (!initializeSpecific()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Failed to perform specific amplifier initialization \n";
        return false;
    }

    if (m_driverCtx.isImpedanceCheckRequested()) {
        if (!configureImpedanceMeasure()) {
            return false;
        }
    }

    if (!m_driverCtx.isImpedanceCheckRequested()) {
        m_sampleSize = getAmplifierSampleSize();
        resetBuffers();
    } else {
        const int res = m_amplifier->StartAcquisition(m_recordingMode);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " [Impedance] Device Handle = NULL.\n";
        }

        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Impedance Acquisition started...\n";
    }

    return true;
}

void CDriverBrainProductsBase::resetBuffers()
{
    m_sampleBuffer.clear();
    m_sampleBuffer.resize(m_sampleSize * m_header.getSamplingFrequency() / 4);  // 250ms of sampling for all channels
    m_sendBuffers.clear();
    m_samples.clear();
    m_samples.resize(m_header.getChannelCount() * m_nSamplePerSentBlock);// allocate for all analog signals / channels
}

bool CDriverBrainProductsBase::uninitialize()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Uninitialized called.\n";
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) {
        return false;
    }

    // ...
    // uninitialize hardware here
    // ...
    if (m_amplifier != nullptr) {
        int res = m_amplifier->StopAcquisition();
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Uninitialized called. stopping acquisition:" << res << "\n";
        }

        res = m_amplifier->Close();
        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Uninitialized called. Closing the device:" << res << "\n";
    }

    m_sampleBuffer.clear();
    m_callback     = nullptr;
    m_samples.clear();

    return true;
}

bool CDriverBrainProductsBase::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Stop called.\n";
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) {
        return false;
    }

    // ...
    // request the hardware to stop
    // sending data
    // ...
    if (m_amplifier != nullptr) {
        const int res = m_amplifier->StopAcquisition();
        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Stopping the acquisition:" << res << "\n";
    }

    return true;
}

bool CDriverBrainProductsBase::loop()
{
    if (!m_driverCtx.isConnected()) {
        return false;
    }

    if (m_driverCtx.isStarted()) {
        std::vector<std::vector<float>> buffers(1, std::vector<float>(1));
        while (true) {
            // receive samples from hardware
            // put them the correct way in the sample array
            // whether the buffer is full, send it to the acquisition server

            const size_t nSample = getData(buffers);
            if (nSample < 1) {
                return true;
            }

            for (size_t sample = 0; sample < nSample; ++sample) {
                m_sendBuffers.push_back(buffers[sample]);
            }
            const int readyToSend = int(m_sendBuffers.size()) - int(m_nSamplePerSentBlock); // must check buffer size in that way !!!
            if (readyToSend > 0) {

                for (size_t ch = 0, i = 0; ch < m_header.getChannelCount(); ++ch) {
                    for (size_t sample = 0; sample < m_nSamplePerSentBlock; ++sample) {
                        m_samples[i++] = m_sendBuffers[sample][ch];
                    }
                }

                for (size_t sample = 0; sample < m_nSamplePerSentBlock; ++sample) { // check triggers:
                    for (size_t t = 0; t < m_triggerIndices.size(); ++t) {
                        const uint16_t trigg = uint16_t(m_sendBuffers[sample][m_triggerIndices[t]]);
                        if (trigg != m_lastTriggerStates[t]) {
                            const uint64_t time = CTime(m_header.getSamplingFrequency(), sample).time();
                            m_stimSet.push_back(OVTK_StimulationId_Label(trigg), time, 0); // send the same time as the 'sample'
                            m_lastTriggerStates[t] = trigg;
                        }
                    }
                }

                // send acquired data...
                m_callback->setSamples(&m_samples[0], m_nSamplePerSentBlock);
                m_callback->setStimulationSet(m_stimSet);

                m_samples.clear();
                m_stimSet.clear();
                // When your sample buffer is fully loaded,
                // it is advised to ask the acquisition server
                // to correct any drift in the acquisition automatically.
                m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

                // delete sent samples
                m_sendBuffers.erase(m_sendBuffers.begin(), m_sendBuffers.begin() + m_nSamplePerSentBlock);
                break;
            }
        }
    }
    else if (m_driverCtx.isImpedanceCheckRequested()) //impedance measurement
    {
        if (m_recordingMode != RM_IMPEDANCE) {
            return true;
        }   // go out of the loop
        checkImpedance();
    }

    return true;
}

bool CDriverBrainProductsBase::getAvailableDevices()
{
    // Interface type
    std::string hwi("ANY");
    // container for Device address
    std::string deviceAddress = "";

    if (m_devicesEnumerated) {
        return true;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info
                                << "CDriverBrainProductsBase::getAvailableDevices: Amp family: "<< ((m_ampFamily == eActiChampFamily) ? "Actichamp":"LiveAmp") << "\n";
    SetAmplifierFamily(m_ampFamily);

    // Clear list before filling it again.
    m_deviceSelection.devices.clear();

    m_amplifier->Close();

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info
                                << "CDriverBrainProductsBase::getAvailableDevices: Enumerate devices\n";
    int res = EnumerateDevices(&hwi[0], int32_t(hwi.size()), deviceAddress.data(), 0);
    if (res == 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "CDriverBrainProductsBase::getAvailableDevices: No amplifier connected! \n";
        return false;
    }
    if (res < 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                    << "CDriverBrainProductsBase::getAvailableDevices: Error enumerating devices ; error code= "
                                    << res << "\n";
        return false;
    }

    const int numDevices = res;
    for (int i = 0; i < numDevices; ++i) {
        BrainProductsDevice dev;
        dev.availableEEGChannels = 0;
        dev.availableAUXChannels = 0;
        dev.availableACCChannels = 0;
        dev.subSampleDivisors.clear();
        dev.baseSampleRates.clear();

        res = m_amplifier->Open(i);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                        << "CDriverBrainProductsBase::getAvailableDevices: Cannot open device # "
                                        << i << "; error code = " << res << "\n";
            return false;
        }

        //Get Device ID
        res = m_amplifier->GetProperty(dev.id, DevicePropertyID::DPROP_CHR_SerialNumber); // get serial number
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                        << "CDriverBrainProductsBase::getAvailableDevices: Cannot read Serial number from device # "
                                        << i << ";  error code = "
                                        << res << "\n";
            return false;
        }

        // Get Device Sample Rate
        PropertyRange<float> availableSampleRates;
        res = m_amplifier->GetPropertyRange(availableSampleRates, DPROP_F32_BaseSampleRate);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                        << "CDriverBrainProductsBase::getAvailableDevices: Cannot read BaseSampleRate range from device # "
                                        << i << ";  error code = "
                                        << res << "\n";
        }
        for (int j = 0; j < availableSampleRates.ByteLength / sizeof(float); ++j) {
            dev.baseSampleRates.push_back(availableSampleRates.RangeArray[j]);
        }

        // Get device Sub Sample Divisor
        PropertyRange<float> availableSubSampleDivisors;
        res = m_amplifier->GetPropertyRange(availableSubSampleDivisors, DPROP_F32_SubSampleDivisor);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                        << "CDriverBrainProductsBase::getAvailableDevices: Cannot read SubSampleDivisor range from device # "
                                        << i << ";  error code = "
                                        << res << "\n";
        }
        for (int j = 0; j < availableSubSampleDivisors.ByteLength / sizeof(float); ++j) {
            dev.subSampleDivisors.push_back(availableSubSampleDivisors.RangeArray[j]);
        }

        // Get device channel details
        int32_t availableChannels;
        res = m_amplifier->GetProperty(availableChannels, DPROP_I32_AvailableChannels);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                        << "CDriverBrainProductsBase::getAvailableDevices: Cannot read AvailableChannels from device # "
                                        << i << ";  error code = "
                                        << res << "\n";
            return false;
        }

        int32_t channelType;
        for (size_t c = 0; c < availableChannels; ++c) {
            res = m_amplifier->GetProperty(channelType, c, ChannelPropertyID::CPROP_I32_Type);
            if (res != AMP_OK) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                            << "CDriverBrainProductsBase::getAvailableDevices: Cannot read AvailableChannels from device # "
                                            << i << " - channel "<< c << ";  error code = "
                                            << res << "\n";
                return false;
            }
            switch(channelType) {
                case ChannelType::CT_EEG:
                case ChannelType::CT_BIP:
                    dev.availableEEGChannels++;
                    break;
                case ChannelType::CT_AUX:
                    char channelFunction[20];
                    res = m_amplifier->GetProperty(channelFunction, c, ChannelPropertyID::CPROP_CHR_Function);
                    if (res != AMP_OK) {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] GetProperty CPROP_CHR_Function #1 error: " << res << "\n";
                        return false;
                    }

                    if (channelFunction[0] == 'X' || channelFunction[0] == 'Y' || channelFunction[0] == 'Z' ||
                        channelFunction[0] == 'x' || channelFunction[0] == 'y' || channelFunction[0] == 'z') {
                        dev.availableACCChannels++;
                    } else {
                        dev.availableAUXChannels++;
                    }
                    break;
                default:
                    break;
            }
        }

        // Add device to selection list
        m_deviceSelection.devices.push_back(dev);

        res = m_amplifier->Close();
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                        << "CDriverBrainProductsBase::getAvailableDevices: Cannot close the device # "
                                        << i << "; error code = " << res << "\n";
            return false;
        }
    }

    m_devicesEnumerated = true;
    return true;
}

bool CDriverBrainProductsBase::openAmplifier() {
    m_amplifier->Close();

    if (!m_devicesEnumerated) {
        getAvailableDevices();
    }

    int res = m_amplifier->Open(m_deviceSelection.selectionIndex);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[InitializeAmplifier] Cannot open the device # " << m_deviceSelection.selectionIndex << ": " << m_deviceSelection.devices[m_deviceSelection.selectionIndex].id << "; error code = " << res << "\n";
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "[InitializeAmplifier] Opened Device # " << m_deviceSelection.selectionIndex << ": " << m_deviceSelection.devices[m_deviceSelection.selectionIndex].id << "\n";


    return true;
}

bool CDriverBrainProductsBase::configureAmplifier() {
    // amplifier configuration
    float sampleRate;
    float subSampleDivisor;
    bool hasSubSampleDivisor;
    if (m_ampFamily == eLiveAmpFamily) {
        sampleRate = float(m_header.getSamplingFrequency());
        subSampleDivisor = 1.0;
        hasSubSampleDivisor = false;
    } else {
        sampleRate = m_deviceSelection.devices[m_deviceSelection.selectionIndex].baseSampleRates[m_deviceSelection.baseSampleRateSelectionIndex];
        subSampleDivisor = m_deviceSelection.devices[m_deviceSelection.selectionIndex].subSampleDivisors[m_deviceSelection.subSampleDivisorSelectionIndex];
        hasSubSampleDivisor = true;
    }


    PropertyRange<float> availableSubSampleDivisors;
    // get the available values for this property
    int res = m_amplifier->GetPropertyRange(availableSubSampleDivisors, DPROP_F32_SubSampleDivisor);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Config] Error in GetPropertyRange SubSampleDivisor: " << res << "\n";
        return false;
    }

    if (availableSubSampleDivisors.ByteLength == 0)
    {
        hasSubSampleDivisor = false;
        subSampleDivisor = 1.0;
    }

    res = m_amplifier->SetProperty(sampleRate, DevicePropertyID::DPROP_F32_BaseSampleRate);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Config]  Error setting sampling rate, error code:" << res << "\n";
        return false;
    }

    float tmp;
    res = m_amplifier->GetProperty(tmp, DevicePropertyID::DPROP_F32_BaseSampleRate);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Config] Error reading back sampling rate: " << res << "\n";
        return false;
    } else {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "[Config]  Set sampling frequency: " << tmp << "\n";
    }

    if (hasSubSampleDivisor) {
        res = m_amplifier->SetProperty(subSampleDivisor, DPROP_F32_SubSampleDivisor);
        if (res != AMP_OK)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Config] Error in SetProperty SubSampleDivisor: " << res << "\n";
            return false;
        }

        res = m_amplifier->GetProperty(tmp, DPROP_F32_SubSampleDivisor);
        if (res != AMP_OK)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Config] Error reading back sub sampling divisor: " << res << "\n";
            return false;
        }
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "[Config] Current Effective Sampling Rate: " << (sampleRate / subSampleDivisor) << "\n";

    m_recordingMode = RM_NORMAL;  // initialize acquisition mode: standard/normal
    res = m_amplifier->SetProperty(m_recordingMode, DevicePropertyID::DPROP_I32_RecordingMode);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Config] Error setting acquisition mode, error code: " << res << "\n";
        return false;
    }

    // set good and bad impedance level
    res = m_amplifier->SetProperty(m_goodImpedanceLimit, DevicePropertyID::DPROP_I32_GoodImpedanceLevel);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Config] Error setting DPROP_I32_GoodImpedanceLevel, error code:  " << res << "\n";
        return false;
    }

    res = m_amplifier->SetProperty(m_badImpedanceLimit, DevicePropertyID::DPROP_I32_BadImpedanceLevel);
    if (res != AMP_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Config] Error setting DPROP_I32_BadImpedanceLevel, error code:  " << res << "\n";
        return false;
    }

    return true;
}

uint32_t CDriverBrainProductsBase::getAmplifierSampleSize() {
    int availableChannels, dataType;
    float resolution;
    int byteSize = 0;

    m_dataTypes.clear();
    m_resolutions.clear();
    m_nUsedChannels = 0;

    // iterate through all enabled channels
    int res = m_amplifier->GetProperty(availableChannels, DevicePropertyID::DPROP_I32_AvailableChannels);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [getLiveAmpSampleSize] GetProperty AvailableChannels - error code= " << res << "\n";
    }
    for (int c = 0; c < availableChannels; ++c) {
        int enabled;
        res = m_amplifier->GetProperty(enabled, c, ChannelPropertyID::CPROP_B32_RecordingEnabled);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [getLiveAmpSampleSize] GetProperty Channel RecordingEnabled - error code= " << res << "\n";
        }
        if (enabled) {
            // get the type of channel
            res = m_amplifier->GetProperty(dataType, c, ChannelPropertyID::CPROP_I32_DataType);
            if (res != AMP_OK) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [getLiveAmpSampleSize] GetProperty Channel DataType - error code= " << res << "\n";
            }
            m_dataTypes.push_back(dataType);
            res = m_amplifier->GetProperty(resolution, c, ChannelPropertyID::CPROP_F32_Resolution);
            if (res != AMP_OK) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [getLiveAmpSampleSize] GetProperty Channel Resolution - error code= " << res << "\n";
            }
            m_resolutions.push_back(resolution);
            m_nUsedChannels++;

            switch (dataType) {
                case DT_INT16:
                case DT_UINT16:
                    byteSize += 2;
                    break;
                case DT_INT32:
                case DT_UINT32:
                case DT_FLOAT32:
                    byteSize += 4;
                    break;
                case DT_INT64:
                case DT_UINT64:
                case DT_FLOAT64:
                    byteSize += 8;
                    break;
                default:
                    break;
            }
        }
    }

    byteSize += 8; // add the sample counter size

    return byteSize;
}

bool CDriverBrainProductsBase::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) {
        return false;
    }

    if (m_amplifier == nullptr) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[start] Device Handle not initialised\n";
    }

    int res = m_amplifier->GetProperty(m_recordingMode, DevicePropertyID::DPROP_I32_RecordingMode);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [start] GetProperty RecordingMode error code = " << res << "\n";
    }

    if (m_recordingMode != RM_NORMAL) {
        res = m_amplifier->StopAcquisition();
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [start] StopAcquisition error code = " << res << "\n";
        }

        // Maybe redundant as the new API gets the recording mode from the StartAcquisition call.
        m_recordingMode = RM_NORMAL;
        m_amplifier->SetProperty(m_recordingMode, DevicePropertyID::DPROP_I32_RecordingMode);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [start] SetProperty RecordingMode error code = " << res << "\n";
        }
    }

    res = m_amplifier->StartAcquisition(m_recordingMode);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [start] ampStartAcquisition error code = " << res << "\n";
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "[start] Acquisition started...\n";

    return true;
}

size_t CDriverBrainProductsBase::getData(std::vector<std::vector<float>>& extractedData)
{
    size_t offset;
    float sample = 0.0;

    const int ret = m_amplifier->GetData(&m_sampleBuffer[0], static_cast<int32_t>(m_sampleBuffer.size()), 0);
    if (ret < 1) {
        return ret;
    }

    size_t samplesRead = ret / m_sampleSize;

    extractedData.clear();
    extractedData.resize(samplesRead);

    for (size_t s = 0; s < samplesRead; ++s) {
        offset = 8; // Sample count offset

        extractedData[s].resize(m_nUsedChannels);

        for (uint32_t i = 0; i < m_nUsedChannels; ++i) {
            switch (m_dataTypes[i]) {
                case DT_INT16:
                {
                    const int16_t tmp = reinterpret_cast<int16_t&>(m_sampleBuffer[s * m_sampleSize + offset]);
                    sample            = float(tmp) * m_resolutions[i];
                    offset += 2;
                    break;
                }
                case DT_UINT16:
                {
                    const uint16_t tmp = reinterpret_cast<uint16_t&>(m_sampleBuffer[s * m_sampleSize + offset]);
                    sample             = float(tmp) * m_resolutions[i];
                    offset += 2;
                    break;
                }
                case DT_INT32:
                {
                    const int tmp = reinterpret_cast<int&>(m_sampleBuffer[s * m_sampleSize + offset]);
                    sample        = float(tmp) * m_resolutions[i];
                    offset += 4;
                    break;
                }
                case DT_UINT32:
                {
                    const uint32_t tmp = reinterpret_cast<uint32_t&>(m_sampleBuffer[s * m_sampleSize + offset]);
                    sample             = float(tmp) * m_resolutions[i];
                    offset += 4;
                    break;
                }
                case DT_FLOAT32:
                {
                    const float tmp = reinterpret_cast<float&>(m_sampleBuffer[s * m_sampleSize + offset]);
                    sample          = tmp * m_resolutions[i];
                    offset += 4;
                    break;
                }
                case DT_INT64:
                {
                    const int64_t tmp = reinterpret_cast<int64_t&>(m_sampleBuffer[s * m_sampleSize + offset]);
                    sample            = float(tmp) * m_resolutions[i];
                    offset += 8;
                    break;
                }
                case DT_UINT64:
                {
                    const uint64_t tmp = reinterpret_cast<uint64_t&>(m_sampleBuffer[s * m_sampleSize + offset]);
                    sample             = float(tmp) * m_resolutions[i];
                    offset += 8;
                    break;
                }
                case DT_FLOAT64:
                {
                    const double tmp = reinterpret_cast<double&>(m_sampleBuffer[s * m_sampleSize + offset]);
                    sample           = float(tmp) * m_resolutions[i];
                    offset += 8;
                    break;
                }
                default:
                    break;
            }

            extractedData[s][i] = sample;
        }
    }

    return samplesRead;
}

bool CDriverBrainProductsBase::configureImpedanceMeasure()
{
    if (!m_driverCtx.isImpedanceCheckRequested()) {
        return true;
    }

    // Set Impedance mode
    m_recordingMode = RM_IMPEDANCE;
    int res = m_amplifier->SetProperty(m_recordingMode, DevicePropertyID::DPROP_I32_RecordingMode);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[configureImpedanceMeasure] Error setting impedance mode, error code:  " << res << "\n";
        return false;
    }

    // for impedance measurements use only EEG and Bip channels
    int availableChannels;
    res = m_amplifier->GetProperty(availableChannels, DevicePropertyID::DPROP_I32_AvailableChannels);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[configureImpedanceMeasure] Get available channels, error code: " << res << "\n";
        return false;
    }

    m_impedanceChannels = 0;
    for (int c = 0; c < availableChannels; ++c) {
        int enabled = 0;
        int impedanceSupported = 0;

        res = m_amplifier->GetProperty(enabled, c, ChannelPropertyID::CPROP_B32_RecordingEnabled);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [configureImpedanceMeasure] Cannot read enable channel: " << c << "; error code: " << res << "\n";
            return false;
        }

        if (enabled) {
            // Check if this channel can do impedance and add it!
            res = m_amplifier->GetProperty(impedanceSupported, c, ChannelPropertyID::CPROP_B32_ImpedanceMeasurement);
            if (res != AMP_OK) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [configureImpedanceMeasure] Cannot read channel impedance support: " << c << "; error code: " << res << "\n";
                return false;
            }
            if (impedanceSupported) {
                m_impedanceChannels++;
            }
        }
    }

    return true;
}

void CDriverBrainProductsBase::checkImpedance() {

    m_impedanceBuffer.clear();
    m_impedanceBuffer.resize(m_impedanceChannels * 2 + 2, -1.0);  // 2 fields per impedance + 2 for REF and GND

    const int bytesRead = m_amplifier->GetData(&m_impedanceBuffer[0], m_impedanceBuffer.size() * sizeof(float), m_impedanceBuffer.size() * sizeof(float));

    if (bytesRead < 1) {
        return;
    }

    for (size_t ch = 0, i = 2; i < m_impedanceBuffer.size(); i += 2, ++ch) {
        double impedance = m_impedanceBuffer[i] - ((m_impedanceBuffer[i+1] < 0) ? 0.0 : m_impedanceBuffer[i+1]);
        m_driverCtx.updateImpedance(ch, impedance);
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif  // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
