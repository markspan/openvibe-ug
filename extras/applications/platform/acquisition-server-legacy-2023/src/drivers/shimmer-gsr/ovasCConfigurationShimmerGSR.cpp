///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCConfigurationShimmerGSR.cpp
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

#if defined TARGET_OS_Windows

#include "ovasCConfigurationShimmerGSR.hpp"

#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {

// If you added more reference attribute, initialize them here
CConfigurationShimmerGSR::CConfigurationShimmerGSR(const char* gtkBuilderFilename, std::vector<uint32_t> serialPorts, uint32_t& portIndex, double& samplingFrequency)
    : CConfigurationBuilder(gtkBuilderFilename), m_portIndex(portIndex), m_samplingFrequency(samplingFrequency)
{
    m_PortsListStore                = gtk_list_store_new(1, G_TYPE_STRING);
    m_samplingFrequencyListStore    = gtk_list_store_new(1, G_TYPE_STRING);

    m_serialPorts = serialPorts;
}

bool CConfigurationShimmerGSR::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkComboBox* serialPortBox          = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_serialport"));
    GtkComboBox* samplingFrequencyBox   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));

    g_object_unref(m_PortsListStore);
    g_object_unref(m_samplingFrequencyListStore);

    m_PortsListStore                = gtk_list_store_new(1, G_TYPE_STRING);
    m_samplingFrequencyListStore    = gtk_list_store_new(1, G_TYPE_STRING);

    gtk_combo_box_set_model(serialPortBox,          GTK_TREE_MODEL(m_PortsListStore));
    gtk_combo_box_set_model(samplingFrequencyBox,   GTK_TREE_MODEL(m_samplingFrequencyListStore));

    bool selected = false;

    for (uint32_t i = 0; i < m_serialPorts.size(); ++i) {
        std::stringstream ss;

        ss << "COM" << m_serialPorts[i];

        gtk_combo_box_append_text(serialPortBox, ss.str().c_str());
        if (m_portIndex == i) {
            gtk_combo_box_set_active(serialPortBox, i);
            selected = true;
        }
    }
    if (!selected) { gtk_combo_box_set_active(serialPortBox, 0); }

    selected = false;
    for (uint32_t i = 0; i < m_samplingFrequenciesSize; ++i) {
        std::stringstream ss;
        ss << m_samplingFrequencies[i];
        gtk_combo_box_append_text(samplingFrequencyBox, ss.str().c_str());
        if (m_samplingFrequency == m_samplingFrequencies[i]) {
            gtk_combo_box_set_active(samplingFrequencyBox, i);
            selected = true;
        }
    }
    if (!selected) { gtk_combo_box_set_active(samplingFrequencyBox, 0); }

    return true;
}

// Called when clicking on "Apply", in the "Device Configuration" window
bool CConfigurationShimmerGSR::postConfigure()
{

    GtkComboBox* serialPortBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_serialport"));
    GtkComboBox* samplingFrequencyBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));

    if (m_applyConfig) {
        const int portIndex = gtk_combo_box_get_active(serialPortBox);
        if (portIndex >= 0) { m_portIndex = uint32_t(portIndex); }


        const int samplingFrequencyIndex = gtk_combo_box_get_active(samplingFrequencyBox);
        if (samplingFrequencyIndex >= 0) { m_samplingFrequency = m_samplingFrequencies[samplingFrequencyIndex]; }
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; } // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed 

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_OS_Windows
