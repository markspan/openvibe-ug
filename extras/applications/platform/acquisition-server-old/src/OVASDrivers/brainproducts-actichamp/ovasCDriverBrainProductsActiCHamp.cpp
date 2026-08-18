///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include "ovasCDriverBrainProductsActiCHamp.h"
#include "ovasCConfigurationBrainProductsActiCHamp.h"

#include <toolkit/ovtk_all.h>
#include <system/ovCTime.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverBrainProductsActiCHamp::CDriverBrainProductsActiCHamp(IDriverContext& ctx):
    CDriverBrainProductsBase(ctx),
    m_settings("AcquisitionServer_Driver_BrainProductsActiCHamp", m_driverCtx.getConfigurationManager())
{


    m_settings.add( "ActiveShieldGain", &m_activeShieldGain);
    m_settings.add( "EEGChannels", &m_nEEGChannels);
    m_settings.add( "UseAuxChannels", &m_useAuxChannels);
    m_settings.add("DeviceSelectionIndex", &m_deviceSelection.selectionIndex);
    m_settings.add( "SampleFrequencyIdx", &m_deviceSelection.baseSampleRateSelectionIndex);
    m_settings.add( "SubSampleDivisorIdx", &m_deviceSelection.subSampleDivisorSelectionIndex);
    m_settings.add( "samplingFrequency", &m_samplingFrequencySetting);
    m_settings.add("GoodImpedanceLimit", &m_goodImpedanceLimit);
    m_settings.add("BadImpedanceLimit", &m_badImpedanceLimit);

    m_settings.load();

    m_header.setSamplingFrequency(m_samplingFrequencySetting);
    m_header.setChannelCount(m_nEEGChannels);
    m_ampFamily = AmplifierFamily::eActiChampFamily;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverBrainProductsActiCHamp::initializeSpecific()
{
    if (!setEnabledChannels()) {
        return false;
    }

    if (!setActiveShieldGain()){
        return false;
    }

    return true;
}

bool CDriverBrainProductsActiCHamp::setEnabledChannels()
{
    int32_t availableChannels;
    int res = m_amplifier->GetProperty(availableChannels, DPROP_I32_AvailableChannels);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                    << "CDriverBrainProductsActiCHamp::setEnabledChannels: Cannot read AvailableChannels from device;  error code = "
                                    << res << "\n";
        return false;
    }

    int32_t channelType;
    size_t enableChannels = 0;
    int enable;
    for (size_t c = 0; c < availableChannels; ++c) {
        res = m_amplifier->GetProperty(channelType, c, ChannelPropertyID::CPROP_I32_Type);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                        << "CDriverBrainProductsBase::getAvailableDevices: Cannot read AvailableChannels from device - channel "
                                        << c << ";  error code = "
                                        << res << "\n";
            return false;
        }
        switch (channelType) {
            case ChannelType::CT_EEG:
            case ChannelType::CT_BIP:
                enable = (enableChannels < m_nEEGChannels) ? 1 : 0;
                enableChannels++;
                break;
            case ChannelType::CT_AUX:
                enable = m_useAuxChannels ? 1 : 0;
                break;
            default:
                enable = 0;
                break;
        }
        res = m_amplifier->SetProperty(enable, c, ChannelPropertyID::CPROP_B32_RecordingEnabled);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                        << "CDriverBrainProductsBase::getAvailableDevices: Cannot set CPROP_B32_RecordingEnabled for channel "
                                        << c << ";  error code = "
                                        << res << "\n";
            return false;
        }
    }

    return true;
}

bool CDriverBrainProductsActiCHamp::setActiveShieldGain()
{
    // Set property DPROP_I32_ActiveShieldGain from m_activeShieldGain
    int res = m_amplifier->SetProperty(m_activeShieldGain, DevicePropertyID::DPROP_I32_ActiveShieldGain);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "CDriverBrainProductsActiCHamp::setActiveShieldGain Failed to set Active shield gain to " << m_activeShieldGain
                                                              << "; error code: " << res << "\n";
        return false;
    }
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainProductsActiCHamp::configure()
{
    getAvailableDevices();

    CConfigurationBrainProductsActiCHamp config(Directories::getDataDir() + "/applications/acquisition-server/interface-BrainProducts-ActiCHamp.ui",
                                                m_deviceSelection, m_activeShieldGain, m_nEEGChannels, m_useAuxChannels,
                                                m_goodImpedanceLimit, m_badImpedanceLimit);

    if (!config.configure(m_header)) {
        return false;
    }

    if (!m_deviceSelection.devices.empty()) {
        size_t samplingFreq = m_deviceSelection.devices[m_deviceSelection.selectionIndex].baseSampleRates[m_deviceSelection.baseSampleRateSelectionIndex] / m_deviceSelection.devices[m_deviceSelection.selectionIndex].subSampleDivisors[m_deviceSelection.subSampleDivisorSelectionIndex];

        m_header.setSamplingFrequency(samplingFreq);
        m_header.setChannelCount(m_nEEGChannels + (m_useAuxChannels ? 8:0));

        m_samplingFrequencySetting = samplingFreq;
    }

    m_settings.save();

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
