#if defined TARGET_HAS_ThirdPartyGUSBampCAPI_Linux

#include "ovasCConfigurationGTecGUSBampLinux.h"

namespace OpenViBE {
namespace AcquisitionServer {

/*_________________________________________________

Callbacks to specific widgets
_________________________________________________*/

void button_apply_bipolar_pressed_cb(GtkButton* button, void* data)
{
    CConfigurationGTecGUSBampLinux* config = static_cast<CConfigurationGTecGUSBampLinux*>(data);
    config->OnButtonApplyConfigPressed(CConfigurationGTecGUSBampLinux::BipolarColumn);
}

void button_apply_bandpass_pressed_cb(GtkButton* button, void* data)
{
    CConfigurationGTecGUSBampLinux* config = static_cast<CConfigurationGTecGUSBampLinux*>(data);
    config->OnButtonApplyConfigPressed(CConfigurationGTecGUSBampLinux::BandpassColumn);
}

void button_apply_notch_pressed_cb(GtkButton* button, void* data)
{
    CConfigurationGTecGUSBampLinux* config = static_cast<CConfigurationGTecGUSBampLinux*>(data);
    config->OnButtonApplyConfigPressed(CConfigurationGTecGUSBampLinux::NotchColumn);
}

void button_check_impedance_pressed_cb(GtkButton* button, void* data) { gtk_button_set_label(button, "Checking..."); }

void button_check_impedance_clicked_cb(GtkButton* button, void* data)
{
    CConfigurationGTecGUSBampLinux* config = static_cast<CConfigurationGTecGUSBampLinux*>(data);
    config->OnButtonCheckImpedanceClicked();
}

void combobox_sampling_frequency_changed_cb(GtkComboBox* pCombobox, void* data)
{
    CConfigurationGTecGUSBampLinux* config = static_cast<CConfigurationGTecGUSBampLinux*>(data);
    config->OnComboboxSamplingFrequencyChanged();
}

void DeviceChangedCB(GtkComboBox* pCombobox, void* data)
{
    CConfigurationGTecGUSBampLinux* config = static_cast<CConfigurationGTecGUSBampLinux*>(data);
    config->OnComboboxDeviceChanged();
}

void entry_impedance_activate_cb(GtkEntry* pEntry, void* data)
{
    // Reset the background colour and stick a question mark on the box so the impedance check function will check this channel
    GdkColor color;
    color.red = color.green = color.blue = 0xFFFF;
    gtk_entry_set_text(pEntry, "?");
    gtk_widget_modify_base(GTK_WIDGET(pEntry), GTK_STATE_NORMAL, &color);
}

/*_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationGTecGUSBampLinux::CConfigurationGTecGUSBampLinux(IDriverContext& ctx, const char* gtkBuilderFilename, std::string* deviceName,
                                                               gt_usbamp_config* config)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_deviceName(deviceName), m_config(config) {}

void CConfigurationGTecGUSBampLinux::UpdateFilters()
{
    GtkListStore* listStore = GTK_LIST_STORE(gtk_builder_get_object(m_builder,"model_channel_config"));
    GtkTreeView* treeView   = GTK_TREE_VIEW(gtk_builder_get_object(m_builder,"treeview_channel_config"));
    GtkTreeModel* treeModel = gtk_tree_view_get_model(treeView);
    GtkTreeIter iter;

    // If the sampling frequency changes the filters may no longer be valid, so clear them all and have the user start again
    for (gboolean end = gtk_tree_model_get_iter_first(treeModel, &iter); end; end = gtk_tree_model_iter_next(treeModel, &iter))
    {
        gtk_list_store_set(listStore, &iter, NotchColumn, "none", -1);
        gtk_list_store_set(listStore, &iter, NotchIdColumn, GT_FILTER_NONE, -1);

        gtk_list_store_set(listStore, &iter, BandpassColumn, "none", -1);
        gtk_list_store_set(listStore, &iter, BandpassIdColumn, GT_FILTER_NONE, -1);
    }

    // Get pointers to the comboboxes
    GtkComboBox* comboBoxDevice   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));
    GtkComboBox* comboBoxSampling = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));

    // Get the device name and the sample rate from the comboboxes
    char* deviceName = gtk_combo_box_get_active_text(comboBoxDevice);
    gt_size sampling = (gt_size)strtol(gtk_combo_box_get_active_text(comboBoxSampling), nullptr, 10);

    // This takes a while so we'll keep the user informed via the console - might have to put this in a thread at some point though
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Opening device [" << deviceName << "] to query filters ...\n";

    // Try opening the device
    if (GT_OpenDevice(deviceName))
    {
        GtkTreeIter iterBandpass, iterNotch;

        GtkListStore* listStoreBandpass = GTK_LIST_STORE(gtk_builder_get_object(m_builder,"model_bandpass"));
        GtkListStore* listStoreNotch    = GTK_LIST_STORE(gtk_builder_get_object(m_builder,"model_notch"));

        // Clear all the entries from the filter comboboxes
        gtk_list_store_clear(listStoreBandpass);
        gtk_list_store_clear(listStoreNotch);

        // Add the none and autoset configurations back in
        // none
        gtk_list_store_append(listStoreBandpass, &iterBandpass);
        gtk_list_store_set(listStoreBandpass, &iterBandpass, 0, "none", 1, GT_FILTER_NONE, -1);
        gtk_list_store_append(listStoreNotch, &iterNotch);
        gtk_list_store_set(listStoreNotch, &iterNotch, 0, "none", 1, GT_FILTER_NONE, -1);

        // autoset
        gtk_list_store_append(listStoreBandpass, &iterBandpass);
        gtk_list_store_set(listStoreBandpass, &iterBandpass, 0, "autoset", 1, GT_FILTER_AUTOSET, -1);
        gtk_list_store_append(listStoreNotch, &iterNotch);
        gtk_list_store_set(listStoreNotch, &iterNotch, 0, "autoset", 1, GT_FILTER_AUTOSET, -1);

        // Set the combo boxes to show none, it's a bit tidier that way
        gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_bandpass")), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_notch")), 0);

        // Get the sizes of the lists
        gt_size bandpassListSize = GT_GetBandpassFilterListSize(deviceName, sampling);
        gt_size notchListSize    = GT_GetNotchFilterListSize(deviceName, sampling);

        // Allocate them
        gt_filter_specification* bandpassList = new gt_filter_specification[bandpassListSize];
        gt_filter_specification* notchList    = new gt_filter_specification[notchListSize];

        // Get the lists themselves - note the last parameter to these two should be specified in bytes
        GT_GetBandpassFilterList(deviceName, sampling, bandpassList, bandpassListSize * sizeof(gt_filter_specification));
        GT_GetNotchFilterList(deviceName, sampling, notchList, notchListSize * sizeof(gt_filter_specification));

        // Repopulate the comboboxes - for each returned filter make a description and put it in the combobox
        // Bandpass
        for (uint32_t i = 0; i < bandpassListSize; ++i)
        {
            std::stringstream ss;
            ss << "HP: " << bandpassList[i].f_lower << " / LP: " << bandpassList[i].f_upper; // HP = High Pass, LP = Low Pass
            gtk_list_store_append(listStoreBandpass, &iterBandpass);
            gtk_list_store_set(listStoreBandpass, &iterBandpass, 0, ss.str().c_str(), 1, bandpassList[i].id, -1);
        }
        // Notch
        for (uint32_t i = 0; i < notchListSize; ++i)
        {
            std::stringstream ss;
            ss << "HS: " << notchList[i].f_lower << " / LS: " << notchList[i].f_upper; // HS = High Stop, LS = Low Stop
            gtk_list_store_append(listStoreNotch, &iterNotch);
            gtk_list_store_set(listStoreNotch, &iterNotch, 0, ss.str().c_str(), 1, notchList[i].id, -1);
        }

        GT_CloseDevice(deviceName);
    }
    else { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open device\n"; }
}

bool CConfigurationGTecGUSBampLinux::preConfigure()
{
    char** deviceList = nullptr;
    size_t size       = 0;

    if (!CConfigurationBuilder::preConfigure()) return false;

    // Refresh and get the list of currently connnected devices
    GT_UpdateDevices();
    size       = GT_GetDeviceListSize();
    deviceList = GT_GetDeviceList();

    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_device"));

    for (uint32_t i = 0; i < size; ++i) { gtk_combo_box_append_text(comboBox, deviceList[i]); }

    GT_FreeDeviceList(deviceList, size);

    // Connect all the callbacks
    g_signal_connect(gtk_builder_get_object(m_builder, "button_apply_bipolar"), "pressed", G_CALLBACK(button_apply_bipolar_pressed_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_apply_bandpass"), "pressed", G_CALLBACK(button_apply_bandpass_pressed_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_apply_notch"), "pressed", G_CALLBACK(button_apply_notch_pressed_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_check_impedance"), "pressed", G_CALLBACK(button_check_impedance_pressed_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_check_impedance"), "clicked", G_CALLBACK(button_check_impedance_clicked_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"), "changed", G_CALLBACK(combobox_sampling_frequency_changed_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "combobox_device"), "changed", G_CALLBACK(DeviceChangedCB), this);

    // Connect up all the activate methods for the text entries so the user can select them and read the associated channel impedance
    for (int i = 0; i < (GT_USBAMP_NUM_ANALOG_IN + GT_USBAMP_NUM_REFERENCE); ++i)
    {
        std::stringstream ss;
        // Compile the string corresponding to the name of the widget displaying the impedance information
        ss << "entry_impedance" << i + 1;
        g_signal_connect(gtk_builder_get_object(m_builder, ss.str().c_str()), "focus-in-event", G_CALLBACK(entry_impedance_activate_cb), this);
    }

    // Couldn't work out how to do this in the designer, set the treeview box to be able to select multiple items at once
    GtkTreeView* treeView       = GTK_TREE_VIEW(gtk_builder_get_object(m_builder,"treeview_channel_config"));
    GtkTreeSelection* selection = gtk_tree_view_get_selection(treeView);
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    // Now apply all the configs recovered from the settings helper to the GUI - don't have to worry about the sampling rate and number of channels though since they're already taken care of

    // First look through all the device names returned from the API. If any match the one recovered from the settings manager set that as active
    GtkTreeModel* treeModelName = gtk_combo_box_get_model(comboBox);
    GtkTreeIter itName;
    for (gboolean end = gtk_tree_model_get_iter_first(treeModelName, &itName); end; end = gtk_tree_model_iter_next(treeModelName, &itName))
    {
        gchar* filterDesc;
        gtk_tree_model_get(treeModelName, &itName, 0, &filterDesc, -1);

        // If the name in the combo box matches the one passed in then make that entry active
        if (*m_deviceName == filterDesc)        // todo: check that this works with the API
        {
            gtk_combo_box_set_active_iter(comboBox, &itName);
        }

        // Free the string now that we're finished with it
        g_free(filterDesc);
    }

    // todo: when we change the devicename combo box we should refresh all the filters as per sampling frequency changed.

    // And since we might have changed to a valid device name, let's update the filter combo boxes
    UpdateFilters();

    // Fill out the analog output configs
    // Shape
    GtkComboBox* comboBoxShape = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_analog_out_shape"));
    gtk_combo_box_set_active(comboBoxShape, m_config->ao_config->shape);
    // Amplitude
    GtkSpinButton* buttonAmplitude = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_analog_out_amplitude"));
    gtk_spin_button_set_value(buttonAmplitude, m_config->ao_config->amplitude);
    // Offset
    GtkSpinButton* buttonOffset = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_analog_out_offset"));
    gtk_spin_button_set_value(buttonOffset, m_config->ao_config->offset);
    // Frequency
    GtkSpinButton* buttonFrequency = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_analog_out_frequency"));
    gtk_spin_button_set_value(buttonFrequency, m_config->ao_config->frequency);

    // Fill out the options
    // Slave
    GtkToggleButton* checkButtonSlave = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_slave"));
    gtk_toggle_button_set_active(checkButtonSlave, m_config->slave_mode);
    // Shortcut
    GtkToggleButton* buttonShortcut = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_shortcut"));
    gtk_toggle_button_set_active(buttonShortcut, m_config->enable_sc);
    // Shortcut
    GtkToggleButton* buttonDio = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_dio"));
    gtk_toggle_button_set_active(buttonDio, m_config->scan_dio);
    // Trigger
    GtkToggleButton* buttonTrigger = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_trigger"));
    gtk_toggle_button_set_active(buttonTrigger, m_config->enable_trigger_line);
    // Mode
    GtkComboBox* comboBoxMode = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_mode"));
    gtk_combo_box_set_active(comboBoxMode, m_config->mode);

    // Set all the blocks A-D to use the common ground and reference voltages
    for (uint32_t i = 0; i < GT_USBAMP_NUM_GROUND; ++i)
    {
        GtkToggleButton* buttonGnd = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, ("checkbutton_block_gnd" + std::to_string(i + 1)).c_str()));
        GtkToggleButton* buttonRef = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, ("checkbutton_block_ref" + std::to_string(i + 1)).c_str()));

        gtk_toggle_button_set_active(buttonGnd, m_config->common_ground[i]);
        gtk_toggle_button_set_active(buttonRef, m_config->common_reference[i]);
    }

    // Config each channel with the info from the configs, no need to check the filters and what not since they must have been correct to have been stored alongside eachother
    GtkListStore* listStore = GTK_LIST_STORE(gtk_builder_get_object(m_builder,"model_channel_config"));
    int i                   = 0;

    GtkTreeModel* treeModelChannel = gtk_tree_view_get_model(treeView);
    GtkTreeIter itChannel;
    for (gboolean end = gtk_tree_model_get_iter_first(treeModelChannel, &itChannel); end; end = gtk_tree_model_iter_next(treeModelChannel, &itChannel), i++)
    {
        // Look through each value in the bandpass model and if there's one that has an id that matches, set it's text to the channel configs
        GtkTreeModel* treeModelBandpass = GTK_TREE_MODEL(gtk_builder_get_object(m_builder,"model_bandpass"));
        GtkTreeIter itBandpass;
        for (gboolean end2 = gtk_tree_model_get_iter_first(treeModelBandpass, &itBandpass); end2; end2 = gtk_tree_model_iter_next(treeModelBandpass, &itBandpass))
        {
            gchar* filterDesc;
            gint filterID;
            gtk_tree_model_get(treeModelBandpass, &itBandpass, 0, &filterDesc, 1, &filterID, -1);

            // If the id is the same as the one in the config, we've found the filter that was set last time
            if (filterID == m_config->bandpass[i]) { gtk_list_store_set(listStore, &itChannel, BandpassColumn, filterDesc, BandpassIdColumn, filterID, -1); }

            // Free the string now that we're finished with it
            g_free(filterDesc);
        }

        // Look through each value in the notch model and if there's one that has an id that matches, set it's text to the channel configs
        GtkTreeModel* treeModelNotch = GTK_TREE_MODEL(gtk_builder_get_object(m_builder,"model_notch"));
        GtkTreeIter iterNotch;
        for (gboolean end2 = gtk_tree_model_get_iter_first(treeModelNotch, &iterNotch); end2; end2 = gtk_tree_model_iter_next(treeModelNotch, &iterNotch))
        {
            gchar* filterDesc;
            gint filterID;
            gtk_tree_model_get(treeModelNotch, &iterNotch, 0, &filterDesc, 1, &filterID, -1);

            // If the id is the same as the one in the config, we've found the filter that was set last time
            if (filterID == m_config->notch[i]) { gtk_list_store_set(listStore, &itChannel, NotchColumn, filterDesc, NotchIdColumn, filterID, -1); }

            // Free the string now that we're finished with it
            g_free(filterDesc);
        }

        // And just straight out set the bipolar channel config
        gtk_list_store_set(listStore, &itChannel, BipolarColumn, gint(m_config->bipolar[i] == GT_BIPOLAR_DERIVATION_NONE ? 0 : m_config->bipolar[i]), -1);
    }

    return true;
}

void CConfigurationGTecGUSBampLinux::OnButtonApplyConfigPressed(ChannelTreeViewColumn type)
{
    // Get the tree view widget
    GtkTreeView* treeView = GTK_TREE_VIEW(gtk_builder_get_object(m_builder,"treeview_channel_config"));

    // Now get it's model, both as a list store so we can set the entries and it's "base class" tree model so we can iterate through it
    GtkTreeModel* treeModel = gtk_tree_view_get_model(treeView);
    GtkListStore* listStore = GTK_LIST_STORE(gtk_builder_get_object(m_builder,"model_channel_config"));

    // Also get the subset of paths that are selected, if any
    GtkTreeSelection* selection = gtk_tree_view_get_selection(treeView);

    // Iterate through them and set the fields
    GtkTreeIter iter;
    for (gboolean end = gtk_tree_model_get_iter_first(treeModel, &iter); end; end = gtk_tree_model_iter_next(treeModel, &iter))
    {
        // If the given row is selected
        if (gtk_tree_selection_iter_is_selected(selection, &iter))
        {
            // Fill in the bipolar field with the spin button contents if the bipolar apply button was pressed
            if (type == BipolarColumn)
            {
                GtkSpinButton* button = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_bipolar"));
                gtk_list_store_set(listStore, &iter, BipolarColumn, gint(gtk_spin_button_get_value(button)), -1);
            }

            // Fill in the filter field with the combobox contents and fill in a hidden field id with another hidden field in the combobox model that stores the filter's id
            gint filterId;
            GtkTreeIter selectedComboIter;

            if (type == NotchColumn)
            {
                GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_notch"));

                // Get the iterator for the active row in the combo box's model
                gtk_combo_box_get_active_iter(comboBox, &selectedComboIter);
                // Get the value of the id column of that row
                gtk_tree_model_get(gtk_combo_box_get_model(comboBox), &selectedComboIter, 1, &filterId, -1);
                // Put the combo box text in the tree view and the id value in the hidden column of the tree view's model
                gtk_list_store_set(listStore, &iter, NotchColumn, gtk_combo_box_get_active_text(comboBox), NotchIdColumn, filterId, -1);
            }

            if (type == BandpassColumn)
            {
                GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_bandpass"));

                // Get the iterator for the active row in the combo box's model
                gtk_combo_box_get_active_iter(comboBox, &selectedComboIter);
                // Get the value of the id column of that row
                gtk_tree_model_get(gtk_combo_box_get_model(comboBox), &selectedComboIter, 1, &filterId, -1);
                // Put the combo box text in the tree view and the id value in the hidden column of the tree view's model
                gtk_list_store_set(listStore, &iter, BandpassColumn, gtk_combo_box_get_active_text(comboBox), BandpassIdColumn, filterId, -1);
            }
        }
    }
}

// We'll make this just check the impedance of the selcected box.
void CConfigurationGTecGUSBampLinux::OnButtonCheckImpedanceClicked()
{
    int impedance;
    GdkColor color;

    // Get the name of the selected device
    GtkComboBox* comboBoxDevice = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_device"));
    char* deviceName            = gtk_combo_box_get_active_text(comboBoxDevice);

    // This takes a while so we'll keep the user informed via the console - might have to put this in a thread at some point though
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Opening device [" << deviceName << "] for impedance check ...\n";

    // Try opening the device
    if (GT_OpenDevice(deviceName))
    {
        // If that worked then for each channel
        for (uint32_t i = 0; i < (GT_USBAMP_NUM_ANALOG_IN + GT_USBAMP_NUM_REFERENCE); ++i)
        {
            // Reset the color so we don't get one impedance's color bleeding into the next
            color.red = color.green = color.blue = color.pixel = 0;

            // Compile the string corresponding to the name of the widget displaying the impedance information
            std::string name = "entry_impedance" + std::to_string(i + 1);
            // Get the relevant text entry
            GtkEntry* text = GTK_ENTRY(gtk_builder_get_object(m_builder, name.c_str()));

            if (strcmp(gtk_entry_get_text(text), "?") == 0)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Reading from channel " << i + 1 << "...\n";

                // Try to get the impedance for each channel (channels 17 - 20 correspond to references A through D)
                if (GT_GetImpedance(deviceName, i + 1, &impedance))
                {
                    // Convert the impedance into kohms
                    impedance /= 1000;

                    // impedance is good
                    if (impedance < LowImpedance) { color.green = 0xFFFF; }
                        // impedance is moderate
                    else if (impedance < ModerateImpedance) { color.red = color.green = 0xFFFF; }
                        // impedance is high
                    else if (impedance < HighImpedance) { color.red = 0xFFFF; }
                        // impedance is so high that the channel is probably not connected
                    else { color.blue = 0xFFFF; }

                    // If the impedance is larger than 100kohm just write NC
                    std::string tmp = (impedance < HighImpedance) ? std::to_string(impedance) : "NC";

                    // Set the text
                    gtk_entry_set_text(text, tmp.c_str());
                }
                else
                {
                    // If impedance reading fails, produce an error message and set the relevant cell to black
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to read from channel " << i + 1 << "...\n";
                    color.red = color.green = color.blue = 1;
                    gtk_entry_set_text(text, "");
                }

                // Then get it as just a widget so we can set the background colour
                GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_builder, name.c_str()));
                gtk_widget_modify_base(widget, GTK_STATE_NORMAL, &color);
            }
        }

        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Closing device...\n";
        GT_CloseDevice(deviceName);
    }
    else { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open device\n"; }

    // Set the text back so the user knows the test is over
    GtkButton* buttonCheckImpedance = GTK_BUTTON(gtk_builder_get_object(m_builder,"button_check_impedance"));
    gtk_button_set_label(buttonCheckImpedance, "Check Impedance");
}

// Open the device, get all the possible filters given the sampling frequency and we also blank out all the preset filters since they might not be valid or the same for the new frequency
void CConfigurationGTecGUSBampLinux::OnComboboxSamplingFrequencyChanged()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Changing sampling frequency invalidates filters\n";
    UpdateFilters();
}

// Same again
void CConfigurationGTecGUSBampLinux::OnComboboxDeviceChanged()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Changing device invalidates filters\n";
    UpdateFilters();
}

bool CConfigurationGTecGUSBampLinux::postConfigure()
{
    if (m_applyConfig)
    {
        // Fill in all the parts of the config that differ from the default configuration we've set out in the CGTecGUSBampLinux constructor
        GtkComboBox* comboBoxDevice = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

        // If there's any active text in the device combo box then set it to the
        if (char* deviceName = gtk_combo_box_get_active_text(comboBoxDevice)) { *m_deviceName = deviceName; }

        // Get the sample rate and the number of channels
        GtkComboBox* comboBoxSampling = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));
        m_config->sample_rate         = (gt_size)strtol(gtk_combo_box_get_active_text(comboBoxSampling), nullptr, 10);

        GtkSpinButton* buttonChannel = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels"));
        m_config->num_analog_in      = gtk_spin_button_get_value(buttonChannel);

        // Fill out the analog output configs
        // Shape
        GtkComboBox* comboBoxShape = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_analog_out_shape"));
        m_config->ao_config->shape = usbamp_analog_out_shape(gtk_combo_box_get_active(comboBoxShape));
        // Amplitude
        GtkSpinButton* buttonAmplitude = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_analog_out_amplitude"));
        m_config->ao_config->amplitude = gtk_spin_button_get_value(buttonAmplitude);
        // Offset
        GtkSpinButton* buttonOffset = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_analog_out_offset"));
        m_config->ao_config->offset = gtk_spin_button_get_value(buttonOffset);
        // Frequency
        GtkSpinButton* buttonFrequency = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_analog_out_frequency"));
        m_config->ao_config->frequency = gtk_spin_button_get_value(buttonFrequency);

        // Fill out the options
        // Slave
        GtkToggleButton* buttonSlave = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_slave"));
        m_config->slave_mode         = gtk_toggle_button_get_active(buttonSlave);
        // Shortcut
        GtkToggleButton* buttonShortcut = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_shortcut"));
        m_config->enable_sc             = gtk_toggle_button_get_active(buttonShortcut);
        // Shortcut
        GtkToggleButton* buttonDio = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_dio"));
        m_config->scan_dio         = gtk_toggle_button_get_active(buttonDio);
        // Trigger
        GtkToggleButton* buttonTrigger = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_trigger"));
        m_config->enable_trigger_line  = gtk_toggle_button_get_active(buttonTrigger);
        // Mode
        GtkComboBox* comboBoxMode = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_mode"));
        m_config->mode            = usbamp_device_mode(gtk_combo_box_get_active(comboBoxMode));

        // Set all the blocks A-D to use the common ground and reference voltages
        for (uint32_t i = 0; i < GT_USBAMP_NUM_GROUND; ++i)
        {
            GtkToggleButton* buttonGnd = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, ("checkbutton_block_gnd" + std::to_string(i + 1)).c_str()));
            GtkToggleButton* buttonRef = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, ("checkbutton_block_ref" + std::to_string(i + 1)).c_str()));

            m_config->common_ground[i]    = gtk_toggle_button_get_active(buttonGnd);
            m_config->common_reference[i] = gtk_toggle_button_get_active(buttonRef);
        }

        // Config each channel with the info
        GtkTreeView* treeView   = GTK_TREE_VIEW(gtk_builder_get_object(m_builder,"treeview_channel_config"));
        GtkTreeModel* treeModel = gtk_tree_view_get_model(treeView);
        gint value;

        GtkTreeIter iter;
        int i = 0;
        for (gboolean end = gtk_tree_model_get_iter_first(treeModel, &iter); end; end = gtk_tree_model_iter_next(treeModel, &iter), i++)
        {
            gtk_tree_model_get(treeModel, &iter, BipolarColumn, &value, -1);
            m_config->bipolar[i] = (value == 0 ? GT_BIPOLAR_DERIVATION_NONE : value);

            gtk_tree_model_get(treeModel, &iter, NotchIdColumn, &value, -1);
            m_config->notch[i] = value;

            gtk_tree_model_get(treeModel, &iter, BandpassIdColumn, &value, -1);
            m_config->bandpass[i] = value;
        }
    }
    if (!CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
        return false;

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGUSBampCAPI_Linux
