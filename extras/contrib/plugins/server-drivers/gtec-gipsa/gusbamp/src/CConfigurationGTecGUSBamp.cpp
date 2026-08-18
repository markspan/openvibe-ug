#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

#include "CConfigurationGTecGUSBamp.hpp"

#include <windows.h>
#include <gUSBamp.h>
#include <iostream>
#include <sstream>

namespace OpenViBE {
namespace AcquisitionServer {

static void ApplyFiltersPressedCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationGTecGUSBamp*>(data)->buttonFiltersApplyPressedCB(); }
static void CalibratePressedCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationGTecGUSBamp*>(data)->buttonCalibratePressedCB(); }
static void CommonGndRefPressedCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationGTecGUSBamp*>(data)->buttonCommonGndRefPressedCB(); }
static void FiltersPressedCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationGTecGUSBamp*>(data)->buttonFiltersPressedCB(); }

static gboolean idle_calibrate_cb(void* data)
{
    static_cast<CConfigurationGTecGUSBamp*>(data)->idleCalibrateCB();
    return FALSE;
}

CConfigurationGTecGUSBamp::CConfigurationGTecGUSBamp(const char* gtkBuilderFilename, uint8_t& commonGndAndRefBitmap, int& notchFilterIdx,
                                                     int& bandPassFilterIdx, bool& triggerInput, const std::vector<std::string>& devicesSerials,
                                                     std::string& masterDeviceIndex, bool& bipolar, bool& calibrationSignalEnabled, bool& showDeviceName)
    : CConfigurationBuilder(gtkBuilderFilename), m_commonGndAndRefBitmap(commonGndAndRefBitmap), m_notchFilterIdx(notchFilterIdx),
      m_bandPassFilterIdx(bandPassFilterIdx), m_triggerInput(triggerInput), m_devicesSerials(devicesSerials), m_masterDeviceIdx(masterDeviceIndex),
      m_bipolarEnabled(bipolar), m_calibrationSignalEnabled(calibrationSignalEnabled), m_showDeviceName(showDeviceName) {}

bool CConfigurationGTecGUSBamp::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkCheckButton* hardwareTagging = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_EventChannel"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hardwareTagging), m_triggerInput);

    GtkCheckButton* bipolar = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_Bipolar"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bipolar), m_bipolarEnabled);

    GtkCheckButton* showDeviceName = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_ShowDeviceName"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showDeviceName), m_showDeviceName);

    GtkCheckButton* calibrationMode = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_CalibrationSignal"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(calibrationMode), m_calibrationSignalEnabled);

    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_master_device"));

    // Default active device is the last one
    if (!m_devicesSerials.empty()) { gtk_combo_box_set_active(comboBox, m_devicesSerials.size() - 1); }

    // If a device is already set, try to use that as the combo box selection; if not found, use last (set above).
    int masterIndex = int(m_devicesSerials.size()) - 1;
    for (size_t i = 0; i < m_devicesSerials.size(); ++i) {
        gtk_combo_box_append_text(comboBox, m_devicesSerials[i].c_str());
        if (this->m_masterDeviceIdx == m_devicesSerials[i]) { masterIndex = i; }
    }

    if (masterIndex >= 0) { gtk_combo_box_set_active(comboBox, masterIndex); }

    // Sets the channel limits depending on the number of amps
    GtkSpinButton* numChannels = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels"));
    gtk_spin_button_set_range(numChannels, 1, m_devicesSerials.size() * 16);    // GTEC_NUM_CHANNELS

    const size_t count = this->m_devicesSerials.size();

    /*
    char buffer[1024];
    int count=0;
    bool selected=false;*/

    // autodetection of the connected device
    /*for (uint32_t i=1; i<11; ++i)
    {
        ::HANDLE handle=::GT_OpenDevice(i);
        if(handle)
        {
            ::GT_CloseDevice(&handle);
    
            sprintf(buffer, "USB port %i", i);
            ::gtk_combo_box_append_text(comboBox, buffer);
            if(m_usbIdx==i)
            {
                ::gtk_combo_box_set_active(comboBox, count);
                selected=true;
            }
            count++;
        }
    }
    */

    //if(!selected && count!=0) { ::gtk_combo_box_set_active(comboBox, 0); }

    g_signal_connect(gtk_builder_get_object(m_builder, "button_calibrate"), "pressed", G_CALLBACK(CalibratePressedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button-common-gnd-ref"), "pressed", G_CALLBACK(CommonGndRefPressedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button-filters"), "pressed", G_CALLBACK(FiltersPressedCB), this);

    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog-common-gnd-ref"));
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog-filters"));

    GtkWidget* buttonApplyFilters = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
    g_signal_connect(buttonApplyFilters, "pressed", G_CALLBACK(ApplyFiltersPressedCB), this);

    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

    GtkToggleButton* checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockA"));
    gtk_toggle_button_set_active(checkBox, (m_commonGndAndRefBitmap & 1));
    checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockB"));
    gtk_toggle_button_set_active(checkBox, (m_commonGndAndRefBitmap & (1 << 1)));
    checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockC"));
    gtk_toggle_button_set_active(checkBox, (m_commonGndAndRefBitmap & (1 << 2)));
    checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockD"));
    gtk_toggle_button_set_active(checkBox, (m_commonGndAndRefBitmap & (1 << 3)));

    checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockA"));
    gtk_toggle_button_set_active(checkBox, (m_commonGndAndRefBitmap & (1 << 4)));
    checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockB"));
    gtk_toggle_button_set_active(checkBox, (m_commonGndAndRefBitmap & (1 << 5)));
    checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockC"));
    gtk_toggle_button_set_active(checkBox, (m_commonGndAndRefBitmap & (1 << 6)));
    checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockD"));
    gtk_toggle_button_set_active(checkBox, (m_commonGndAndRefBitmap & (1 << 7)));


    if (count == 0) {
        // deactivate the buttons
        GtkWidget* button = GTK_WIDGET(gtk_builder_get_object(m_builder, "button-filters"));
        gtk_widget_set_sensitive(button, false);
        button = GTK_WIDGET(gtk_builder_get_object(m_builder, "button-common-gnd-ref"));
        gtk_widget_set_sensitive(button, false);
    }
    return true;
}

bool CConfigurationGTecGUSBamp::postConfigure()
{
    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_master_device"));

    if (m_applyConfig) {
        /*
        int usbIdx=0;
        const char* usbIdx=::gtk_combo_box_get_active_text(comboBox);
        if(usbIdx) { if(sscanf(usbIdx, "USB port %i", &usbIdx)==1) { m_usbIdx=(uint32_t)usbIdx; } }
        */
        if (this->m_devicesSerials.size() > 1) {
            char* selectedSerial = gtk_combo_box_get_active_text(comboBox);
            m_masterDeviceIdx    = (selectedSerial == nullptr) ? "" : selectedSerial;
        }

        GtkToggleButton* checkBox = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockA"));
        m_commonGndAndRefBitmap   = (gtk_toggle_button_get_active(checkBox) ? 1 : 0);
        checkBox                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockB"));
        m_commonGndAndRefBitmap   = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(checkBox) ? (1 << 1) : 0);
        checkBox                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockC"));
        m_commonGndAndRefBitmap   = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(checkBox) ? (1 << 2) : 0);
        checkBox                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockD"));
        m_commonGndAndRefBitmap   = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(checkBox) ? (1 << 3) : 0);

        checkBox                = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockA"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(checkBox) ? (1 << 4) : 0);
        checkBox                = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockB"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(checkBox) ? (1 << 5) : 0);
        checkBox                = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockC"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(checkBox) ? (1 << 6) : 0);
        checkBox                = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockD"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(checkBox) ? (1 << 7) : 0);

        GtkComboBox* notch = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-notch"));
        if (gtk_combo_box_get_active(notch) >= 0) {
            // Only update the filter index if the user chose something. This is needed so going to the Configuration menu doesn't
            // change previous filter choices.
            m_notchFilterIdx = ((gtk_combo_box_get_active(notch) == 0) ? -1 : m_comboBoxNotchFilterIdx[gtk_combo_box_get_active(notch) - 1]
            ); //-1 because there is one more in the beginning
        }
        GtkComboBox* bandPass = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-band-pass"));
        if (gtk_combo_box_get_active(bandPass) >= 0) {
            // Only update the filter index if the user chose something -1 because there is one more in the beginning
            m_bandPassFilterIdx = ((gtk_combo_box_get_active(bandPass) == 0) ? -1 : int(m_comboBoxBandPassFilterIdx[gtk_combo_box_get_active(bandPass) - 1]));
        }
        GtkCheckButton* hardwareTagging = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_EventChannel"));
        m_triggerInput                  = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hardwareTagging)) ? true : false);

        GtkCheckButton* showDeviceName = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_ShowDeviceName"));
        m_showDeviceName               = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(showDeviceName)) ? true : false);

        GtkCheckButton* bipolar = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_Bipolar"));
        m_bipolarEnabled        = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bipolar)) ? true : false);

        GtkCheckButton* calibrationSignal = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_CalibrationSignal"));
        m_calibrationSignalEnabled        = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(calibrationSignal)) ? true : false);
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

void CConfigurationGTecGUSBamp::buttonCalibratePressedCB()
{
    g_idle_add(idle_calibrate_cb, this);

    m_calibrateDialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "Calibrating...");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(m_calibrateDialog), "Please wait a few seconds...");
    gtk_dialog_run(GTK_DIALOG(m_calibrateDialog));
    gtk_widget_destroy(m_calibrateDialog);

    if (m_calibrationDone) {
        GtkWidget* dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "Calibration finished !");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    else {
        GtkWidget* dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "Calibration failed !");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

void CConfigurationGTecGUSBamp::idleCalibrateCB()
{
    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_master_device"));

    m_calibrationDone = false;

    //calibrate all detected devices
    for (uint32_t i = 0; i < this->m_devicesSerials.size(); ++i) {
        LPSTR serial  = const_cast<char*>(m_devicesSerials[i].c_str());
        HANDLE handle = GT_OpenDeviceEx(serial);
        if (handle) {
            m_calibrationDone = true;

            SCALE calibration;
            if (!GT_Calibrate(handle, &calibration)) {
                std::cout << "err GT_Calibrate\n";
                m_calibrationDone = false;
            }
            if (!GT_SetScale(handle, &calibration)) {
                std::cout << "err GT_SetScale\n";
                m_calibrationDone = false;
            }
            GT_CloseDevice(&handle);
        }
    }

    gtk_dialog_response(GTK_DIALOG(m_calibrateDialog), 0);
}

void CConfigurationGTecGUSBamp::buttonCommonGndRefPressedCB()
{
    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog-common-gnd-ref"));
    gint resp         = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
}

void CConfigurationGTecGUSBamp::buttonFiltersPressedCB()
{
    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog-filters"));
    setHardwareFiltersDialog();
    gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
}

void CConfigurationGTecGUSBamp::setHardwareFiltersDialog()
{
    m_comboBoxBandPassFilterIdx.clear();
    m_comboBoxNotchFilterIdx.clear();

    GtkComboBox* comboBoxBandPass   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-band-pass"));
    GtkTreeModel* bandPassListStore = gtk_combo_box_get_model(comboBoxBandPass);
    gtk_list_store_clear(GTK_LIST_STORE(bandPassListStore));

    GtkComboBox* comboBoxNotch   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-notch"));
    GtkTreeModel* notchListStore = gtk_combo_box_get_model(comboBoxNotch);
    gtk_list_store_clear(GTK_LIST_STORE(notchListStore));

    // To check for available filters in the amplifier, we must connect to it.
    if (!m_devicesSerials[0].empty()) {
        LPSTR serial  = const_cast<char*>(m_devicesSerials[0].c_str());
        HANDLE handle = GT_OpenDeviceEx(serial);

        int nBandPassFilters, nNotchFilters;
        if (!GT_GetNumberOfFilter(&nBandPassFilters)) { std::cout << "err GT_GetNumberOfFilter\n"; }
        if (nBandPassFilters == 0) { std::cout << "err No band pass filters found at all!\n"; }
        if (!GT_GetNumberOfNotch(&nNotchFilters)) { std::cout << "err GT_GetNumberOfNotch\n"; }
        if (nNotchFilters == 0) { std::cout << "err No notch filters found at all!\n"; }

        FILT* bpFilterSpec    = new FILT[nBandPassFilters];
        FILT* notchFilterSpec = new FILT[nNotchFilters];

        if (!GT_GetFilterSpec(bpFilterSpec)) { std::cout << "err GT_GetFilterSpec\n"; }
        if (!GT_GetNotchSpec(notchFilterSpec)) { std::cout << "err GT_GetNotchSpec\n"; }

        //Set BandPass filter list

        std::stringstream desc;
        desc << "no band pass filter.";
        GtkTreeIter it;
        gtk_list_store_append(GTK_LIST_STORE(bandPassListStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(bandPassListStore), &it, 0, desc.str().c_str(), -1);
        desc.clear();
        desc.str("");

        gchar* sSampling        = gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_sampling));
        const uint32_t sampling = (sSampling ? atoi(sSampling) : 0);

        int cbBandPassSelectedIdx = -1;

        // std::cout << "The device reports " << nBandPassFilters << " band pass filters and " << nNotchFilters << " notch filters\n";

        if (nBandPassFilters > 0) {
            for (int i = 0; i < nBandPassFilters; ++i) {
                if (sampling == uint32_t(bpFilterSpec[i].fs)) {
                    if (bpFilterSpec[i].type == 1) { desc << "Butterworth - "; }
                    if (bpFilterSpec[i].type == 2) { desc << "Chebyshev   - "; }
                    desc << bpFilterSpec[i].order << " - [" << bpFilterSpec[i].fu << "; " << bpFilterSpec[i].fo << "] - " << bpFilterSpec[i].fs;
                    GtkTreeIter iter;
                    gtk_list_store_append(GTK_LIST_STORE(bandPassListStore), &iter);
                    gtk_list_store_set(GTK_LIST_STORE(bandPassListStore), &iter, 0, desc.str().c_str(), -1);
                    desc.clear();
                    desc.str("");

                    m_comboBoxBandPassFilterIdx.push_back(i);

                    //here a previous selection is loaded
                    if (cbBandPassSelectedIdx == -1 && i == m_bandPassFilterIdx) { cbBandPassSelectedIdx = int(m_comboBoxBandPassFilterIdx.size()) - 1; }
                }
            }
        }
        gtk_combo_box_set_active(comboBoxBandPass, cbBandPassSelectedIdx + 1); // +1 because -1 is for "no filter".


        //Set Notch filter List

        desc << "no notch filter.";
        gtk_list_store_append(GTK_LIST_STORE(notchListStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(notchListStore), &it, 0, desc.str().c_str(), -1);
        desc.clear();
        desc.str("");

        int cbNotchSelectedIndex = -1;

        if (nNotchFilters > 0) {
            for (int i = 0; i < nNotchFilters; ++i) {
                if (sampling == int(notchFilterSpec[i].fs)) {
                    if (notchFilterSpec[i].type == 1) { desc << "Butterworth - "; }
                    if (notchFilterSpec[i].type == 2) { desc << "Chebyshev   - "; }
                    desc << notchFilterSpec[i].order << " - [" << notchFilterSpec[i].fu << "; " << notchFilterSpec[i].fo << "] - " << notchFilterSpec[i].fs;
                    GtkTreeIter iter;
                    gtk_list_store_append(GTK_LIST_STORE(notchListStore), &iter);
                    gtk_list_store_set(GTK_LIST_STORE(notchListStore), &iter, 0, desc.str().c_str(), -1);
                    desc.clear();
                    desc.str("");

                    m_comboBoxNotchFilterIdx.push_back(i);

                    //here a previous selection is loaded
                    if (cbNotchSelectedIndex == -1 && i == m_notchFilterIdx) { cbNotchSelectedIndex = int(m_comboBoxNotchFilterIdx.size()) - 1; }
                }
            }
        }
        gtk_combo_box_set_active(comboBoxNotch, cbNotchSelectedIndex + 1); // +1 because -1 is for "no filter".

        delete bpFilterSpec;
        delete notchFilterSpec;
        GT_CloseDevice(&handle);
    }
}

void CConfigurationGTecGUSBamp::buttonFiltersApplyPressedCB()
{
    GtkComboBox* comboBoxBandPass = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-band-pass"));

    //-1 because there is one more in the beginning
    m_bandPassFilterIdx = (gtk_combo_box_get_active(comboBoxBandPass) == -1 || gtk_combo_box_get_active(comboBoxBandPass) == 0) ? -1
                              : int(m_comboBoxBandPassFilterIdx[gtk_combo_box_get_active(comboBoxBandPass) - 1]);

    GtkComboBox* comboBoxNotch = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-notch"));
    //-1 because there is one more in the beginning
    m_notchFilterIdx = (gtk_combo_box_get_active(comboBoxNotch) == -1 || gtk_combo_box_get_active(comboBoxNotch) == 0) ? -1
                           : int(m_comboBoxNotchFilterIdx[gtk_combo_box_get_active(comboBoxNotch) - 1]);
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
