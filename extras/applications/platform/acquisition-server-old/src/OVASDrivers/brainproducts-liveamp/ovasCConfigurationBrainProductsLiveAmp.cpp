///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCConfigurationBrainProductsLiveAmp.cpp
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
* [2017-03-29] ver 1.0                                            - RP
* [2017-05-02] ver 1.1  Due to support for LiveAmp 8 and 16 channels, 
*                       a new variable "m_nBip" added        - RP
*
*********************************************************************/

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include "ovasCConfigurationBrainProductsLiveAmp.h"

namespace OpenViBE {
namespace AcquisitionServer {

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:

//Callback connected to a dedicated gtk button:
static void button_calibrate_pressed_cb(GtkButton* button, void* data)
{
    CConfigurationBrainProductsLiveAmp* config=static_cast<CConfigurationBrainProductsLiveAmp*>(data);
    config->buttonCalibratePressedCB();
}
_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationBrainProductsLiveAmp::CConfigurationBrainProductsLiveAmp(const char* gtkBuilderFilename, uint32_t& physicalSampling,
    uint32_t& rCountEEG, uint32_t& rCountBip, uint32_t& rCountAUX, uint32_t& rCountACC, bool& rUseAccChannels,
    uint32_t& goodImpedanceLimit, uint32_t& badImpedanceLimit, DeviceSelection& deviceSelection, bool& rUseBipolarChannels)
    : CConfigurationBuilder(gtkBuilderFilename), m_physicalSampling(physicalSampling), m_nEEG(rCountEEG),
    m_nBip(rCountBip), m_nAUX(rCountAUX), m_nACC(rCountACC), m_useAccChannels(rUseAccChannels), m_useBipolarChannels(rUseBipolarChannels),
    m_goodImpedanceLimit(goodImpedanceLimit), m_badImpedanceLimit(badImpedanceLimit),
    m_triggers(badImpedanceLimit), m_deviceSelection(deviceSelection) {}

bool CConfigurationBrainProductsLiveAmp::preConfigure()
{
    if (! CConfigurationBuilder::preConfigure()) { return false; }

    m_comboBoxSerialNumber              = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "entrySerialNr"));
    m_comboBoxPhysicalSampleRate = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));

    m_buttonNEEGChannels = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels"));
    m_buttonNBipolar     = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_number_of_bipolar"));
    m_buttonNAUXChannels = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spbBtnAUXchn"));

    m_enableACCChannels     = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_acc"));
    m_enableBipolarChannels = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_use_bipolar_channels"));

    if (gtk_builder_get_object(m_builder, "checkbutton_impedance")) {
        m_impedanceCheck = GTK_WIDGET(gtk_builder_get_object(m_builder, "checkbutton_impedance"));
    }

    m_buttonGoodImpedanceLimit = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_good_imp"));
    m_buttonBadImpedanceLimit  = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_bad_imp"));

    // Connect here all callbacks
    // Example:
    // g_signal_connect(gtk_builder_get_object(m_builder, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

    // Insert here the pre-configure code.
    // For example, you may want to check if a device is currently connected
    // and if more than one are connected. Then you can list in a dedicated combo-box 
    // the device currently connected so the user can choose which one he wants to acquire from.

    for (auto dev: m_deviceSelection.devices) {
        gtk_combo_box_append_text(m_comboBoxSerialNumber, dev.id.c_str());
    }

    gtk_combo_box_set_active(m_comboBoxSerialNumber, static_cast<gint>(m_deviceSelection.selectionIndex));

    gtk_combo_box_set_active(m_comboBoxPhysicalSampleRate, static_cast<gint>(m_physicalSampling));
    gtk_spin_button_set_value(m_buttonNEEGChannels, m_nEEG);
    gtk_spin_button_set_value(m_buttonNBipolar, m_nBip);
    gtk_spin_button_set_value(m_buttonNAUXChannels, m_nAUX);
    gtk_spin_button_set_value(m_buttonGoodImpedanceLimit, m_goodImpedanceLimit);
    gtk_spin_button_set_value(m_buttonBadImpedanceLimit, m_badImpedanceLimit);
    gtk_toggle_button_set_active(m_enableACCChannels, m_useAccChannels);

    return true;
}

bool CConfigurationBrainProductsLiveAmp::postConfigure()
{
    if (m_applyConfig) {
        // If the user pressed the "apply" button, you need to save the changes made in the configuration.
        // For example, you can save the connection ID of the selected device:
        // m_connectionID = <value-from-gtk-widget>
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonGoodImpedanceLimit));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonGoodImpedanceLimit));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonNEEGChannels));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonNBipolar));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonNAUXChannels));

        m_physicalSampling   = gtk_combo_box_get_active(m_comboBoxPhysicalSampleRate);
        GtkTreeModel* widget = gtk_combo_box_get_model(m_comboBoxPhysicalSampleRate);

        m_nEEG = uint32_t(gtk_spin_button_get_value(m_buttonNEEGChannels));
        m_nBip = uint32_t(gtk_spin_button_get_value(m_buttonNBipolar));
        m_nAUX = uint32_t(gtk_spin_button_get_value(m_buttonNAUXChannels));

        m_useAccChannels = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_enableACCChannels)) != 0;

        if (m_useAccChannels) { m_nACC = 3; }  // can be 3 or 6
        else { m_nACC = 0; }

        m_goodImpedanceLimit = uint32_t(gtk_spin_button_get_value(m_buttonGoodImpedanceLimit));
        m_badImpedanceLimit  = uint32_t(gtk_spin_button_get_value(m_buttonBadImpedanceLimit));

        m_deviceSelection.selectionIndex = static_cast<size_t>(gtk_combo_box_get_active(m_comboBoxSerialNumber));
    }

    // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
    if (! CConfigurationBuilder::postConfigure()) { return false; }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
