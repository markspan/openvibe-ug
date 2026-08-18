///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include "ovasCConfigurationBrainProductsActiCHamp.h"

#include "ovasIHeader.h"

#include <system/ovCTime.h>

#include <iostream>
#include <sstream>

namespace OpenViBE {
namespace AcquisitionServer {

static void DeviceChangedCB(GtkComboBox* /*comboBox*/, gpointer data)
{
    reinterpret_cast<CConfigurationBrainProductsActiCHamp*>(data)->comboBoxDeviceChangedCB();
}

static void SampleRateChangeCB(GtkComboBox*, gpointer data)
{
    reinterpret_cast<CConfigurationBrainProductsActiCHamp*>(data)->labelSamplingRateChangedCB();
}

CConfigurationBrainProductsActiCHamp::CConfigurationBrainProductsActiCHamp(const char* gtkBuilderFilename, DeviceSelection& deviceSelection,
                                                                           int32_t& activeShieldGain, uint32_t& nEEGChannels, bool& useAuxChannels,
                                                                           uint32_t& goodImpedanceLimit, uint32_t& badImpedanceLimit):
    CConfigurationBuilder(gtkBuilderFilename), m_deviceSelection(deviceSelection),
    m_activeShieldGain(activeShieldGain), m_nEEGChannels(nEEGChannels), m_useAuxChannels(useAuxChannels),
    m_goodImpedanceLimit(goodImpedanceLimit), m_badImpedanceLimit(badImpedanceLimit) {}


bool CConfigurationBrainProductsActiCHamp::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) {
        return false;
    }

    m_comboBoxDeviceId              = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device_id"));
    m_comboBoxSampleRate            = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));
    m_comboBoxSubSampleDivisor      = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sub_sample_divisor"));
    m_labelNominalSamplingFrequency = GTK_LABEL(gtk_builder_get_object(m_builder, "nominal_sampling_frequency"));
    m_buttonActiveShieldGain        = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_active_shield_gain"));
    m_buttonChannelsEnabled         = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels"));
    m_buttonUseAuxChannels          = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_aux_channels"));
    m_buttonGoodImpedanceLimit      = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_good_imp"));
    m_buttonBadImpedanceLimit       = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_bad_imp"));

    g_signal_connect(G_OBJECT(m_comboBoxDeviceId), "changed", G_CALLBACK(DeviceChangedCB), this);
    g_signal_connect(G_OBJECT(m_comboBoxSampleRate), "changed", G_CALLBACK(SampleRateChangeCB), this);
    g_signal_connect(G_OBJECT(m_comboBoxSubSampleDivisor), "changed", G_CALLBACK(SampleRateChangeCB), this);

    // autodetection of the connected device(s)
    for (uint32_t i = 0; i < m_deviceSelection.devices.size(); ++i) {
        gtk_combo_box_append_text(m_comboBoxDeviceId, m_deviceSelection.devices[i].id.c_str());
        if (m_deviceSelection.selectionIndex == i) {
            gtk_combo_box_set_active(m_comboBoxDeviceId, i);
        }
    }
    if (!m_deviceSelection.devices.empty() && gtk_combo_box_get_active(m_comboBoxDeviceId) < 0) {
        gtk_combo_box_set_active(m_comboBoxDeviceId, 0);
    }

    if (m_deviceSelection.devices.size() > m_deviceSelection.selectionIndex) {
        gtk_spin_button_set_range(m_buttonChannelsEnabled, gdouble(1),static_cast<gdouble>(m_deviceSelection.devices[m_deviceSelection.selectionIndex].availableEEGChannels));
    }

    if (m_deviceSelection.devices.empty()) {
        m_nEEGChannels = 0;
    }

    gtk_spin_button_set_value(m_buttonChannelsEnabled, m_nEEGChannels);
    gtk_toggle_button_set_active(m_buttonUseAuxChannels, m_useAuxChannels);
    gtk_spin_button_set_value(m_buttonActiveShieldGain, m_activeShieldGain);
    gtk_spin_button_set_value(m_buttonGoodImpedanceLimit, m_goodImpedanceLimit);
    gtk_spin_button_set_value(m_buttonBadImpedanceLimit, m_badImpedanceLimit);
    return true;
}

bool CConfigurationBrainProductsActiCHamp::postConfigure()
{
    if (m_applyConfig) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonChannelsEnabled));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonActiveShieldGain));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonGoodImpedanceLimit));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_buttonBadImpedanceLimit));

        m_deviceSelection.selectionIndex = gtk_combo_box_get_active(m_comboBoxDeviceId);
        m_deviceSelection.baseSampleRateSelectionIndex  = static_cast<size_t>(gtk_combo_box_get_active(m_comboBoxSampleRate));
        m_deviceSelection.subSampleDivisorSelectionIndex  = static_cast<size_t>(gtk_combo_box_get_active(m_comboBoxSubSampleDivisor));
        m_activeShieldGain   = uint32_t(gtk_spin_button_get_value(m_buttonActiveShieldGain));
        m_goodImpedanceLimit = uint32_t(gtk_spin_button_get_value(m_buttonGoodImpedanceLimit));
        m_badImpedanceLimit  = uint32_t(gtk_spin_button_get_value(m_buttonBadImpedanceLimit));
        m_nEEGChannels       = uint32_t(gtk_spin_button_get_value(m_buttonChannelsEnabled));
        m_useAuxChannels     = gtk_toggle_button_get_active(m_buttonUseAuxChannels) != 0;
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

void CConfigurationBrainProductsActiCHamp::comboBoxDeviceChangedCB()
{
    m_selectedDeviceIndex = gtk_combo_box_get_active(m_comboBoxDeviceId);

    resetComboBox(m_comboBoxSampleRate, m_deviceSelection.devices[m_selectedDeviceIndex].baseSampleRates);
    if ( m_deviceSelection.baseSampleRateSelectionIndex < m_deviceSelection.devices[m_selectedDeviceIndex].baseSampleRates.size()) {
        gtk_combo_box_set_active(m_comboBoxSampleRate, m_deviceSelection.baseSampleRateSelectionIndex);
    } else {
        gtk_combo_box_set_active(m_comboBoxSampleRate, 0);
    }

    resetComboBox(m_comboBoxSubSampleDivisor, m_deviceSelection.devices[m_selectedDeviceIndex].subSampleDivisors);
    if (m_deviceSelection.subSampleDivisorSelectionIndex < m_deviceSelection.devices[m_selectedDeviceIndex].subSampleDivisors.size()) {
        gtk_combo_box_set_active(m_comboBoxSubSampleDivisor, m_deviceSelection.subSampleDivisorSelectionIndex);
    } else {
        gtk_combo_box_set_active(m_comboBoxSubSampleDivisor, 0);
    }
}

void CConfigurationBrainProductsActiCHamp::labelSamplingRateChangedCB() {
    std::stringstream ss;
    ss << m_deviceSelection.devices[m_selectedDeviceIndex].baseSampleRates[gtk_combo_box_get_active(m_comboBoxSampleRate)] / m_deviceSelection.devices[m_selectedDeviceIndex].subSampleDivisors[gtk_combo_box_get_active(m_comboBoxSubSampleDivisor)];
    ss << " Hz";
    gtk_label_set_text(m_labelNominalSamplingFrequency, ss.str().c_str());
}

template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
void CConfigurationBrainProductsActiCHamp::resetComboBox(GtkComboBox *comboBox, std::vector<T>& values)
{
    // Clear
    // Not pretty mechanism, but gtk_combo_box_get_has_entry() not available on x86
    int maxFreqOptions = 7;
    while (maxFreqOptions > 0) {
        maxFreqOptions--;
        gtk_combo_box_remove_text(comboBox, 0);
    }

    // Set new values
    std::ostringstream val;
    val.precision(0);
    for (size_t i = 0; i < values.size(); ++i) {
        if (comboBox != nullptr) {
            val.str("");
            val << std::fixed << values[i];
            gtk_combo_box_append_text(comboBox, val.str().c_str());
        }
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
