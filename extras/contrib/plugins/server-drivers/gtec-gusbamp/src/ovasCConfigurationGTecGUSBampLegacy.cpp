#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

#include "ovasCConfigurationGTecGUSBampLegacy.h"

#include <windows.h>
#include <gUSBamp.h>
#include <iostream>
#include <sstream>

namespace OpenViBE {
namespace AcquisitionServer {

static void CalibratePressedCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationGTecGUSBampLegacy*>(data)->buttonCalibratePressedCB(); }

static gboolean idle_calibrate_cb(void* data)
{
    static_cast<CConfigurationGTecGUSBampLegacy*>(data)->idleCalibrateCB();
    return FALSE;
}

static void button_common_gnd_ref_pressed_cb(GtkButton* /*button*/, void* data)
{
    static_cast<CConfigurationGTecGUSBampLegacy*>(data)->buttonCommonGndRefPressedCB();
}

static void FiltersPressedCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationGTecGUSBampLegacy*>(data)->buttonFiltersPressedCB(); }

CConfigurationGTecGUSBampLegacy::CConfigurationGTecGUSBampLegacy(const char* gtkBuilderFilename, uint32_t& usbIdx, uint8_t& commonGndAndRefBitmap,
                                                                 int& notchFilterIdx, int& bandPassFilterIdx, bool& triggerInput)
    : CConfigurationBuilder(gtkBuilderFilename), m_usbIdx(usbIdx), m_commonGndAndRefBitmap(commonGndAndRefBitmap), m_notchFilterIdx(notchFilterIdx),
      m_bandPassFilterIdx(bandPassFilterIdx), m_triggerInput(triggerInput) {}

bool CConfigurationGTecGUSBampLegacy::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkCheckButton* hardwareTagging = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_EventChannel"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hardwareTagging), m_triggerInput);

    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    char buffer[1024];
    int count     = 0;
    bool selected = false;

    // autodetection of the connected device
    for (uint32_t i = 1; i < 11; ++i) {
        HANDLE handle = GT_OpenDevice(i);
        if (handle) {
            GT_CloseDevice(&handle);

            sprintf(buffer, "USB port %i", i);
            gtk_combo_box_append_text(comboBox, buffer);
            if (m_usbIdx == i) {
                gtk_combo_box_set_active(comboBox, count);
                selected = true;
            }
            count++;
        }
    }

    g_signal_connect(gtk_builder_get_object(m_builder, "button_calibrate"), "pressed", G_CALLBACK(CalibratePressedCB), this);

    if (!selected && count != 0) { gtk_combo_box_set_active(comboBox, 0); }

    g_signal_connect(gtk_builder_get_object(m_builder, "button-common-gnd-ref"), "pressed", G_CALLBACK(button_common_gnd_ref_pressed_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button-filters"), "pressed", G_CALLBACK(FiltersPressedCB), this);

    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog-common-gnd-ref"));
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

    dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog-filters"));
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
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

    // To check for available filters in the amplifier, we must connect to it.
    if (count != 0) {
        HANDLE handle = GT_OpenDevice(count);

        int nBandPassFilters, nNotchFilters;
        if (!GT_GetNumberOfFilter(&nBandPassFilters)) { std::cout << "err GT_GetNumberOfFilter\n"; }
        if (!GT_GetNumberOfNotch(&nNotchFilters)) { std::cout << "err GT_GetNumberOfNotch\n"; }

        FILT* bpFilterSpec    = new FILT[nBandPassFilters];
        FILT* notchFilterSpec = new FILT[nNotchFilters];

        if (!GT_GetFilterSpec(bpFilterSpec)) { std::cout << "err GT_GetFilterSpec\n"; }
        if (!GT_GetNotchSpec(notchFilterSpec)) { std::cout << "err GT_GetNotchSpec\n"; }

        GtkComboBox* comboBoxBandPass   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-band-pass"));
        GtkTreeModel* bandPassListStore = gtk_combo_box_get_model(comboBoxBandPass);
        std::stringstream desc;
        desc << "no band pass filter.";
        GtkTreeIter lIter;
        gtk_list_store_append(GTK_LIST_STORE(bandPassListStore), &lIter);
        gtk_list_store_set(GTK_LIST_STORE(bandPassListStore), &lIter, 0, desc.str().c_str(), -1);
        desc.clear();
        desc.str("");
        for (int i = 0; i < nBandPassFilters; ++i) {
            if (bpFilterSpec[i].type == 1) { desc << "Butterworth - "; }
            if (bpFilterSpec[i].type == 2) { desc << "Chebyshev   - "; }
            desc << bpFilterSpec[i].order << " - [" << bpFilterSpec[i].fu << "; " << bpFilterSpec[i].fo << "] - " << bpFilterSpec[i].fs;
            GtkTreeIter iter;
            gtk_list_store_append(GTK_LIST_STORE(bandPassListStore), &iter);
            gtk_list_store_set(GTK_LIST_STORE(bandPassListStore), &iter, 0, desc.str().c_str(), -1);
            desc.clear();
            desc.str("");
        }
        gtk_combo_box_set_active(comboBoxBandPass, m_bandPassFilterIdx + 1); // +1 because -1 is for "no filter".

        GtkComboBox* comboBoxNotch   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-notch"));
        GtkTreeModel* notchListStore = gtk_combo_box_get_model(comboBoxNotch);
        desc << "no notch filter.";
        gtk_list_store_append(GTK_LIST_STORE(notchListStore), &lIter);
        gtk_list_store_set(GTK_LIST_STORE(notchListStore), &lIter, 0, desc.str().c_str(), -1);
        desc.clear();
        desc.str("");
        for (int i = 0; i < nNotchFilters; ++i) {
            if (notchFilterSpec[i].type == 1) { desc << "Butterworth - "; }
            if (notchFilterSpec[i].type == 2) { desc << "Chebyshev   - "; }
            desc << notchFilterSpec[i].order << " - [" << notchFilterSpec[i].fu << "; " << notchFilterSpec[i].fo << "] - " << notchFilterSpec[i].fs;
            GtkTreeIter iter;
            gtk_list_store_append(GTK_LIST_STORE(notchListStore), &iter);
            gtk_list_store_set(GTK_LIST_STORE(notchListStore), &iter, 0, desc.str().c_str(), -1);
            desc.clear();
            desc.str("");
        }
        gtk_combo_box_set_active(comboBoxNotch, m_notchFilterIdx + 1);

        delete bpFilterSpec;
        delete notchFilterSpec;
        GT_CloseDevice(&handle);
    }
    else {
        // deactivate the buttons
        GtkWidget* button = GTK_WIDGET(gtk_builder_get_object(m_builder, "button-filters"));
        gtk_widget_set_sensitive(button, false);
        button = GTK_WIDGET(gtk_builder_get_object(m_builder, "button-common-gnd-ref"));
        gtk_widget_set_sensitive(button, false);
    }
    return true;
}

bool CConfigurationGTecGUSBampLegacy::postConfigure()
{
    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    if (m_applyConfig) {
        int iUsbIdx        = 0;
        const char* usbIdx = gtk_combo_box_get_active_text(comboBox);
        if (usbIdx) { if (sscanf(usbIdx, "USB port %i", &iUsbIdx) == 1) { m_usbIdx = uint32_t(iUsbIdx); } }

        GtkToggleButton* button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockA"));
        m_commonGndAndRefBitmap = (gtk_toggle_button_get_active(button) ? 1 : 0);
        button                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockB"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(button) ? (1 << 1) : 0);
        button                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockC"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(button) ? (1 << 2) : 0);
        button                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-gnd-blockD"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(button) ? (1 << 3) : 0);
        button                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockA"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(button) ? (1 << 4) : 0);
        button                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockB"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(button) ? (1 << 5) : 0);
        button                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockC"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(button) ? (1 << 6) : 0);
        button                  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton-ref-blockD"));
        m_commonGndAndRefBitmap = m_commonGndAndRefBitmap + (gtk_toggle_button_get_active(button) ? (1 << 7) : 0);

        GtkComboBox* notch    = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-notch"));
        m_notchFilterIdx      = gtk_combo_box_get_active(notch) - 1;
        GtkComboBox* bandPass = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox-band-pass"));
        m_bandPassFilterIdx   = gtk_combo_box_get_active(bandPass) - 1;

        GtkCheckButton* hardwareTagging = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_EventChannel"));
        m_triggerInput                  = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hardwareTagging)) ? true : false);
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

void CConfigurationGTecGUSBampLegacy::buttonCalibratePressedCB()
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

void CConfigurationGTecGUSBampLegacy::idleCalibrateCB()
{
    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    m_calibrationDone  = false;
    int iUsbIdx        = 0;
    const char* usbIdx = gtk_combo_box_get_active_text(comboBox);

    if (usbIdx) {
        if (sscanf(usbIdx, "USB port %i", &iUsbIdx) == 1) {
            HANDLE handle = GT_OpenDevice(iUsbIdx);
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
    }

    gtk_dialog_response(GTK_DIALOG(m_calibrateDialog), 0);
}

void CConfigurationGTecGUSBampLegacy::buttonCommonGndRefPressedCB()
{
    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog-common-gnd-ref"));
    gint resp         = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
}

void CConfigurationGTecGUSBampLegacy::buttonFiltersPressedCB()
{
    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog-filters"));
    gint resp         = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
