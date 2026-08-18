///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCConfigurationBrainmasterDiscovery.h
/// \copyright Copyright (C) 2012, Yann Renard. All rights reserved.
/// 
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Lesser General Public
/// License as published by the Free Software Foundation; either
/// version 2.1 of the License, or (at your option) any later version.
/// 
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
/// 
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
/// MA 02110-1301  USA
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#if defined TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI

#include "../ovasCConfigurationBuilder.h"

#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <map>

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationBrainmasterDiscovery final : public CConfigurationBuilder
{
public:
    CConfigurationBrainmasterDiscovery(const char* gtkBuilderFilename, uint32_t& rPort, uint32_t& rPreset, uint32_t& rType,
                                       uint32_t& rBaudRate, uint32_t& rSamplingFrequency, uint32_t& rBitDepth, uint32_t& rNotchFilters,
                                       std::map<uint32_t, uint32_t>& rvChannelType, std::string& sDeviceSerial, std::string& sDevicePasskey);
    ~CConfigurationBrainmasterDiscovery() override;

    bool preConfigure() override;
    bool postConfigure() override;

    GtkSpinButton* m_buttonChannelCount = nullptr;
    GtkComboBox* m_comboBoxDevice       = nullptr;
    GtkComboBox* m_comboBoxPreset       = nullptr;
    GtkComboBox* m_comboBoxType         = nullptr;
    GtkComboBox* m_comboBoxBaudRate     = nullptr;
    GtkComboBox* m_comboBoxSamplingRate = nullptr;
    GtkComboBox* m_comboBoxBitDepth     = nullptr;
    GtkComboBox* m_comboBoxNotchFilters = nullptr;
    GtkEntry* m_pEntryDeviceSerial      = nullptr;
    GtkEntry* m_pEntryDevicePasskey     = nullptr;

    using CConfigurationBuilder::m_channelNames;


    GtkListStore* m_listStore = nullptr;

    uint32_t& m_rPort;
    uint32_t& m_rPreset;
    uint32_t& m_rType;
    uint32_t& m_rBaudRate;
    uint32_t& m_rSamplingRate;
    uint32_t& m_rBitDepth;
    uint32_t& m_rNotchFilters;
    std::map<uint32_t, uint32_t>& m_rvChannelType;
    std::string& m_deviceSerial;
    std::string& m_devicePasskey;

    std::vector<GtkWidget*> m_sensitiveWidgets;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI
