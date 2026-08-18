///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCConfigurationBrainmasterDiscovery.cpp
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

#if defined TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI

#include "ovasCConfigurationBrainmasterDiscovery.h"

#include "ovas_defines_brainmaster_discovery.h"

namespace OpenViBE {
namespace AcquisitionServer {

namespace {
static bool guardFlag = false;

void on_spin_button_changed(GtkSpinButton* pAnySpinButton, GtkComboBox* pDevicePresetComboBox)
{
    if (!guardFlag) { gtk_combo_box_set_active(pDevicePresetComboBox, 0); } // Custom 
}

void on_combo_box_changed(GtkComboBox* pAnyComboBox, GtkComboBox* pDevicePresetComboBox)
{
    if (!guardFlag) { gtk_combo_box_set_active(pDevicePresetComboBox, 0); } // Custom
}

void on_device_preset_changed(GtkComboBox* pComboBox, CConfigurationBrainmasterDiscovery* pThis)
{
    guardFlag = true;

    // int i;
    const int active = gtk_combo_box_get_active(pComboBox);
    switch (active)
    {
        default:
        case Preset_Custom: break;
        case Preset_Discovery_24: gtk_spin_button_set_value(pThis->m_buttonChannelCount, 24); // 22 EEG channels + 2 AUX
            gtk_combo_box_set_active(pThis->m_comboBoxType, 0);                     // Discovery
            gtk_combo_box_set_active(pThis->m_comboBoxSamplingRate, 0);             // default
            //gtk_combo_box_set_active(pThis->m_comboBoxDevice, );
            //gtk_combo_box_set_active(pThis->m_comboBoxPreset, );
            gtk_combo_box_set_active(pThis->m_comboBoxBaudRate, 0);                 // default
            gtk_combo_box_set_active(pThis->m_comboBoxBitDepth, 0);                 // default
            gtk_combo_box_set_active(pThis->m_comboBoxNotchFilters, 0);             // default
            gtk_widget_set_sensitive(GTK_WIDGET(pThis->m_comboBoxBitDepth), FALSE); // default
            pThis->m_rvChannelType.clear();
            //for (i=0;  i<22; ++i) pThis->m_rvChannelType[i]=ChannelType_EEG;
            //for (i=22; i<24; ++i) pThis->m_rvChannelType[i]=ChannelType_AUX;
            pThis->m_channelNames[0]  = "Fp1";
            pThis->m_channelNames[1]  = "F3";
            pThis->m_channelNames[2]  = "C3";
            pThis->m_channelNames[3]  = "P3";
            pThis->m_channelNames[4]  = "O1";
            pThis->m_channelNames[5]  = "F7";
            pThis->m_channelNames[6]  = "T3";
            pThis->m_channelNames[7]  = "T5";
            pThis->m_channelNames[8]  = "Fz";
            pThis->m_channelNames[9]  = "Fp2";
            pThis->m_channelNames[10] = "F4";
            pThis->m_channelNames[11] = "C4";
            pThis->m_channelNames[12] = "P4";
            pThis->m_channelNames[13] = "O2";
            pThis->m_channelNames[14] = "F8";
            pThis->m_channelNames[15] = "T4";
            pThis->m_channelNames[16] = "T6";
            pThis->m_channelNames[17] = "Cz";
            pThis->m_channelNames[18] = "Pz";
            pThis->m_channelNames[19] = "A2";
            pThis->m_channelNames[20] = "Fpz";
            pThis->m_channelNames[21] = "Oz";
            pThis->m_channelNames[22] = "AUX1";
            pThis->m_channelNames[23] = "AUX2";
            break;
        case Preset_Atlantis_2x2: gtk_spin_button_set_value(pThis->m_buttonChannelCount, 4); // 2 EEG channels + 2 AUX
            gtk_combo_box_set_active(pThis->m_comboBoxType, 1); // Atlantis
            gtk_combo_box_set_active(pThis->m_comboBoxSamplingRate, 0); // default
            //gtk_combo_box_set_active(pThis->m_comboBoxDevice, );
            //gtk_combo_box_set_active(pThis->m_comboBoxPreset, );
            gtk_combo_box_set_active(pThis->m_comboBoxBaudRate, 0); // default
            gtk_combo_box_set_active(pThis->m_comboBoxBitDepth, 0); // default
            gtk_combo_box_set_active(pThis->m_comboBoxNotchFilters, 0); // default
            gtk_widget_set_sensitive(GTK_WIDGET(pThis->m_comboBoxBitDepth), TRUE); // default
            pThis->m_rvChannelType.clear();
            pThis->m_rvChannelType[0] = ChannelType_AUX;
            pThis->m_rvChannelType[1] = ChannelType_AUX;
            pThis->m_rvChannelType[2] = ChannelType_EEG;
            pThis->m_rvChannelType[3] = ChannelType_EEG;
            pThis->m_channelNames[0]  = "AUX1";
            pThis->m_channelNames[1]  = "AUX2";
            pThis->m_channelNames[2]  = "A1-R1";
            pThis->m_channelNames[3]  = "A2-R2";
            break;
        case Preset_Atlantis_4x4: gtk_spin_button_set_value(pThis->m_buttonChannelCount, 8); // 2 EEG channels + 2 AUX + 2 EEG channels + 2 AUX
            gtk_combo_box_set_active(pThis->m_comboBoxType, 1); // Atlantis
            gtk_combo_box_set_active(pThis->m_comboBoxSamplingRate, 0); // default
            //gtk_combo_box_set_active(pThis->m_comboBoxDevice, );
            //gtk_combo_box_set_active(pThis->m_comboBoxPreset, );
            gtk_combo_box_set_active(pThis->m_comboBoxBaudRate, 0); // default
            gtk_combo_box_set_active(pThis->m_comboBoxBitDepth, 0); // default
            gtk_combo_box_set_active(pThis->m_comboBoxNotchFilters, 0); // default
            gtk_widget_set_sensitive(GTK_WIDGET(pThis->m_comboBoxBitDepth), TRUE); // default
            pThis->m_rvChannelType.clear();
            pThis->m_rvChannelType[0] = ChannelType_AUX;
            pThis->m_rvChannelType[1] = ChannelType_AUX;
            pThis->m_rvChannelType[2] = ChannelType_EEG;
            pThis->m_rvChannelType[3] = ChannelType_EEG;
            pThis->m_rvChannelType[4] = ChannelType_AUX;
            pThis->m_rvChannelType[5] = ChannelType_AUX;
            pThis->m_rvChannelType[6] = ChannelType_EEG;
            pThis->m_rvChannelType[7] = ChannelType_EEG;
            pThis->m_channelNames[0]  = "AUX1";
            pThis->m_channelNames[1]  = "AUX2";
            pThis->m_channelNames[2]  = "A1-R1";
            pThis->m_channelNames[3]  = "A2-R2";
            pThis->m_channelNames[4]  = "AUX3";
            pThis->m_channelNames[5]  = "AUX4";
            pThis->m_channelNames[6]  = "A3-R3";
            pThis->m_channelNames[7]  = "A4-R4";
            break;
    }

    guardFlag = false;
}
}  // namespace

CConfigurationBrainmasterDiscovery::CConfigurationBrainmasterDiscovery(const char* gtkBuilderFilename, uint32_t& rPort, uint32_t& rPreset, uint32_t& rType,
                                                                       uint32_t& rBaudRate, uint32_t& rSamplingRate, uint32_t& rBitDepth,
                                                                       uint32_t& rNotchFilters, std::map<uint32_t, uint32_t>& rvChannelType,
                                                                       std::string& sDeviceSerial, std::string& sDevicePasskey)
    : CConfigurationBuilder(gtkBuilderFilename), m_rPort(rPort), m_rPreset(rPreset), m_rType(rType), m_rBaudRate(rBaudRate), m_rSamplingRate(rSamplingRate),
      m_rBitDepth(rBitDepth), m_rNotchFilters(rNotchFilters), m_rvChannelType(rvChannelType), m_deviceSerial(sDeviceSerial), m_devicePasskey(sDevicePasskey)
{
    m_listStore = gtk_list_store_new(1, G_TYPE_STRING);
}

CConfigurationBrainmasterDiscovery::~CConfigurationBrainmasterDiscovery() { g_object_unref(m_listStore); }

bool CConfigurationBrainmasterDiscovery::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    const uint32_t port = m_rPort;

    m_buttonChannelCount   = GTK_SPIN_BUTTON(this->m_nChannels);
    m_comboBoxDevice       = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));
    m_comboBoxPreset       = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device_preset"));
    m_comboBoxType         = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_type"));
    m_comboBoxBaudRate     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_baud_rate"));
    m_comboBoxSamplingRate = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));
    m_comboBoxBitDepth     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_bit_depth"));
    m_comboBoxNotchFilters = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_notch_filters"));
    m_pEntryDeviceSerial   = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_device_serial"));
    m_pEntryDevicePasskey  = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_device_passkey"));

    g_signal_connect(G_OBJECT(m_buttonChannelCount), "value-changed", G_CALLBACK(on_spin_button_changed), m_comboBoxPreset);
    g_signal_connect(G_OBJECT(m_comboBoxPreset), "changed", G_CALLBACK(on_device_preset_changed), this);
    g_signal_connect(G_OBJECT(m_comboBoxType), "changed", G_CALLBACK(on_combo_box_changed), m_comboBoxPreset);
    g_signal_connect(G_OBJECT(m_comboBoxBaudRate), "changed", G_CALLBACK(on_combo_box_changed), m_comboBoxPreset);
    g_signal_connect(G_OBJECT(m_comboBoxSamplingRate), "changed", G_CALLBACK(on_combo_box_changed), m_comboBoxPreset);
    g_signal_connect(G_OBJECT(m_comboBoxBitDepth), "changed", G_CALLBACK(on_combo_box_changed), m_comboBoxPreset);
    g_signal_connect(G_OBJECT(m_comboBoxNotchFilters), "changed", G_CALLBACK(on_combo_box_changed), m_comboBoxPreset);

    g_object_unref(m_listStore);
    m_listStore = gtk_list_store_new(1, G_TYPE_STRING);

    gtk_combo_box_set_model(m_comboBoxDevice, GTK_TREE_MODEL(m_listStore));

    char buffer[1024];
    bool selected = false;

    gtk_combo_box_append_text(m_comboBoxDevice, "Auto");
    for (uint32_t i = 1; i < 17; ++i)
    {
        sprintf(buffer, "COM%i", i);
        gtk_combo_box_append_text(m_comboBoxDevice, buffer);
        if (port == i)
        {
            gtk_combo_box_set_active(m_comboBoxDevice, i);
            selected = true;
        }
    }

    if (!selected) { gtk_combo_box_set_active(m_comboBoxDevice, 0); }

    switch (m_rType)
    {
        default:
        case Type_Discovery: gtk_combo_box_set_active(m_comboBoxType, 0);
            break;
        case Type_Atlantis: gtk_combo_box_set_active(m_comboBoxType, 1);
            break;
    }

    switch (m_rBaudRate)
    {
        default:
        case BaudRate_Default: gtk_combo_box_set_active(m_comboBoxBaudRate, 0);
            break;
        case BaudRate_9600: gtk_combo_box_set_active(m_comboBoxBaudRate, 1);
            break;
        case BaudRate_115200: gtk_combo_box_set_active(m_comboBoxBaudRate, 2);
            break;
        case BaudRate_460800: gtk_combo_box_set_active(m_comboBoxBaudRate, 3);
            break;
    }

    switch (m_rSamplingRate)
    {
        default:
        case SamplingFrequency_256: gtk_combo_box_set_active(m_comboBoxSamplingRate, 0);
            break;
        case SamplingFrequency_512: gtk_combo_box_set_active(m_comboBoxSamplingRate, 1);
            break;
        case SamplingFrequency_1024: gtk_combo_box_set_active(m_comboBoxSamplingRate, 2);
            break;
        case SamplingFrequency_2048: gtk_combo_box_set_active(m_comboBoxSamplingRate, 3);
            break;
    }

    switch (m_rBitDepth)
    {
        default:
        case BitDepth_Default: gtk_combo_box_set_active(m_comboBoxBitDepth, 0);
            break;
        case BitDepth_8: gtk_combo_box_set_active(m_comboBoxBitDepth, 1);
            break;
        case BitDepth_16: gtk_combo_box_set_active(m_comboBoxBitDepth, 2);
            break;
        case BitDepth_24: gtk_combo_box_set_active(m_comboBoxBitDepth, 3);
            break;
    }

    switch (m_rNotchFilters)
    {
        default:
        case NotchFilter_Default: gtk_combo_box_set_active(m_comboBoxNotchFilters, 0);
            break;
        case NotchFilter_Off: gtk_combo_box_set_active(m_comboBoxNotchFilters, 1);
            break;
        case NotchFilter_50: gtk_combo_box_set_active(m_comboBoxNotchFilters, 2);
            break;
        case NotchFilter_60: gtk_combo_box_set_active(m_comboBoxNotchFilters, 3);
            break;
    }

    switch (m_rPreset)
    {
        default:
        case Preset_Custom: gtk_combo_box_set_active(m_comboBoxPreset, 0);
            break;
        case Preset_Discovery_24: gtk_combo_box_set_active(m_comboBoxPreset, 1);
            break;
        case Preset_Atlantis_4x4: gtk_combo_box_set_active(m_comboBoxPreset, 2);
            break;
        case Preset_Atlantis_2x2: gtk_combo_box_set_active(m_comboBoxPreset, 3);
            break;
    }

    gtk_entry_set_text(m_pEntryDeviceSerial, m_deviceSerial.c_str());
    gtk_entry_set_text(m_pEntryDevicePasskey, m_devicePasskey.c_str());

    return true;
}

bool CConfigurationBrainmasterDiscovery::postConfigure()
{
    if (m_applyConfig)
    {
        const int port = gtk_combo_box_get_active(m_comboBoxDevice);
        if (port >= 0) { m_rPort = uint32_t(port); }

        const int preset = gtk_combo_box_get_active(m_comboBoxPreset);
        if (preset >= 0) { m_rPreset = uint32_t(preset); }

        const int type = gtk_combo_box_get_active(m_comboBoxType);
        if (type >= 0) { m_rType = uint32_t(type); }

        const int baudRate = gtk_combo_box_get_active(m_comboBoxBaudRate);
        if (baudRate >= 0) { m_rBaudRate = uint32_t(baudRate); }

        const int sampling = gtk_combo_box_get_active(m_comboBoxSamplingRate);
        if (sampling >= 0) { m_rSamplingRate = uint32_t(sampling); }

        const int bitDepth = gtk_combo_box_get_active(m_comboBoxBitDepth);
        if (bitDepth >= 0) { m_rBitDepth = uint32_t(bitDepth); }

        const int notchFilters = gtk_combo_box_get_active(m_comboBoxNotchFilters);
        if (notchFilters >= 0) { m_rNotchFilters = uint32_t(notchFilters); }

        m_deviceSerial  = gtk_entry_get_text(m_pEntryDeviceSerial);
        m_devicePasskey = gtk_entry_get_text(m_pEntryDevicePasskey);
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI
