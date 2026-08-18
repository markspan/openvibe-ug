///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDriverBrainProductsLiveAmp.cpp
/// \brief Brain Products LiveAmp driver for OpenViBE
/// \author Ratko Petrovic
/// \copyright Copyright (C) 2017 Brain Products
/// 
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Library General Public
/// License as published by the Free Software Foundation; either
/// version 2 of the License, or (at your option) any later version.
/// 
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Library General Public License for more details.
/// 
/// You should have received a copy of the GNU Library General Public
/// License along with this library; if not, write to the
/// Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
/// Boston, MA  02110-1301, USA.
/// 
///-------------------------------------------------------------------------------------------------

/*********************************************************************
* History
* [2017-03-29] ver 1.0                                                    - RP
* [2017-04-04] ver 1.1 Cosmetic changes: int/uint32_t, static_cast<>...   - RP
*              Function loop: optimized buffer copying. 
* [2017-04-28] ver 1.2 LiveAmp8 and LiveAmp16 channles support added.     - RP
*              Introduced checking of Bipolar channels.
*
*********************************************************************/
#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include <algorithm>

#include "ovasCDriverBrainProductsLiveAmp.h"
#include "ovasCConfigurationBrainProductsLiveAmp.h"

#include <toolkit/ovtk_all.h>
#include <SDK.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverBrainProductsLiveAmp::CDriverBrainProductsLiveAmp(IDriverContext& ctx):
    CDriverBrainProductsBase(ctx),
    m_settings("AcquisitionServer_Driver_BrainProductsLiveAmp", m_driverCtx.getConfigurationManager())
{
    // set default sampling rates:
    m_samplings.push_back(250);
    m_samplings.push_back(500);
    m_samplings.push_back(1000);

    m_header.setSamplingFrequency(250); // init for the first time

    // The following class allows saving and loading driver settings from the acquisition server .conf file 
    m_settings.add("EEGchannels", &m_nEEG);
    m_settings.add("AUXchannels", &m_nAux);
    m_settings.add("ACCchannels", &m_nACC);
    m_settings.add("BipolarChannels", &m_nBipolar);
    m_settings.add("IncludeACC", &m_useAccChannels);

    m_settings.add("SerialNr", &m_sSerialNumber);
    m_settings.add("GoodImpedanceLimit", &m_goodImpedanceLimit);
    m_settings.add("BadImpedanceLimit", &m_badImpedanceLimit);
    m_settings.add("Header", &m_header);
    m_settings.add("UseBipolarChannels", &m_useBipolarChannels);    //m_settings.add("SettingName", &variable);

    // To save your custom driver settings, register each variable to the SettingsHelper    
    m_settings.load();

    m_physicalSampling = m_header.getSamplingFrequency();
    m_header.setChannelCount(m_nEEG + m_nBipolar + m_nAux + m_nACC);

    // Tells SDK which DLL to use.
    m_ampFamily = AmplifierFamily::eLiveAmpFamily;
}

// Empty destructor required here for unique_ptr default_delete to work with forward declaration in header.
CDriverBrainProductsLiveAmp::~CDriverBrainProductsLiveAmp() {}

//___________________________________________________________________//
//                                                                   //
bool CDriverBrainProductsLiveAmp::initializeSpecific()
{
    // must disable all channels, then only enable the ones that will be used.
    if (!checkAvailableChannels() || !disableAllAvailableChannels() || !getChannelIndices()) {
        return false;
    }


}

bool CDriverBrainProductsLiveAmp::isConfigurable()
{
    return true; // change to false if your device is not configurable
}

bool CDriverBrainProductsLiveAmp::configure()
{
    m_amplifier->Close();

    // get sampling rate index:
    uint32_t sampRateIndex = -1;
    for (uint32_t i = 0; i < m_samplings.size(); ++i) {
        if (m_physicalSampling == m_samplings[i]) {
            sampRateIndex = i;
        }
    }

    /* Warning:
     * An error occurs when doing the following:
     * Connecting -> Disconnecting -> Changing device -> Connecting
     * The last connection fails, unless EnumerateDevices is called before.
     * Therefore, this call needs to happen each time
    */
    getAvailableDevices();

    for (size_t i = 0; i < m_deviceSelection.devices.size(); ++i) {
        if (m_deviceSelection.devices[i].id == m_sSerialNumber) {
            m_deviceSelection.selectionIndex = i;
            break;
        }
    }

    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationBrainProductsLiveAmp config(Directories::getDataDir() + "/applications/acquisition-server/interface-BrainProductsLiveAmp.ui",
                                              sampRateIndex, m_nEEG, m_nBipolar, m_nAux, m_nACC, m_useAccChannels,
                                              m_goodImpedanceLimit, m_badImpedanceLimit, m_deviceSelection, m_useBipolarChannels);
    if (!config.configure(m_header)) {
        return false;
    }

    // update sampling rate
    if (sampRateIndex >= 0 && sampRateIndex < m_samplings.size()) {
        m_physicalSampling = m_samplings[sampRateIndex];
    }

    m_header.setSamplingFrequency(m_physicalSampling);
    m_header.setChannelCount(m_nEEG + m_nBipolar + m_nAux + m_nACC);


    m_sSerialNumber = m_deviceSelection.devices[m_deviceSelection.selectionIndex].id;

    m_settings.save();

    return true;
}

bool CDriverBrainProductsLiveAmp::checkAvailableChannels()
{
    // check the "LiveAmp_Channel" version: LiveAmp8, LiveAmp16, LiveAmp32 or LiveAmp64
    uint32_t moduleChannels; // check number of channels that are allowed!
    // Module number set to 0 as per code example, but not sure what modules are and how they are numbered
    int res = m_amplifier->GetProperty(moduleChannels, 0, ModulePropertyID::MPROP_I32_UseableChannels);

    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "#1 MPROP_I32_UseableChannels, error code= " << res << "\n";
        return false;
    }

    // checks available channels, gets the count of each channel type
    uint32_t availableEEG  = 0;
    uint32_t availableAUX  = 0;
    uint32_t availableACC  = 0;
    uint32_t availableTrig = 0;


    int availableChannels;
    m_amplifier->GetProperty(availableChannels, DevicePropertyID::DPROP_I32_AvailableChannels);
    for (int c = 0; c < availableChannels; ++c) {
        int channelType;
        res = m_amplifier->GetProperty(channelType, c, ChannelPropertyID::CPROP_I32_Type);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] GetProperty type error: " << res << "\n";
            return false;
        }

        if (channelType == CT_AUX) {
            char channelFunction[20];
            res = m_amplifier->GetProperty(channelFunction, c, ChannelPropertyID::CPROP_CHR_Function);
            if (res != AMP_OK) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] GetProperty CPROP_CHR_Function #1 error: " << res << "\n";
                return false;
            }

            if (channelFunction[0] == 'X' || channelFunction[0] == 'Y' || channelFunction[0] == 'Z' ||
                channelFunction[0] == 'x' || channelFunction[0] == 'y' || channelFunction[0] == 'z') {
                availableACC++;
            } else {
                availableAUX++;
            }
        }

        else if (channelType == CT_EEG || channelType == CT_BIP) {
            availableEEG++;
        } else if (channelType == CT_TRG || channelType == CT_DIG) {
            char channelFunction[20];
            res = m_amplifier->GetProperty(channelFunction, c, ChannelPropertyID::CPROP_CHR_Function);
            if (res != AMP_OK) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] GetProperty CPROP_CHR_Function #2 error: " << res << "\n";
                return false;
            }
            if (strcmp("Trigger Input", channelFunction) == 0) { availableTrig += 1 + 8; }  // One LiveAmp trigger input + 8 digital inputs from AUX box
        }
    }


    //*********************************************************************************
    // very important check !!! EEG + Bipolar must match configuration limitations
    //*********************************************************************************
    if (moduleChannels == 32) {
        // if there is any Bipolar channel, it means that last 8 physical channels must be Bipolar
        if (m_nBipolar > 0 && m_nEEG > (32 - 8)) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] Number of EEG channels:" << m_nEEG << " and Bipolar channels: " <<
                    m_nBipolar << " don't match the LiveAmp configuration !!!\n";
            return false;
        }
    }

    // Used EEG channels:
    if (m_nEEG + m_nBipolar > moduleChannels) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] Number of used EEG and Bip. channels '" << (m_nEEG + m_nBipolar) <<
                "' don't match with number of channels from LiveAmp Channel configuration '" << moduleChannels << "\n";
        return false;
    }
    if (m_nEEG + m_nBipolar > availableEEG) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] Number of available EEG channels '" << availableEEG <<
                "' don't match with number of channels from Device configuration '" << m_nEEG << "\n";
        return false;
    }
    if (m_header.getSamplingFrequency() >= 1000 && moduleChannels >= 32 && (m_nEEG + m_nBipolar) > 24) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                "If the sampling rate is 1000Hz, there should be 24 EEG (or 21 EEG and 3 AUX)  channels used, to avoid sample loss due to Bluetooth connection.\n";
        return false;
    }


    if (m_nAux > availableAUX) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Number of input AUX channeles (" << m_nAux <<
                ") don't match available number of AUX channels (" << availableAUX << ") \n";
        return false;
    }

    if (m_header.getSamplingFrequency() >= 1000 && moduleChannels >= 32 && (m_nEEG + m_nBipolar + m_nAux + m_nACC) > 24) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                "If the sampling rate is 1000Hz, there should be 24 EEG (or 21 EEG and 3 AUX)  channels used, to avoid sample loss due to Bluetooth connection. \n ";
        return false;
    }

    return true;
}

bool CDriverBrainProductsLiveAmp::disableAllAvailableChannels()
{
    // disables all channle first. It is better to do so, than to enable only the channels that will be used, according to the driver settings.
    int availableChannels;
    int res = m_amplifier->GetProperty(availableChannels, DevicePropertyID::DPROP_I32_AvailableChannels);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " Get available channels, error code= " << res << "\n";
        return false;
    }


    // now disable all channel first,
    for (int c = 0; c < availableChannels; ++c) {
        BOOL disabled = false;
        int channelType      = 0;

        res = m_amplifier->GetProperty(channelType, c, ChannelPropertyID::CPROP_I32_Type);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " Error: ampGetProperty for channel type, channel= " << c << "; error code= " << res <<
                    "\n";
            return false;
        }

        // can not disable trigger and digital channels.
        if (channelType == CT_DIG || channelType == CT_TRG) {
            continue;
        }

        res = m_amplifier->SetProperty(disabled, c, ChannelPropertyID::CPROP_B32_RecordingEnabled);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " Error: ampGetProperty for channel type, channel= " << c << "; error code= " << res << "\n";
            return false;
        }
    }
    return true;
}


bool CDriverBrainProductsLiveAmp::getChannelIndices()
{
    int enable = 1;
    m_nEnabledChannels = 0;
    m_triggerIndices.clear();

    int availableChannels;
    int res = m_amplifier->GetProperty(availableChannels, DevicePropertyID::DPROP_I32_AvailableChannels);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "#3 Get available channels, error code= " << res << "\n";
        return false;
    }

    // check the "LiveAmp_Channel" version: LiveAmp8, LiveAmp16, LiveAmp32 or LiveAmp64
    uint32_t moduleChannels;
    res = m_amplifier->GetProperty(moduleChannels, 0, ModulePropertyID::MPROP_I32_UseableChannels);
    if (res != AMP_OK) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "#2 MPROP_I32_UseableChannels, error code= " << res << "\n";
        return false;
    }

    // enable channels and get indexes of channels to be used!
    // The order of physical channels by LiveAmp: EEGs, BIPs, AUXs, ACCs, TRIGs 
    uint32_t nEEG = 0;
    uint32_t nBip = 0;
    uint32_t nAux = 0;
    uint32_t nAcc = 0;

    for (int c = 0; c < availableChannels; ++c) {
        int channelType;
        res = m_amplifier->GetProperty(channelType, c, ChannelPropertyID::CPROP_I32_Type);
        if (res != AMP_OK) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] GetProperty type error: " << res << "\n";
            return false;
        }

        // type of channel is first EEG. After one of the channle is re-typed as Bipolar, that the rest of 8 channels in the group of 32 will be Bipolar as well
        if (channelType == CT_EEG || channelType == CT_BIP) {
            if (nEEG < m_nEEG) {
                res = m_amplifier->SetProperty(enable, c, ChannelPropertyID::CPROP_B32_RecordingEnabled);
                if (res != AMP_OK) {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " Cannot enable channel: " << c << "; error code= " << res << "\n";
                    return false;
                }

                m_nEnabledChannels++;
                nEEG++;
            } else if (nBip < m_nBipolar) {
                //*********************************************************************************
                // If Bipolar channel will be used, set the type to Bipolar here!!!
                // Still it works for LiveAmp8, LiveAmp16 and LiveAmp32.
                // Bipolar channels can be only phisical channels from index 24 - 31 !!!
                //*********************************************************************************
                if ((c > 23 && c < 32) || (moduleChannels > 32 && c > 55 && c < 64))  {// last 8 channels of 32ch module can be bipolar channels
                    res = m_amplifier->SetProperty(enable, c, ChannelPropertyID::CPROP_B32_RecordingEnabled);
                    if (res != AMP_OK) {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " Cannot enable channel: " << c << "; error code= " << res << "\n";
                        return false;
                    }

                    int cType = CT_BIP;
                    res = m_amplifier->SetProperty(cType, c, ChannelPropertyID::CPROP_I32_Type);
                    if (res != AMP_OK) {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] SetProperty type CT_BIP error: " << res << "\n";
                        return false;
                    }

                    m_nEnabledChannels++;
                    nBip++;
                }
            }

        } else if (channelType == CT_AUX) {
            char channelFunction[20];
            res = m_amplifier->GetProperty(channelFunction, c, ChannelPropertyID::CPROP_CHR_Function);
            if (res != AMP_OK) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[Check] GetProperty CPROP_CHR_Function #1 error: " << res << "\n";
                return false;
            }

            // detect ACC channels
            if (channelFunction[0] == 'X' || channelFunction[0] == 'Y' || channelFunction[0] == 'Z' ||
                channelFunction[0] == 'x' || channelFunction[0] == 'y' || channelFunction[0] == 'z') {
                if (nAcc < m_nACC) {
                    res = m_amplifier->SetProperty(enable, c, ChannelPropertyID::CPROP_B32_RecordingEnabled);
                    if (res != AMP_OK) {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " Cannot enable channel: " << c << "; error code= " << res << "\n";
                        return false;
                    }

                    m_nEnabledChannels++;
                    nAcc++;
                }
            } else {
                if (nAux < m_nAux) {
                    res = m_amplifier->SetProperty(enable, c, ChannelPropertyID::CPROP_B32_RecordingEnabled);
                    if (res != AMP_OK) {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " Cannot enable channel: " << c << "; error code= " << res << "\n";
                        return false;
                    }

                    m_nEnabledChannels++;
                    nAux++;
                }
            }
        } else if (channelType == CT_TRG || channelType == CT_DIG) {  // those channels are always enabled!
            char channelFunction[20];
            res = m_amplifier->GetProperty(channelFunction, c, ChannelPropertyID::CPROP_CHR_Function);
            if (res != AMP_OK) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " GetProperty CPROP_CHR_Function error by Trigger, error code= " << res << "\n";
                return false;
            }

            if (strcmp("Trigger Input", channelFunction) == 0) {
                m_triggerIndices.push_back(m_nEnabledChannels);
            }

            m_nEnabledChannels++; // Trigger and digital channels are always enabled!
        }
    }

    // initialize trigger states
    for (uint32_t i = 0; i < m_triggerIndices.size(); ++i) {
        m_lastTriggerStates.push_back(0);
    }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif //  TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
