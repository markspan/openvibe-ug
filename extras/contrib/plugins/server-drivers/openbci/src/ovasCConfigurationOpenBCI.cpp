/*
 * OpenBCI driver for OpenViBE
 *
 * \author Jeremy Frey
 * \author Yann Renard
 *
 */
#include "ovasCConfigurationOpenBCI.h"
#include <algorithm>
#include <string>

#if defined TARGET_OS_Windows
#include <windows.h>
#include <winbase.h>
#include <cstdio>
#include <commctrl.h>
//#define TERM_SPEED 57600
#define TERM_SPEED CBR_115200 // OpenBCI is a bit faster than others
#elif defined TARGET_OS_Linux
 #include <cstdio>
 #include <unistd.h>
 #include <fcntl.h>
 #include <termios.h>
 #include <sys/select.h>
 #include <netinet/in.h> // htons and co.
 #include <unistd.h>
 #define TERM_SPEED B115200
#else
#endif


namespace OpenViBE {
namespace AcquisitionServer {

#define MAXIMUM_SERIAL_TTY     (32)
#define MAXIMUM_SERIAL_USB_TTY (256-MAXIMUM_SERIAL_TTY)

uint32_t CConfigurationOpenBCI::getMaximumTtyCount() { return MAXIMUM_SERIAL_USB_TTY + MAXIMUM_SERIAL_TTY; }

CString CConfigurationOpenBCI::getTTYFileName(const uint32_t ttyNumber)
{
    char buffer[1024];
#if defined TARGET_OS_Windows
    sprintf(buffer, "\\\\.\\COM%u", ttyNumber);
#elif defined TARGET_OS_Linux
    if(ttyNumber<MAXIMUM_SERIAL_USB_TTY)
    {
        ::sprintf(buffer, "/dev/ttyUSB%u", ttyNumber);
    }
    else
    {
        ::sprintf(buffer, "/dev/ttyS%u", ttyNumber-MAXIMUM_SERIAL_USB_TTY);
    }
#else
    ::sprintf(buffer, "");
#endif
    return buffer;
}

bool CConfigurationOpenBCI::isTTYFile(const CString& filename)
{
#if defined TARGET_OS_Windows
    HANDLE file = ::CreateFile(LPCSTR(filename), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE || file == nullptr) { return false; }
    CloseHandle(file);
    return true;
#elif defined TARGET_OS_Linux
    int file=::open(filename, O_RDONLY);
    if(file < 0) { return false; }
    close(file);
    return true;
#else
    // Caution, this path will claim all serial ports do exist because there was no platform specific implementation
    return true;
#endif
}

static void checkbutton_daisy_module_cb(GtkToggleButton* button, CConfigurationOpenBCI* data)
{
    data->checkbuttonDaisyModuleCB(gtk_toggle_button_get_active(button) ? CConfigurationOpenBCI::EDaisyStatus::Active
                                       : CConfigurationOpenBCI::EDaisyStatus::Inactive);
}

CConfigurationOpenBCI::CConfigurationOpenBCI(const char* gtkBuilderFilename, uint32_t& usbIdx)
    : CConfigurationBuilder(gtkBuilderFilename), m_usbIdx(usbIdx) { m_listStore = gtk_list_store_new(1, G_TYPE_STRING); }

CConfigurationOpenBCI::~CConfigurationOpenBCI() { g_object_unref(m_listStore); }

bool CConfigurationOpenBCI::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

#if 0
    ::GtkEntry* m_entryComInit=GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_com_init"));
    ::gtk_entry_set_text(m_entryComInit, m_additionalCmds.toASCIIString());
#else
    std::string additionalCmds = m_additionalCmds.toASCIIString();
    std::replace(additionalCmds.begin(), additionalCmds.end(), '\255', '\n');
    GtkTextView* textViewComInit     = GTK_TEXT_VIEW(gtk_builder_get_object(m_builder, "text_view_com_init"));
    GtkTextBuffer* textBufferComInit = gtk_text_view_get_buffer(textViewComInit);
    gtk_text_buffer_set_text(textBufferComInit, additionalCmds.c_str(), -1);
#endif

    GtkSpinButton* buttonReadBoardReplyTimeout = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_read_board_reply_timeout"));
    gtk_spin_button_set_value(buttonReadBoardReplyTimeout, m_readBoardReplyTimeout);

    GtkSpinButton* buttonFlushBoardReplyTimeout = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_flush_board_reply_timeout"));
    gtk_spin_button_set_value(buttonFlushBoardReplyTimeout, m_flushBoardReplyTimeout);

    GtkToggleButton* buttonDaisyModule = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_daisy_module"));
    gtk_toggle_button_set_active(buttonDaisyModule, m_daisyModule ? true : false);

    ::g_signal_connect(gtk_builder_get_object(m_builder, "checkbutton_daisy_module"), "toggled", G_CALLBACK(checkbutton_daisy_module_cb), this);
    this->checkbuttonDaisyModuleCB(m_daisyModule ? EDaisyStatus::Active : EDaisyStatus::Inactive);

    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    g_object_unref(m_listStore);
    m_listStore = gtk_list_store_new(1, G_TYPE_STRING);
    m_comboSlotsIndexToSerialPort.clear();

    gtk_combo_box_set_model(comboBox, GTK_TREE_MODEL(m_listStore));

    bool selected = false;

    m_comboSlotsIndexToSerialPort[0] = -1;
    gtk_combo_box_append_text(comboBox, "Automatic");

    for (uint32_t i = 0, j = 1; i < getMaximumTtyCount(); ++i)
    {
        CString filename = getTTYFileName(i);
        if (isTTYFile(filename))
        {
            m_comboSlotsIndexToSerialPort[j] = i;
            gtk_combo_box_append_text(comboBox, filename.toASCIIString());
            if (m_usbIdx == i)
            {
                gtk_combo_box_set_active(comboBox, j);
                selected = true;
            }
            j++;
        }
    }

    if (!selected) { gtk_combo_box_set_active(comboBox, 0); }

    return true;
}

bool CConfigurationOpenBCI::postConfigure()
{
    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    if (m_applyConfig)
    {
        const int idx = m_comboSlotsIndexToSerialPort[gtk_combo_box_get_active(comboBox)];
        if (idx >= 0) { m_usbIdx = uint32_t(idx); }
        else { m_usbIdx = uint32_t(-1); }

#if 0
        ::GtkEntry* m_entryComInit=GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_com_init"));
        m_additionalCmds=::gtk_entry_get_text(m_entryComInit);
#else
        GtkTextView* textViewComInit     = GTK_TEXT_VIEW(gtk_builder_get_object(m_builder, "text_view_com_init"));
        GtkTextBuffer* textBufferComInit = gtk_text_view_get_buffer(textViewComInit);
        GtkTextIter startIt;
        GtkTextIter endIt;
        gtk_text_buffer_get_start_iter(textBufferComInit, &startIt);
        gtk_text_buffer_get_end_iter(textBufferComInit, &endIt);
        std::string additionalCmds = gtk_text_buffer_get_text(textBufferComInit, &startIt, &endIt, FALSE);
        std::replace(additionalCmds.begin(), additionalCmds.end(), '\n', '\255');
        m_additionalCmds = additionalCmds.c_str();
#endif

        GtkSpinButton* buttonReadBoardReplyTimeout = GTK_SPIN_BUTTON(
            gtk_builder_get_object(m_builder, "spinbutton_read_board_reply_timeout"));
        gtk_spin_button_update(GTK_SPIN_BUTTON(buttonReadBoardReplyTimeout));
        m_readBoardReplyTimeout = gtk_spin_button_get_value_as_int(buttonReadBoardReplyTimeout);

        GtkSpinButton* buttonFlushBoardReplyTimeout = GTK_SPIN_BUTTON(
            gtk_builder_get_object(m_builder, "spinbutton_flush_board_reply_timeout"));
        gtk_spin_button_update(GTK_SPIN_BUTTON(buttonFlushBoardReplyTimeout));
        m_flushBoardReplyTimeout = gtk_spin_button_get_value_as_int(buttonFlushBoardReplyTimeout);

        GtkToggleButton* buttonDaisyModule = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_daisy_module"));
        m_daisyModule                      = gtk_toggle_button_get_active(buttonDaisyModule) ? true : false;
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

void CConfigurationOpenBCI::checkbuttonDaisyModuleCB(const EDaisyStatus status) const
{
    const daisy_Info_t info = this->getDaisyInformation(status);

    std::string buffer = std::to_string(info.nEEGChannel) + " EEG Channels";
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(m_builder, "label_status_eeg_channel_count")), buffer.c_str());

    buffer = std::to_string(info.nAccChannel) + " Accelerometer Channels";
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(m_builder, "label_status_acc_channel_count")), buffer.c_str());

    buffer = std::to_string(info.sampling) + " Hz Sampling Rate";
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(m_builder, "label_status_sampling_rate")), buffer.c_str());

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels")), info.nEEGChannel + info.nAccChannel);
}

CConfigurationOpenBCI::daisy_Info_t CConfigurationOpenBCI::getDaisyInformation(const EDaisyStatus status)
{
    daisy_Info_t res;
    switch (status)
    {
        case EDaisyStatus::Inactive: res.nEEGChannel = DEFAULT_N_EEG_CHANNEL;
            res.nAccChannel = DEFAULT_N_ACC_CHANNEL;
            res.sampling    = DEFAULT_SAMPLING;
            break;

        case EDaisyStatus::Active: res.nEEGChannel = DEFAULT_N_EEG_CHANNEL * 2;
            res.nAccChannel = DEFAULT_N_ACC_CHANNEL;
            res.sampling    = DEFAULT_SAMPLING / 2;
            break;

        default: res.nEEGChannel = 0;
            res.nAccChannel = 0;
            res.sampling    = 0;
            break;
    }

    return res;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
