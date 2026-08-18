///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#include "ovasCConfigurationTMSi.h"
#include "ovasCDriverTMSi.h"
#include "ovasCTMSiAccess.h"

namespace OpenViBE {
namespace AcquisitionServer {

#if defined TARGET_HAS_ThirdPartyTMSi

namespace {
void ComboBoxCommunicationProtocolChangedCB(GtkComboBox* comboBox, CConfigurationTMSi* configuration)
{
    configuration->showWaitWindow();
    configuration->m_Driver->m_pTMSiAccess->initializeTMSiLibrary(gtk_combo_box_get_active_text(comboBox));
    configuration->hideWaitWindow();
    configuration->fillDeviceCombobox();
}

void ComboBoxDeviceChangedCB(GtkComboBox* comboBox, CConfigurationTMSi* configuration)
{
    CTMSiAccess* tmsiAccess = configuration->m_Driver->m_pTMSiAccess;

    const CString currentDevice = CString(gtk_combo_box_get_active_text(comboBox));

    // If the currently chosen device has no name it means the combo box is empty
    if (currentDevice == CString("")) {
        configuration->clearAdditionalChannelsTable();

        // clear the list of sampling frequencies
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(configuration->m_comboBoxSamplingFrequency)));
        return;
    }

    configuration->showWaitWindow();
    tmsiAccess->openFrontEnd(currentDevice);

    bool deviceHasImpedanceTestingAbility = false;
    if (!tmsiAccess->getImpedanceTestingCapability(&deviceHasImpedanceTestingAbility)) {
        tmsiAccess->closeFrontEnd();
        configuration->hideWaitWindow();
        return;
    }


    if (!tmsiAccess->calculateSignalFormat(currentDevice)) {
        tmsiAccess->closeFrontEnd();
        configuration->hideWaitWindow();
        return;
    }

    if (!configuration->fillSamplingFrequencyCombobox()) {
        tmsiAccess->freeSignalFormat();
        tmsiAccess->closeFrontEnd();
        configuration->hideWaitWindow();
        return;
    }

    configuration->fillAdditionalChannelsTable();

    tmsiAccess->freeSignalFormat();
    tmsiAccess->closeFrontEnd();
    configuration->hideWaitWindow();

    // activate/deactivate the impedance checking dropbown box
    gtk_widget_set_sensitive(GTK_WIDGET(configuration->m_comboBoxImpedanceLimit), deviceHasImpedanceTestingAbility);

    // modify the EEG channel count spinbox to reflect the detected maximum number of channels
    GtkAdjustment* eegChannelCountAdjustment = gtk_spin_button_get_adjustment(configuration->m_buttonChannelCount);
    gtk_adjustment_set_upper(eegChannelCountAdjustment, tmsiAccess->getMaximumEEGChannelCount());
    gtk_adjustment_set_value(eegChannelCountAdjustment, tmsiAccess->getMaximumEEGChannelCount());
}

void ChannelCountChangedCB(GtkSpinButton* spinButton, CConfigurationTMSi* /*configuration*/) { gtk_spin_button_update(spinButton); }

void RemoveWidgets(GtkWidget* widget, gpointer /*data*/) { gtk_widget_destroy(widget); }
}  // namespace

/* Fills the combobox containing list of devices with TMSi devices connected via the
 * currently selected protocol
 */
void CConfigurationTMSi::fillDeviceCombobox() const
{
    int idx                       = -1;
    GtkListStore* deviceListStore = GTK_LIST_STORE(gtk_combo_box_get_model(m_comboBoxDeviceID));
    gtk_list_store_clear(deviceListStore);

    for (size_t i = 0; i < m_Driver->m_pTMSiAccess->getDeviceList().size(); i++) {
        gtk_combo_box_append_text(m_comboBoxDeviceID, m_Driver->m_pTMSiAccess->getDeviceList()[i].toASCIIString());
        if (m_Driver->m_deviceID == m_Driver->m_pTMSiAccess->getDeviceList()[i]) { idx = int(i); }
    }

    // set the active combobox field to the currently set device (if it is present)
    if (idx != -1) { gtk_combo_box_set_active(m_comboBoxDeviceID, idx); }
}

// fills the sampling freuqency combobox, as a side-effect, will also correctly set the maximum number of EEG channels inside the TMSiAccess object
bool CConfigurationTMSi::fillSamplingFrequencyCombobox()
{
    CTMSiAccess* tmsiAccess = m_Driver->m_pTMSiAccess;

    // clear the list of sampling frequencies
    GtkListStore* frequencyListStore = GTK_LIST_STORE(gtk_combo_box_get_model(m_comboBoxSamplingFrequency));
    gtk_list_store_clear(frequencyListStore);

    showWaitWindow();
    std::vector<unsigned long> samplingFrequencies = tmsiAccess->discoverDeviceSamplingFrequencies();
    // Emtpy the list of acquisition frequencies and fill it with the frequencies supported by the device
    hideWaitWindow();

    if (samplingFrequencies.empty()) { return false; }

    int idx = -1;
    for (size_t samplingFrequencyIdx = 0; samplingFrequencyIdx < samplingFrequencies.size(); samplingFrequencyIdx++) {
        std::stringstream ss;

        // TMSi uses sampling frequencies in mHz but we want to display them in Hz
        ss << samplingFrequencies[samplingFrequencyIdx] / 1000;
        gtk_combo_box_append_text(m_comboBoxSamplingFrequency, ss.str().c_str());

        if (m_Driver->m_header.getSamplingFrequency() == samplingFrequencies[samplingFrequencyIdx] / 1000) { idx = int(samplingFrequencyIdx); }
    }

    // set the active combobox field to the currently set sampling frequency (if it is present)
    if (idx != -1) { gtk_combo_box_set_active(m_comboBoxSamplingFrequency, idx); }
    else {
        // set the sampling frequency to the biggest one
        gtk_combo_box_set_active(m_comboBoxSamplingFrequency, gint(samplingFrequencies.size() - 1));
    }

    return true;
}

// clears the table with additional channels
void CConfigurationTMSi::clearAdditionalChannelsTable()
{
    gtk_container_foreach(GTK_CONTAINER(m_TableAdditionalChannels), RemoveWidgets, m_TableAdditionalChannels);
    gtk_table_resize(m_TableAdditionalChannels, 1, 2);
    m_AdditionalChannelCheckButtons.erase(m_AdditionalChannelCheckButtons.begin(), m_AdditionalChannelCheckButtons.end());
    m_additionalChannelNames.erase(m_additionalChannelNames.begin(), m_additionalChannelNames.end());
    gtk_widget_hide(GTK_WIDGET(m_TableAdditionalChannels));
    gtk_widget_hide(GTK_WIDGET(m_LabelAdditionalChannels));
}

// fills the table with additional channels
void CConfigurationTMSi::fillAdditionalChannelsTable()
{
    CTMSiAccess* tmsiAccess = m_Driver->m_pTMSiAccess;

    const uint32_t nAdditionalChannels       = tmsiAccess->getActualChannelCount() - tmsiAccess->getMaximumEEGChannelCount();
    const uint32_t firstAdditionalChannelIdx = tmsiAccess->getMaximumEEGChannelCount();

    clearAdditionalChannelsTable();
    gtk_table_resize(m_TableAdditionalChannels, nAdditionalChannels, 2);

    for (size_t c = 0; c < nAdditionalChannels; ++c) {
        const size_t actualIdx = c + firstAdditionalChannelIdx;

        CString label = "Channel <b>" + tmsiAccess->getChannelName(actualIdx) + "</b> of type " + tmsiAccess->getChannelType(actualIdx).toASCIIString();

        GtkWidget* labelChannelName = gtk_label_new(label.toASCIIString());
        gtk_label_set_use_markup(GTK_LABEL(labelChannelName), TRUE);

        GtkWidget* buttonChannelActive = gtk_check_button_new();
        gtk_table_set_row_spacings(m_TableAdditionalChannels, 5);
        m_AdditionalChannelCheckButtons.push_back(GTK_CHECK_BUTTON(buttonChannelActive));
        m_additionalChannelNames.push_back(tmsiAccess->getChannelName(actualIdx));

        // if the channel was previously selected, check it in the list
        // the list of selected channels is in form ;CH1;CH2;CH3, thus we seek for substring of type ;CHX;
        if (std::string(m_Driver->m_sActiveAdditionalChannels).find(";" + std::string(tmsiAccess->getChannelName(actualIdx).toASCIIString()) + ";")
            != std::string::npos) { gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttonChannelActive), TRUE); }
        else { gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttonChannelActive), FALSE); }

        gtk_table_attach_defaults(m_TableAdditionalChannels, labelChannelName, 0, 1, guint(c), guint(c + 1));
        gtk_table_attach_defaults(m_TableAdditionalChannels, buttonChannelActive, 1, 2, guint(c), guint(c + 1));
    }

    gtk_widget_show(GTK_WIDGET(m_LabelAdditionalChannels));
    gtk_widget_show_all(GTK_WIDGET(m_TableAdditionalChannels));
}


CString CConfigurationTMSi::getActiveAdditionalChannels()
{
    CString additionalChannelsString = ";";

    CTMSiAccess* tmsiAccess = m_Driver->m_pTMSiAccess;

    const uint32_t nAdditionalChannels = tmsiAccess->getActualChannelCount() - tmsiAccess->getMaximumEEGChannelCount();

    for (size_t i = 0; i < nAdditionalChannels; i++) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_AdditionalChannelCheckButtons[i]))) {
            additionalChannelsString = additionalChannelsString + m_additionalChannelNames[i] + ";";
        }
    }

    return additionalChannelsString;
}

CConfigurationTMSi::CConfigurationTMSi(const char* gtkBuilderFilename, CDriverTMSi* driver)
    : CConfigurationBuilder(gtkBuilderFilename), m_Driver(driver) { m_waitWindow = nullptr; }

CConfigurationTMSi::~CConfigurationTMSi()
{
    // Hide the wait window when the object dies
    hideWaitWindow();
}

bool CConfigurationTMSi::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    m_buttonChannelCount         = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels"));
    m_comboBoxSamplingFrequency  = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));
    m_comboBoxConnectionProtocol = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_tmsi_connection_protocol"));
    m_comboBoxDeviceID           = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_tmsi_device"));
    m_comboBoxImpedanceLimit     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_tmsi_impedance_limit"));
    //  m_pToggleButtonCommonAverageReference = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_tmsi_average_reference"));
    m_LabelAdditionalChannels = GTK_LABEL(gtk_builder_get_object(m_builder, "label_tmsi_additional_channels"));
    m_TableAdditionalChannels = GTK_TABLE(gtk_builder_get_object(m_builder, "table_tmsi_additional_channels"));

    g_signal_connect(G_OBJECT(m_comboBoxConnectionProtocol), "changed", G_CALLBACK(ComboBoxCommunicationProtocolChangedCB), this);
    g_signal_connect(G_OBJECT(m_comboBoxDeviceID), "changed", G_CALLBACK(ComboBoxDeviceChangedCB), this);
    g_signal_connect(G_OBJECT(m_buttonChannelCount), "changed", G_CALLBACK(ChannelCountChangedCB), this);

    // The order of the drivers corresponds to indexes in the ConnectionProtocols map inside the driver
    gtk_combo_box_append_text(m_comboBoxConnectionProtocol, "USB");
    gtk_combo_box_append_text(m_comboBoxConnectionProtocol, "WiFi");
    gtk_combo_box_append_text(m_comboBoxConnectionProtocol, "Network");
    gtk_combo_box_append_text(m_comboBoxConnectionProtocol, "Bluetooth");

    // Get the index of the currently set connection protocol from the map
    gtk_combo_box_set_active(m_comboBoxConnectionProtocol, m_Driver->m_pTMSiAccess->getConnectionProtocols()[m_Driver->m_sConnectionProtocol].second);

    gtk_combo_box_set_active(m_comboBoxImpedanceLimit, gint(m_Driver->m_impedanceLimit));

    //  gtk_toggle_button_set_active(m_pToggleButtonCommonAverageReference, m_Driver->m_bCommonAverageReference);
    gtk_spin_button_set_value(m_buttonChannelCount, double(m_Driver->m_activeEEGChannels));

    return true;
}

bool CConfigurationTMSi::postConfigure()
{
    if (m_applyConfig) {
        // set the channel count to the number of EEG channels + number of active additional channels
        CTMSiAccess* tmsiAccess = m_Driver->m_pTMSiAccess;

        m_Driver->m_sConnectionProtocol = gtk_combo_box_get_active_text(m_comboBoxConnectionProtocol);
        m_Driver->m_deviceID            = gtk_combo_box_get_active_text(m_comboBoxDeviceID);

        // If the device was set to an actual existing device
        if (m_Driver->m_deviceID != CString("")) {
            //          m_Driver->m_bCommonAverageReference = gtk_toggle_button_get_active(m_pToggleButtonCommonAverageReference) != 0;
            m_Driver->m_sActiveAdditionalChannels = getActiveAdditionalChannels();
            m_Driver->m_activeEEGChannels         = gtk_spin_button_get_value_as_int(m_buttonChannelCount);
            m_Driver->m_impedanceLimit            = gtk_combo_box_get_active(m_comboBoxImpedanceLimit);
        }
        // If the device is unset then set all driver settings to neutral values
        else {
            //          m_Driver->m_bCommonAverageReference = 0;
            m_Driver->m_sActiveAdditionalChannels = CString(";");
            m_Driver->m_activeEEGChannels         = 0;
            m_Driver->m_impedanceLimit            = 1;
        }

        // Increase the Channel Count spin button to the value of all active channels (EEG+additional)
        uint32_t activeChannels = gtk_spin_button_get_value_as_int(m_buttonChannelCount);

        for (size_t idx = 0; idx < m_AdditionalChannelCheckButtons.size(); idx++) {
            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_AdditionalChannelCheckButtons[idx]))) { activeChannels++; }
        }

        GtkAdjustment* adjustment = gtk_spin_button_get_adjustment(m_buttonChannelCount);
        gtk_adjustment_set_upper(adjustment, tmsiAccess->getActualChannelCount());
        gtk_adjustment_set_value(adjustment, activeChannels);
    }
    if (!CConfigurationBuilder::postConfigure()) { return false; }

    return true;
}

void CConfigurationTMSi::showWaitWindow()
{
    if (m_waitWindow != nullptr) { return; }

    m_waitWindow = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_position(GTK_WINDOW(m_waitWindow), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(m_waitWindow), FALSE);
    gtk_window_set_modal(GTK_WINDOW(m_waitWindow), TRUE);
    GdkColor bgColor;
    bgColor.blue  = 0;
    bgColor.green = 0xffff;
    bgColor.red   = 0xffff;
    gtk_widget_modify_bg(m_waitWindow, GTK_STATE_NORMAL, &bgColor);
    GtkWidget* box   = gtk_vbox_new(FALSE, 0);
    GtkWidget* label = gtk_label_new("  COMMUNICATING WITH DEVICE...  ");
    gtk_box_pack_end(GTK_BOX(box), label, TRUE, TRUE, 10);

    gtk_container_add(GTK_CONTAINER(m_waitWindow), box);

    gtk_widget_show_all(m_waitWindow);
    while (gtk_events_pending()) { gtk_main_iteration(); }
}

void CConfigurationTMSi::hideWaitWindow()
{
    if (m_waitWindow != nullptr) {
        gtk_widget_destroy(GTK_WIDGET(m_waitWindow));
        m_waitWindow = nullptr;
    }
}

#endif // TARGET_HAS_ThirdPartyTMSi

}  // namespace AcquisitionServer
}  // namespace OpenViBE
