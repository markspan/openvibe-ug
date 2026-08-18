#include "ovasCConfigurationBrainProductsVAmp.h"
#include "ovasCHeaderBrainProductsVAmp.h"

#if defined TARGET_HAS_ThirdPartyUSBFirstAmpAPI

namespace OpenViBE {
namespace AcquisitionServer {

gboolean idle_check_service(gpointer data)
{
    GtkBuilder* builder = static_cast<GtkBuilder*>(data);

    SC_HANDLE scm     = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE service = nullptr;
    if (scm != nullptr && scm != INVALID_HANDLE_VALUE)
    {
        service = OpenService(scm, "VampService", SERVICE_ALL_ACCESS);
        if (service != nullptr)
        {
            SERVICE_STATUS status;
            QueryServiceStatus(service, &status);

            if (status.dwCurrentState == SERVICE_RUNNING)
            {
                gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_start_service")), false);
                gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_stop_service")), true);
                gtk_label_set(GTK_LABEL(gtk_builder_get_object(builder, "label_service")), "VampService is Enabled");
                gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(builder, "image_service")),GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
                CloseServiceHandle(scm);
                CloseServiceHandle(service);
                return true;
            }
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_start_service")), true);
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_stop_service")), false);
            gtk_label_set(GTK_LABEL(gtk_builder_get_object(builder, "label_service")), "VampService is Disabled");
            gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(builder, "image_service")),GTK_STOCK_APPLY, GTK_ICON_SIZE_BUTTON);
            CloseServiceHandle(scm);
            CloseServiceHandle(service);
            return true;
        }
    }

    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_start_service")), false);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_stop_service")), false);
    gtk_label_set(GTK_LABEL(gtk_builder_get_object(builder, "label_service")), (scm != nullptr && scm != INVALID_HANDLE_VALUE)
                                                                                   ? "VampService has not been Detected"
                                                                                   : "Service Manager not Available (you must be administrator)");
    gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(builder, "image_service")),GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_BUTTON);
    return false;
}
//____________________________________________________________________________________
//
static void StartServiceCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationBrainProductsVAmp*>(data)->buttonStartServiceCB(); }
static void StopServiceCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationBrainProductsVAmp*>(data)->buttonStopServiceCB(); }
static void FastModeSettingsCB(GtkButton* /*button*/, void* data) { static_cast<CConfigurationBrainProductsVAmp*>(data)->buttonFastModeSettingsCB(); }
static void ChannelCountChangedCB(GtkComboBox* /*box*/, void* data) { static_cast<CConfigurationBrainProductsVAmp*>(data)->channelCountChangedCB(); }

static void GTKComboBoxSetActiveText(GtkComboBox* box, const gchar* text)
{
    GtkTreeModel* treeModel = gtk_combo_box_get_model(box);
    GtkTreeIter it;
    int index   = 0;
    gchar* name = nullptr;
    if (gtk_tree_model_get_iter_first(treeModel, &it))
    {
        do
        {
            gtk_tree_model_get(treeModel, &it, 0, &name, -1);
            if (std::string(name) == std::string(text))
            {
                gtk_combo_box_set_active(box, index);
                return;
            }
            index++;
        } while (gtk_tree_model_iter_next(treeModel, &it));
    }
}

//____________________________________________________________________________________

// Inits the combo box, and sets the active value
// Warning : active value must be 7 8 9 10 or -1.
void initFastModeSettingsComboBox(GtkWidget* comboBox, const uint32_t value, const bool initComboBox = true)
{
    if (initComboBox)
    {
        //-1
        gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), "-1");
        //7-10
        for (size_t i = 7; i < 11; ++i)
        {
            const std::string tmp = std::to_string(i);
            gtk_combo_box_append_text(GTK_COMBO_BOX(comboBox), tmp.c_str());
        }
    }
    const std::string tmp = std::to_string(value);
    GTKComboBoxSetActiveText(GTK_COMBO_BOX(comboBox), tmp.c_str());
}

//____________________________________________________________________________________

CConfigurationBrainProductsVAmp::CConfigurationBrainProductsVAmp(IDriverContext& ctx, const char* gtkBuilderFilename,
                                                                 CHeaderBrainProductsVAmp* headerBrainProductsVAmp, bool& acquireAuxiliaryAsEEG,
                                                                 bool& acquireTriggerAsEEG)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_headerBrainProductsVAmp(headerBrainProductsVAmp),
      m_rAcquireAuxiliaryAsEEG(acquireAuxiliaryAsEEG), m_rAcquireTriggerAsEEG(acquireTriggerAsEEG) { m_giIdleID = 0; }

bool CConfigurationBrainProductsVAmp::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    // Finds all the widgets
    m_dialogFastModeSettings = GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog_fast_mode_settings"));

    // the acquisition mode combo box in the main interface
    m_acquisitionMode = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_acquisition_mode"));

    // the device combo box autofilled with all connected device
    m_Device = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_device"));

    // the toggle button for aux channels and triggers
    m_auxiliaryChannels = GTK_WIDGET(gtk_builder_get_object(m_builder, "checkbutton_aux"));
    m_triggerChannels   = GTK_WIDGET(gtk_builder_get_object(m_builder, "checkbutton_trigger"));

    // the 8 spin buttons for the settings in the "fast mode settings" interface
    m_pair1PositiveInputs = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_pair1_positive_input"));
    m_pair1NegativeInputs = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_pair1_negative_input"));

    m_pair2PositiveInputs = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_pair2_positive_input"));
    m_pair2NegativeInputs = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_pair2_negative_input"));

    m_pair3PositiveInputs = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_pair3_positive_input"));
    m_pair3NegativeInputs = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_pair3_negative_input"));

    m_pair4PositiveInputs = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_pair4_positive_input"));
    m_pair4NegativeInputs = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_pair4_negative_input"));

    // hide the Change Channel Names button (for compatibility with original contributor (Mensia Technologies) base code)
#if defined TARGET_Is_Mensia_Acquisition_Server
    gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_change_channel_names")), false);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_change_channel_names")), false);
#endif

    // connects callbacks to buttons
    g_signal_connect(gtk_builder_get_object(m_builder,"button_fast_mode_settings"), "pressed", G_CALLBACK(FastModeSettingsCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder,"combobox_acquisition_mode"), "changed", G_CALLBACK(ChannelCountChangedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder,"checkbutton_aux"), "toggled", G_CALLBACK(ChannelCountChangedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder,"checkbutton_trigger"), "toggled", G_CALLBACK(ChannelCountChangedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder,"button_start_service"), "pressed", G_CALLBACK(StartServiceCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder,"button_stop_service"), "pressed", G_CALLBACK(StopServiceCB), this);

    // start the idle function that checks the VampService
    m_giIdleID = g_idle_add(idle_check_service, m_builder);

    // Configures interface with given values
    //Data mode
    gtk_combo_box_set_active(GTK_COMBO_BOX(m_acquisitionMode), m_headerBrainProductsVAmp->getAcquisitionMode());

    //Device(s)
    uint32_t nDevice  = 0;
    uint32_t nRetries = 0;

    // We try to get the last opened device, (max 5 tries, 500ms sleep between tries)
    while (nRetries++ < 5)
    {
        nDevice = faGetCount(); // Get the last opened Device id.
        if (nDevice < 1) { Sleep(500); }
        else { break; }
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "VAmp Configuration: " << nDevice << " device(s) connected.\n";

    if (nDevice != 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "VAmp Configuration: Device(s) information :\n";

        t_faInformation info;
        GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

        for (uint32_t i = 0; i < nDevice; ++i)
        {
            int deviceId = faGetId(i);
            faGetInformation(deviceId, &info);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "     device#" << i << " > Model(" << (info.Model == 1 ? "VAmp 16" : "VAmp 8") << ") | SN(" << info
                    .SerialNumber << ")\n";
            const std::string buffer = "device#" + std::to_string(i);
            gtk_combo_box_append_text(comboBox, buffer.c_str());
        }
        // active = the last opened device
        gtk_combo_box_set_active(comboBox, nDevice - 1);
    }
    else
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] VAmp Driver: faGetCount() returned [" << faGetCount() << "]. No device detected.\n";
        gtk_widget_set_sensitive(m_dialogFastModeSettings, false);
        gtk_widget_set_sensitive(m_acquisitionMode, false);
        gtk_widget_set_sensitive(m_Device, false);
        gtk_widget_set_sensitive(m_auxiliaryChannels, false);
        gtk_widget_set_sensitive(m_triggerChannels, false);
        gtk_widget_set_sensitive(m_pair1PositiveInputs, false);
        gtk_widget_set_sensitive(m_pair1NegativeInputs, false);
        gtk_widget_set_sensitive(m_pair2PositiveInputs, false);
        gtk_widget_set_sensitive(m_pair2NegativeInputs, false);
        gtk_widget_set_sensitive(m_pair3PositiveInputs, false);
        gtk_widget_set_sensitive(m_pair3NegativeInputs, false);
        gtk_widget_set_sensitive(m_pair4PositiveInputs, false);
        gtk_widget_set_sensitive(m_pair4NegativeInputs, false);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_change_channel_names")), false);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "checkbutton_impedance")), false);
    }

    m_iDeviceCount = nDevice;

    // acquisition mode
    gtk_combo_box_set_active(GTK_COMBO_BOX(m_acquisitionMode), m_headerBrainProductsVAmp->getAcquisitionMode());

    // aux channels and triggers
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_auxiliaryChannels), m_rAcquireAuxiliaryAsEEG);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_triggerChannels), m_rAcquireTriggerAsEEG);

    // channel count (widget is hidden but is read when clicnking on "channel names..."
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_nChannels), m_header->getChannelCount());

    //SETTINGS:
    //Adding all possible settings : 7/8/9/10 and -1
    // and setting active text
    //_______________________________________________________

    initFastModeSettingsComboBox(m_pair1PositiveInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[0]);
    initFastModeSettingsComboBox(m_pair1NegativeInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[0]);
    initFastModeSettingsComboBox(m_pair2PositiveInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[1]);
    initFastModeSettingsComboBox(m_pair2NegativeInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[1]);
    initFastModeSettingsComboBox(m_pair3PositiveInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[2]);
    initFastModeSettingsComboBox(m_pair3NegativeInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[2]);
    initFastModeSettingsComboBox(m_pair4PositiveInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[3]);
    initFastModeSettingsComboBox(m_pair4NegativeInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[3]);

    return true;
}

bool CConfigurationBrainProductsVAmp::postConfigure()
{
    if (m_applyConfig)
    {
        m_headerBrainProductsVAmp->setAcquisitionMode(static_cast<enum EAcquisitionModes>(gtk_combo_box_get_active(GTK_COMBO_BOX(m_acquisitionMode))));
        int usbIdx = 0;

        // Device number
        if (m_iDeviceCount != 0)
        {
            int number;
            GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));
            if (sscanf(gtk_combo_box_get_active_text(comboBox), "device#%i", &number) == 1)
            {
                m_headerBrainProductsVAmp->setDeviceId(faGetId(uint32_t(number)));
            }
        }

        // aux and triggers: update the references
        m_rAcquireAuxiliaryAsEEG = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_auxiliaryChannels)) != 0;
        m_rAcquireTriggerAsEEG   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_triggerChannels)) != 0;
    }

#if 0
    // Code from CConfigurationBuilder::postConfigure() in order to have the channel names before making the pairs !
    if(m_applyConfig)
    {
        m_header->setChannelCount(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_nChannels)));

        for (size_t i=0; i!=m_header->getChannelCount(); ++i)
        {
            if(m_channelNames[i]!="")
            {
                m_header->setChannelName(i, m_channelNames[i].c_str());
            }
        }
    }
#endif

    //releasing ressources as we dont need it anymore
    gtk_widget_hide(m_dialogFastModeSettings);

    // making the pairs names
    if (m_headerBrainProductsVAmp->getAcquisitionMode() == VAmp4Fast)
    {
        size_t nPair = 0;
        std::vector<size_t> pairIdx; // if the user has n<4 pairs but not in the n first settings, we need the indexes
        for (size_t i = 0; i < 4; ++i)
        {
            if (m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[i] != -1
                || m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[i] != -1)
            {
                nPair++;
                pairIdx.push_back(i);
            }
        }

        m_headerBrainProductsVAmp->setPairCount(nPair);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << nPair << " channel pairs used.\n";

        // we need to rename the 4 first channels, in order to match the pairs
        std::vector<std::string> pairNames;

        for (size_t i = 0; i < nPair; ++i)
        {
            std::string positiveChannelName, negativeChannelName, pairName;
            bool positiveChannelSet = false;
            bool negativeChannelSet = false;

            if (m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[pairIdx[i]] != -1)
            {
                positiveChannelName = m_header->getChannelName(m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[pairIdx[i]]);
                if (positiveChannelName == "")
                {
                    int val             = m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[pairIdx[i]];
                    positiveChannelName = std::to_string(val);
                }
                positiveChannelSet = true;
            }

            if (m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[pairIdx[i]] != -1)
            {
                negativeChannelName = m_header->getChannelName(m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[pairIdx[i]]);
                if (negativeChannelName == "")
                {
                    int val             = m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[pairIdx[i]];
                    negativeChannelName = std::to_string(val);
                }
                negativeChannelSet = true;
            }

            if (negativeChannelSet && positiveChannelSet) { pairName = "Differential (" + positiveChannelName + " - " + negativeChannelName + ")"; }

            if (!negativeChannelSet && positiveChannelSet) { pairName = "Monopolar(+) " + positiveChannelName; }
            if (negativeChannelSet && !positiveChannelSet) { pairName = "Monopolar(-) " + negativeChannelName; }

            pairNames.push_back(pairName);
        }

        for (size_t i = 0; i < nPair; ++i) { m_headerBrainProductsVAmp->setPairName(i, pairNames[i].c_str()); }
    }

    //remove the idle function for the service check
    if (m_giIdleID != 0) g_source_remove(m_giIdleID);

    if (!CConfigurationBuilder::postConfigure()) { return false; } // normal header is filled, ressources are realesed

    // Force sampling frequency
#if 0
    m_headerBrainProductsVAmp->setSamplingFrequency(m_headerBrainProductsVAmp->getAcquisitionMode()==EAcquisitionModes::VAmp4Fast?20000:2000);
#else
    // Signal is now decimated down to 512Hz whatever input sampling frequency was acquired
    m_headerBrainProductsVAmp->setSamplingFrequency(512);
#endif

    return true;
}

//____________________________________________________________________________________//

void CConfigurationBrainProductsVAmp::buttonFastModeSettingsCB()
{
    GtkDialog* dialog = GTK_DIALOG(gtk_builder_get_object(m_builder, "dialog_fast_mode_settings"));
    int response;
    do
    {
        response = gtk_dialog_run(dialog);
        switch (response)
        {
            case GTK_RESPONSE_APPLY:
            {
                t_faDataModeSettings settings;
                settings.Mode20kHz4Channels.ChannelsPos[0] = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pair1PositiveInputs)));
                settings.Mode20kHz4Channels.ChannelsNeg[0] = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pair1NegativeInputs)));

                settings.Mode20kHz4Channels.ChannelsPos[1] = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pair2PositiveInputs)));
                settings.Mode20kHz4Channels.ChannelsNeg[1] = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pair2NegativeInputs)));

                settings.Mode20kHz4Channels.ChannelsPos[2] = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pair3PositiveInputs)));
                settings.Mode20kHz4Channels.ChannelsNeg[2] = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pair3NegativeInputs)));

                settings.Mode20kHz4Channels.ChannelsPos[3] = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pair4PositiveInputs)));
                settings.Mode20kHz4Channels.ChannelsNeg[3] = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pair4NegativeInputs)));

                m_headerBrainProductsVAmp->setFastModeSettings(settings);
                GTKComboBoxSetActiveText(GTK_COMBO_BOX(m_acquisitionMode), "Fast");

                break;
            }
            case GTK_RESPONSE_CANCEL:
            {
                //Data mode
                gtk_combo_box_set_active(GTK_COMBO_BOX(m_acquisitionMode), m_headerBrainProductsVAmp->getAcquisitionMode());

                //Settings
                initFastModeSettingsComboBox(m_pair1PositiveInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[0], false);
                initFastModeSettingsComboBox(m_pair1NegativeInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[0], false);
                initFastModeSettingsComboBox(m_pair2PositiveInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[1], false);
                initFastModeSettingsComboBox(m_pair2NegativeInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[1], false);
                initFastModeSettingsComboBox(m_pair3PositiveInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[2], false);
                initFastModeSettingsComboBox(m_pair3NegativeInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[2], false);
                initFastModeSettingsComboBox(m_pair4PositiveInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsPos[3], false);
                initFastModeSettingsComboBox(m_pair4NegativeInputs, m_headerBrainProductsVAmp->getFastModeSettings().Mode20kHz4Channels.ChannelsNeg[3], false);

                break;
            }
            default: break;
        }
    } while (response != GTK_RESPONSE_APPLY && response != GTK_RESPONSE_CANCEL);

    gtk_widget_hide(GTK_WIDGET(dialog));
}

void CConfigurationBrainProductsVAmp::channelCountChangedCB()
{
    const bool acquireAuxiliaryAsEEG = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_auxiliaryChannels)) != 0;
    const bool acquireTriggerAsEEG   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_triggerChannels)) != 0;
    const gint acquisitionMode       = gtk_combo_box_get_active(GTK_COMBO_BOX(m_acquisitionMode));

    uint32_t nChannel = m_headerBrainProductsVAmp->getEEGChannelCount(acquisitionMode);
    nChannel += (acquireTriggerAsEEG ? m_headerBrainProductsVAmp->getTriggerChannelCount(acquisitionMode) : 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_nChannels), nChannel);

    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_change_channel_names")), acquisitionMode != VAmp4Fast);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_fast_mode_settings")), acquisitionMode == VAmp4Fast);
}

bool CConfigurationBrainProductsVAmp::controlVampService(bool start)
{
    SC_HANDLE scm     = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE service = nullptr;
    //bool bReturn = false;
    //DWORD dwReturn = NO_ERROR;

    if (scm == nullptr || scm == INVALID_HANDLE_VALUE)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [VampService] The driver was unable to create the handler to the SCManager (err code "
                << uint32_t(GetLastError()) << ".\n";
        return false;
    }
    try
    {
        service = OpenService(scm, "VampService", SERVICE_ALL_ACCESS);
        if (service != nullptr)
        {
            SERVICE_STATUS status;
            QueryServiceStatus(service, &status);
            if (start)
            {
                if (status.dwCurrentState != SERVICE_RUNNING)
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " [VampService] Starting VampService...\n";
                    if (!StartService(service, 0, nullptr)) // handle to service, number of arguments, no arguments
                    {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [VampService] The driver was unable to restart the service (err code " <<
                                size_t(GetLastError()) << ".\n";
                        return false;
                    }
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " [VampService] VampService started successfully.\n";
                    return true;
                }
            }
            else // try to stop service
            {
                if (status.dwCurrentState != SERVICE_STOPPED)
                {
                    ControlService(service, SERVICE_CONTROL_STOP, &status);
                    bool stopped = false;
                    for (int i = 0; i < 5 && !stopped; ++i) // about 5 seconds
                    {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " [VampService] Checking the VampService...\n";
                        Sleep(1000);
                        ControlService(service, SERVICE_CONTROL_INTERROGATE, &status);
                        if (status.dwCurrentState == SERVICE_STOPPED)
                        {
                            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " [VampService] VampService stopped successfully.\n";
                            stopped = true;
                        }
                    }
                    if (!stopped) m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << " [VampService] After 5 seconds check, VampService did not stop.\n";
                }
            }
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " [VampService] Closing handlers.\n";
            CloseServiceHandle(service);
            service = nullptr;
            CloseServiceHandle(scm);
            scm = nullptr;
        }
        else
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [VampService] The driver was unable to create the handler to VampService (err code "
                    << uint32_t(GetLastError()) << ".\n";
            return false;
        }
    }
    catch (...)
    {
        if (service != nullptr)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [VampService] An error occurred with the VampService handler (err code "
                    << uint32_t(GetLastError()) << ".\n";
            CloseServiceHandle(service);
            service = nullptr;
        }
        if (scm != nullptr)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " [VampService] An error occurred with the SCManager handler (err code "
                    << uint32_t(GetLastError()) << ".\n";
            CloseServiceHandle(scm);
            scm = nullptr;
        }
        return false;
    }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyUSBFirstAmpAPI
