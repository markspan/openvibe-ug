#if defined(TARGET_HAS_ThirdPartyGNEEDaccessAPI)

#include "ovasCConfigurationgNautilusInterface.h"

namespace OpenViBE {
namespace AcquisitionServer {

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:
*/

//Callback connected to a dedicated gtk button (button_select_channels_bipolar_car_noise):
static void ChannelSettingsCB(GtkButton* button, void* data) { static_cast<CConfigurationgNautilusInterface*>(data)->buttonChannelSettingsPressedCB(); }

//Callback connected to a dedicated gtk button (button_select_sensitivity_filters):
static void SensitivityFiltersCB(GtkButton* button, void* data) { static_cast<CConfigurationgNautilusInterface*>(data)->buttonSensitivityFiltersPressedCB(); }

//Callback connected to a dedicated gtk button (button_sensitivity_filters_apply):
static void SensitivityFiltersApplyCB(GtkButton* button, void* data)
{
    static_cast<CConfigurationgNautilusInterface*>(data)->buttonSensitivityFiltersApplyPressedCB();
}

//Callback connected to a dedicated gtk button (button_channel_apply):
static void ChannelSettingsApplyCB(GtkButton* button, void* data)
{
    static_cast<CConfigurationgNautilusInterface*>(data)->buttonChannelSettingsApplyPressedCB();
}

//Callback actually called:
void CConfigurationgNautilusInterface::buttonChannelSettingsPressedCB()
{
    // Connect to the hardware, ask for calibration, verify the return code, etc.

    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder,"dialog_select_channels_bipolar_car_noise"));
    gint resp         = gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_hide(dialog);
}

//Callback actually called:
void CConfigurationgNautilusInterface::buttonSensitivityFiltersPressedCB()
{
    // get bandpass and notch filters for currenty selected sampling rate and set to filter list in corresponding dialog
    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder,"dialog_sensitivity_filters"));
    gint resp         = gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_hide(dialog);
}


//Callback actually called:
void CConfigurationgNautilusInterface::buttonSensitivityFiltersApplyPressedCB()
{
    // get handle to sensitivity and filters dialog and close it
    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder,"dialog_sensitivity_filters"));
    gtk_widget_hide(dialog);
}


//Callback actually called:
void CConfigurationgNautilusInterface::buttonChannelSettingsApplyPressedCB()
{
    // get handle to channel settings dialog and close it
    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder,"dialog_select_channels_bipolar_car_noise"));
    gtk_widget_hide(dialog);

    // get number of channels selected and set range as number of channels starting at number of channels
    uint16_t nChannels = 0;
    char tmp[30];
    for (uint32_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        // set electrode names as channel names in channel selection dialog
        sprintf_s(&tmp[0], 30, "checkbutton_channel_%d", (i + 1));
        GtkButton* checkButton = GTK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        if (checkButton)
        {
            if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton))) && (gtk_widget_get_visible(GTK_WIDGET(checkButton)))) { nChannels += 1; }
        }
    }

    GtkSpinButton* button = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_number_of_channels"));
    gtk_spin_button_set_range(button, nChannels, nChannels);
    gtk_spin_button_set_value(button, nChannels);
}

// catch combobox sampling rate changed signal and call function which handles event
static void sample_rate_changed_cb(GtkComboBox* comboBox, void* data) { static_cast<CConfigurationgNautilusInterface*>(data)->comboboxSampleRateChangedCB(); }

// Callback actually called
void CConfigurationgNautilusInterface::comboboxSampleRateChangedCB()
{
    // get hardware filters according to sampling frequency currently selected
    const bool res = getFiltersForNewSamplingRate();
    if (!res) { } // error logged in getFiltersForNewSamplingRate 
}

// catch noise reduction checkbox toggled signal and call function which handles event
static void noise_reduction_changed_cb(GtkCheckButton* pCheckbutton, void* data)
{
    static_cast<CConfigurationgNautilusInterface*>(data)->checkbuttonNoiseReductionChangedCB();
}

// Callback actually called
void CConfigurationgNautilusInterface::checkbuttonNoiseReductionChangedCB()
{
    // activate/deactivate noise reduction checkboxes in dialog_select_channels_bipolar_car_noise
    GtkCheckButton* buttonNoise = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_noise_reduction"));
    const gboolean buttonValue  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttonNoise));
    char tmp[45];
    for (uint16_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        sprintf_s(&tmp[0], 45, "checkbutton_noise_channel_%d", (i + 1));
        GtkCheckButton* buttonNoiseChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));

        if (buttonNoiseChannel)
        {
            if (gtk_widget_get_visible(GTK_WIDGET(buttonNoiseChannel))) { gtk_widget_set_sensitive(GTK_WIDGET(buttonNoiseChannel), buttonValue); }
        }
    }
}

// catch car checkbox toggled signal and call function which handles event
static void car_changed_cb(GtkCheckButton* /*button*/, void* data) { static_cast<CConfigurationgNautilusInterface*>(data)->checkbuttonCARChangedCB(); }

// Callback actually called
void CConfigurationgNautilusInterface::checkbuttonCARChangedCB()
{
    // activate/deactivate car checkboxes in dialog_select_channels_bipolar_car_noise
    GtkCheckButton* buttonCAR  = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_car"));
    const gboolean buttonValue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttonCAR));
    char tmp[45];
    for (uint16_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        sprintf_s(&tmp[0], 45, "checkbutton_car_channel_%d", (i + 1));
        GtkCheckButton* buttonCarChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        if (buttonCarChannel)
        {
            if (gtk_widget_get_visible(GTK_WIDGET(buttonCarChannel))) { gtk_widget_set_sensitive(GTK_WIDGET(buttonCarChannel), buttonValue); }
        }
    }
}

// get hardware related settings from GDS
bool CConfigurationgNautilusInterface::getHardwareSettings()
{
    GtkTreeIter iter;
    uint32_t i;

    // get network channel and set in dialog and set one as active in dialog
    size_t supportedNwChannelsCount;
    // get number of supported network channels to allocate memory to hold supported network channels
    m_gdsResult = GDS_GNAUTILUS_GetSupportedNetworkChannels(m_deviceHandle, m_deviceNames, nullptr, &supportedNwChannelsCount);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    uint32_t* supportedNwChannels = new uint32_t[supportedNwChannelsCount];
    m_gdsResult                   = GDS_GNAUTILUS_GetSupportedNetworkChannels(m_deviceHandle, m_deviceNames, supportedNwChannels, &supportedNwChannelsCount);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    // get network channel currently used between base station and headstage
    uint32_t val;
    m_gdsResult = GDS_GNAUTILUS_GetNetworkChannel(m_deviceHandle, m_deviceNames, &val);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // put network channels to combobox
    GtkComboBox* comboBoxNetworkChannel   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_network_channel"));
    GtkTreeModel* listStoreNetworkChannel = gtk_combo_box_get_model(comboBoxNetworkChannel);
    gtk_list_store_clear(GTK_LIST_STORE(listStoreNetworkChannel));
    std::stringstream networkChannel;
    if (m_comboBoxNetworkChannels.size() > 0) m_comboBoxNetworkChannels.clear();

    // fill network channel combobox with available network channels
    for (i = 0; i < supportedNwChannelsCount; ++i)
    {
        networkChannel << supportedNwChannels[i];
        gtk_list_store_append(GTK_LIST_STORE(listStoreNetworkChannel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(listStoreNetworkChannel), &iter, 0, networkChannel.str().c_str(), -1);
        networkChannel.clear();
        networkChannel.str("");

        m_comboBoxNetworkChannels.push_back(supportedNwChannels[i]);
    }

    const auto it          = find(m_comboBoxNetworkChannels.begin(), m_comboBoxNetworkChannels.end(), val);
    const size_t nwChIndex = distance(m_comboBoxNetworkChannels.begin(), it);
    gtk_combo_box_set_active(comboBoxNetworkChannel, nwChIndex);

    delete [] supportedNwChannels;

    // get supported sample rates
    size_t nSampling;

    // get number of supported sample rates
    m_gdsResult = GDS_GNAUTILUS_GetSupportedSamplingRates(m_deviceHandle, m_deviceNames, nullptr, &nSampling);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    uint32_t* sampling = new uint32_t[nSampling];
    // get supported sample rates
    m_gdsResult = GDS_GNAUTILUS_GetSupportedSamplingRates(m_deviceHandle, m_deviceNames, sampling, &nSampling);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // set sample rates as content of combo box in corresponding dialog
    GtkComboBox* comboBoxSamplings   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));
    GtkTreeModel* listStoreSamplings = gtk_combo_box_get_model(comboBoxSamplings);
    gtk_list_store_clear(GTK_LIST_STORE(listStoreSamplings));
    for (i = 0; i < nSampling; ++i)
    {
        std::string str = std::to_string(sampling[i]);
        gtk_list_store_append(GTK_LIST_STORE(listStoreSamplings), &iter);
        gtk_list_store_set(GTK_LIST_STORE(listStoreSamplings), &iter, 0, str.c_str(), -1);
    }
    gtk_combo_box_set_active(comboBoxSamplings, 0);

    const bool functionReturn = getFiltersForNewSamplingRate();
    if (!functionReturn) { return false; }

    size_t nSensitivities;
    // get number of sensitivities
    m_gdsResult = GDS_GNAUTILUS_GetSupportedSensitivities(m_deviceHandle, m_deviceNames, nullptr, &nSensitivities);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    // get sensitivities
    double* sensitivities = new double[nSensitivities];
    m_gdsResult           = GDS_GNAUTILUS_GetSupportedSensitivities(m_deviceHandle, m_deviceNames, sensitivities, &nSensitivities);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    // set items in dialog item combobox_select_sensitivity
    GtkComboBox* comboBoxSensitivities   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_select_sensitivity"));
    GtkTreeModel* listStoreSensitivities = gtk_combo_box_get_model(comboBoxSensitivities);
    gtk_list_store_clear(GTK_LIST_STORE(listStoreSensitivities));
    for (i = 0; i < nSensitivities; ++i)
    {
        std::string str = std::to_string(sensitivities[i] / 1000);
        gtk_list_store_append(GTK_LIST_STORE(listStoreSensitivities), &iter);
        gtk_list_store_set(GTK_LIST_STORE(listStoreSensitivities), &iter, 0, str.c_str(), -1);
        m_comboBoxSensitivityValues.push_back(sensitivities[i]);
    }
    gtk_combo_box_set_active(comboBoxSensitivities, 0);

    delete [] sensitivities;

    size_t nInputSources;
    // first get number of supported input sources (call with third parameter set to NULL)
    m_gdsResult = GDS_GNAUTILUS_GetSupportedInputSources(m_deviceHandle, m_deviceNames, nullptr, &nInputSources);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    // allocate memory to hold input sources
    GDS_GNAUTILUS_INPUT_SIGNAL* inputSources = new GDS_GNAUTILUS_INPUT_SIGNAL[nInputSources];
    // now get input sources
    m_gdsResult = GDS_GNAUTILUS_GetSupportedInputSources(m_deviceHandle, m_deviceNames, inputSources, &nInputSources);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // set values to combobox_input_source (there are only three allowed at the moment, see code below)
    GtkComboBox* comboBoxInputSources   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_input_source"));
    GtkTreeModel* listStoreInputSources = gtk_combo_box_get_model(comboBoxInputSources);
    gtk_list_store_clear(GTK_LIST_STORE(listStoreInputSources));
    for (i = 0; i < nInputSources; ++i)
    {
        if (inputSources[i] == 0)
        {
            // electrode input
            m_comboBoxInputSources.push_back(inputSources[i]);
            gtk_list_store_append(GTK_LIST_STORE(listStoreInputSources), &iter);
            gtk_list_store_set(GTK_LIST_STORE(listStoreInputSources), &iter, 0, "Electrode", -1);
        }
        else if (inputSources[i] == 1)
        {
            // shortcut
            m_comboBoxInputSources.push_back(inputSources[i]);
            gtk_list_store_append(GTK_LIST_STORE(listStoreInputSources), &iter);
            gtk_list_store_set(GTK_LIST_STORE(listStoreInputSources), &iter, 0, "Shortcut", -1);
        }
        else if (inputSources[i] == 5)
        {
            // test signal
            m_comboBoxInputSources.push_back(inputSources[i]);
            gtk_list_store_append(GTK_LIST_STORE(listStoreInputSources), &iter);
            gtk_list_store_set(GTK_LIST_STORE(listStoreInputSources), &iter, 0, "Test Signal", -1);
        }
    }
    gtk_combo_box_set_active(comboBoxInputSources, 0);

    return true;
}

// get channel names for hardware currently connected (cannot be changed as electrode grid is fixed)
bool CConfigurationgNautilusInterface::getChannelNames()
{
    uint32_t nMountedModules;
    size_t nElectrodeNames;
    char tmp[30];
    std::stringstream bipolarEntry;

    // get number of mounted modules and electrode names currently available
    nMountedModules = 0;
    nElectrodeNames = 0;
    m_gdsResult     = GDS_GNAUTILUS_GetChannelNames(m_deviceHandle, m_deviceNames, &nMountedModules,NULL, &nElectrodeNames);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    // set array of electrode names according to names currently available
    char (*electrodeNames)[GDS_GNAUTILUS_ELECTRODE_NAME_LENGTH_MAX] = new char[nElectrodeNames][GDS_GNAUTILUS_ELECTRODE_NAME_LENGTH_MAX];
    // get electrode names
    m_gdsResult = GDS_GNAUTILUS_GetChannelNames(m_deviceHandle, m_deviceNames, &nMountedModules, electrodeNames, &nElectrodeNames);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    GtkComboBox* comboBoxBipolarChannels   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_channel_1"));
    GtkTreeModel* listStoreBipolarChannels = gtk_combo_box_get_model(comboBoxBipolarChannels);
    gtk_list_store_clear(GTK_LIST_STORE(listStoreBipolarChannels));

    bipolarEntry << "none";
    GtkTreeIter it;
    gtk_list_store_append(GTK_LIST_STORE(listStoreBipolarChannels), &it);
    gtk_list_store_set(GTK_LIST_STORE(listStoreBipolarChannels), &it, 0, bipolarEntry.str().c_str(), -1);
    for (uint16_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        // set electrode names as channel names in channel selection dialog
        sprintf_s(&tmp[0], 30, "checkbutton_channel_%d", (i + 1));
        GtkButton* buttonChannel = GTK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        if ((buttonChannel) && (i < nElectrodeNames))
        {
            gtk_button_set_label(buttonChannel, &electrodeNames[i][0]);
            gtk_list_store_append(GTK_LIST_STORE(listStoreBipolarChannels), &it);
            gtk_list_store_set(GTK_LIST_STORE(listStoreBipolarChannels), &it, 0, &electrodeNames[i][0], -1);
        }
        else if ((buttonChannel) && (i >= nElectrodeNames))
        {
            gtk_widget_set_sensitive(GTK_WIDGET(buttonChannel), false);
            gtk_button_set_label(buttonChannel, "");
        }
    }
    for (uint16_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        sprintf_s(&tmp[0], 30, "combobox_channel_%d", (i + 1));
        comboBoxBipolarChannels = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,tmp));
        if (comboBoxBipolarChannels)
        {
            gtk_combo_box_set_model(comboBoxBipolarChannels, listStoreBipolarChannels);
            gtk_combo_box_set_active(comboBoxBipolarChannels, 0);
        }
    }
    // adapt number of recorded channels in main configuration dialog according to number of actual selected channels
    GtkSpinButton* buttonNChannels = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder,"spinbutton_number_of_channels"));
    gtk_spin_button_set_range(buttonNChannels, nElectrodeNames, nElectrodeNames);
    gtk_spin_button_set_value(buttonNChannels, nElectrodeNames);

    // set electrode names as channel names
    m_channelNames.clear();
    for (size_t i = 0; i < nElectrodeNames; ++i) { m_channelNames[i] = electrodeNames[i]; }

    delete [] electrodeNames;

    return true;
}

// get channels currenly available for g.Nautilus used
bool CConfigurationgNautilusInterface::getAvailableChannels()
{
    char tmp[30];

    // get channels currently available on connected device
    BOOL availableChannels[GDS_GNAUTILUS_CHANNELS_MAX];
    m_gdsResult = GDS_GNAUTILUS_GetAvailableChannels(m_deviceHandle, m_deviceNames, &availableChannels);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    m_availableChannels.resize(GDS_GNAUTILUS_CHANNELS_MAX);

    for (uint16_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        sprintf_s(&tmp[0], 30, "checkbutton_channel_%d", (i + 1));
        GtkCheckButton* buttonChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        sprintf_s(&tmp[0], 30, "label_channel_%d", (i + 1));
        GtkLabel* labelChannel = GTK_LABEL(gtk_builder_get_object(m_builder,tmp));
        sprintf_s(&tmp[0], 30, "combobox_channel_%d", (i + 1));
        GtkComboBox* comboBoxChannel = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,tmp));
        sprintf_s(&tmp[0], 30, "checkbutton_car_channel_%d", (i + 1));
        GtkCheckButton* buttonCARChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        sprintf_s(&tmp[0], 30, "checkbutton_noise_channel_%d", (i + 1));
        GtkCheckButton* buttonNoiseChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        if (availableChannels[i] == 1)
        {
            if (buttonChannel)
            {
                gtk_widget_set_visible(GTK_WIDGET(buttonChannel), true);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttonChannel), true);
                gtk_widget_set_visible(GTK_WIDGET(labelChannel), true);
                gtk_widget_set_visible(GTK_WIDGET(comboBoxChannel), true);
                gtk_widget_set_visible(GTK_WIDGET(buttonCARChannel), true);
                gtk_widget_set_visible(GTK_WIDGET(buttonNoiseChannel), true);
            }
            m_availableChannels[i] = true;
        }
        else
        {
            if (buttonChannel)
            {
                gtk_widget_set_visible(GTK_WIDGET(buttonChannel), false);
                gtk_widget_set_visible(GTK_WIDGET(labelChannel), false);
                gtk_widget_set_visible(GTK_WIDGET(comboBoxChannel), false);
                gtk_widget_set_visible(GTK_WIDGET(buttonCARChannel), false);
                gtk_widget_set_visible(GTK_WIDGET(buttonNoiseChannel), false);
            }
            m_availableChannels[i] = false;
        }
    }

    return true;
}

// if sampling rate changed filters in filter settings dialog have to be updated according to new sampling rate
bool CConfigurationgNautilusInterface::getFiltersForNewSamplingRate()
{
    GtkTreeIter iter;

    // get current sample rate
    GtkComboBox* comboBoxSampling = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"));
    const char* sSampling         = gtk_combo_box_get_active_text(comboBoxSampling);
    double sampling               = atof(sSampling);

    size_t nBandPassFilters, nNotchFilters;
    std::stringstream filterDesc;
    filterDesc << "no bandpass filter";

    // get number of bandpass filters and allocate filter array correspondingly
    m_gdsResult = GDS_GNAUTILUS_GetBandpassFilters(m_deviceHandle, m_deviceNames, nullptr, &nBandPassFilters);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    GDS_FILTER_INFO* bandPassFilters = new GDS_FILTER_INFO[nBandPassFilters];
    // get bandpass filters
    m_gdsResult = GDS_GNAUTILUS_GetBandpassFilters(m_deviceHandle, m_deviceNames, bandPassFilters, &nBandPassFilters);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage;
        return false;
    }
    // get number of notch filters and allocate filter array correspondingly
    m_gdsResult = GDS_GNAUTILUS_GetNotchFilters(m_deviceHandle, m_deviceNames, nullptr, &nNotchFilters);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    GDS_FILTER_INFO* notchFilters = new GDS_FILTER_INFO[nNotchFilters];
    // get notch filters
    m_gdsResult = GDS_GNAUTILUS_GetNotchFilters(m_deviceHandle, m_deviceNames, notchFilters, &nNotchFilters);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // set filters as combobox entries depending on the current sample rate selected
    GtkComboBox* comboBoxBandPass   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_select_bandpass_filter"));
    GtkTreeModel* listStoreBandPass = gtk_combo_box_get_model(comboBoxBandPass);
    gtk_list_store_clear(GTK_LIST_STORE(listStoreBandPass));

    GtkComboBox* comboBoxNotch   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_select_notch_filter"));
    GtkTreeModel* listStoreNotch = gtk_combo_box_get_model(comboBoxNotch);
    gtk_list_store_clear(GTK_LIST_STORE(listStoreNotch));

    if (m_comboBoxBandpassFilterIdx.size() > 0) m_comboBoxBandpassFilterIdx.clear();
    if (m_comboBoxNotchFilterIdx.size() > 0) m_comboBoxNotchFilterIdx.clear();
    if (m_comboBoxSensitivityValues.size() > 0) m_comboBoxSensitivityValues.clear();

    // fill bandpass filter combobox with available filters
    gtk_list_store_append(GTK_LIST_STORE(listStoreBandPass), &iter);
    gtk_list_store_set(GTK_LIST_STORE(listStoreBandPass), &iter, 0, filterDesc.str().c_str(), -1);
    filterDesc.clear();
    filterDesc.str("");

    m_comboBoxBandpassFilterIdx.push_back(-1);
    for (size_t i = 0; i < nBandPassFilters; ++i)
    {
        if (sampling == bandPassFilters[i].SamplingRate)
        {
            if (bandPassFilters[i].TypeId == 1) filterDesc << "Butterworth - ";
            if (bandPassFilters[i].TypeId == 2) filterDesc << "Chebyshev   - ";

            filterDesc << bandPassFilters[i].Order << " - [";
            filterDesc << bandPassFilters[i].LowerCutoffFrequency << "; ";
            filterDesc << bandPassFilters[i].UpperCutoffFrequency << "] - ";
            filterDesc << bandPassFilters[i].SamplingRate;
            gtk_list_store_append(GTK_LIST_STORE(listStoreBandPass), &iter);
            gtk_list_store_set(GTK_LIST_STORE(listStoreBandPass), &iter, 0, filterDesc.str().c_str(), -1);
            m_comboBoxBandpassFilterIdx.push_back(i);
        }
        filterDesc.clear();
        filterDesc.str("");
    }
    gtk_combo_box_set_active(comboBoxBandPass, m_bandpassFilterIdx + 1); // +1 because -1 is for "no filter".

    // fill notch filter combobox with available fliters
    filterDesc << "no notch filter";
    gtk_list_store_append(GTK_LIST_STORE(listStoreNotch), &iter);
    gtk_list_store_set(GTK_LIST_STORE(listStoreNotch), &iter, 0, filterDesc.str().c_str(), -1);
    filterDesc.clear();
    filterDesc.str("");

    m_comboBoxNotchFilterIdx.push_back(-1);
    for (size_t i = 0; i < nNotchFilters; ++i)
    {
        if (sampling == notchFilters[i].SamplingRate)
        {
            if (notchFilters[i].TypeId == 1) filterDesc << "Butterworth - ";
            if (notchFilters[i].TypeId == 2) filterDesc << "Chebyshev   - ";

            filterDesc << notchFilters[i].Order << " - [";
            filterDesc << notchFilters[i].LowerCutoffFrequency << "; ";
            filterDesc << notchFilters[i].UpperCutoffFrequency << "] - ";
            filterDesc << notchFilters[i].SamplingRate;
            gtk_list_store_append(GTK_LIST_STORE(listStoreNotch), &iter);
            gtk_list_store_set(GTK_LIST_STORE(listStoreNotch), &iter, 0, filterDesc.str().c_str(), -1);
            m_comboBoxNotchFilterIdx.push_back(i);
        }
        filterDesc.clear();
        filterDesc.str("");
    }
    gtk_combo_box_set_active(comboBoxNotch, m_notchFilterIdx + 1); // +1 because -1 is for "no filter".

    delete [] bandPassFilters;
    delete [] notchFilters;

    return true;
}

// If you added more reference attribute, initialize them here
CConfigurationgNautilusInterface::CConfigurationgNautilusInterface(IDriverContext& ctx, const char* gtkBuilderFilename, std::string& deviceSerial,
                                                                   int& inputSource, uint32_t& networkChannel, int& bandpassFilterIdx, int& notchFilterIdx, 
                                                                   double& sensitivity, bool& digitalInputEnabled, bool& noiseReductionEnabled, 
                                                                   bool& carEnabled, bool& accelerationDataEnabled, bool& counterEnabled, 
                                                                   bool& linkQualityEnabled, bool& batteryLevelEnabled, bool& validationIndicatorEnabled,
                                                                   std::vector<uint16_t>& selectedChannels, std::vector<int>& bipolarChannels,
                                                                   std::vector<bool>& cars, std::vector<bool>& noiseReduction)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_deviceSerial(deviceSerial), m_inputSource(inputSource), m_networkChannel(networkChannel),
      m_bandpassFilterIdx(bandpassFilterIdx), m_notchFilterIdx(notchFilterIdx), m_sensitivity(sensitivity), m_digitalInputEnabled(digitalInputEnabled),
      m_noiseReductionEnabled(noiseReductionEnabled), m_carEnabled(carEnabled), m_accelerationDataEnabled(accelerationDataEnabled),
      m_counterEnabled(counterEnabled), m_linkQualityEnabled(linkQualityEnabled), m_batteryLevelEnabled(batteryLevelEnabled),
      m_validationIndicatorEnabled(validationIndicatorEnabled), m_selectedChannels(selectedChannels), m_bipolarChannels(bipolarChannels), m_cars(cars),
      m_noiseReduction(noiseReduction) { m_connectionOpen = false; }

bool CConfigurationgNautilusInterface::preConfigure()
{
    if (! CConfigurationBuilder::preConfigure()) { return false; }

    // Connect here all callbacks
    // Example:
    // g_signal_connect(gtk_builder_get_object(m_builder, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

    // Insert here the pre-configure code.
    // For example, you may want to check if a device is currently connected
    // and if more than one are connected. Then you can list in a dedicated combo-box 
    // the device currently connected so the user can choose which one he wants to acquire from.

    bool functionReturn;

    // open connection to device if not done yet before reading filters, channel names etc.
    if (!m_connectionOpen)
    {
        // open device handle and get connected device
        functionReturn = openDevice();
        if (!functionReturn) { return false; } // error logged in openDevice;
    }

    // get settings for connected hardware
    functionReturn = getHardwareSettings();
    if (!functionReturn) { return false; }
    // error logged in getHardwareSettings

    // get available channels
    functionReturn = getAvailableChannels();
    if (!functionReturn) { return false; }  // error logged in getAvailableChannels

    functionReturn = getChannelNames();
    if (!functionReturn) { return false; }  // error logged in getChannelNames

    // activate checkboxes in g.Nautilus Configuration dialog
    GtkCheckButton* buttonEventChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_event_channel"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttonEventChannel), m_digitalInputEnabled);

    GtkCheckButton* buttonNoiseReduction = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_noise_reduction"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttonNoiseReduction), m_noiseReductionEnabled);

    GtkCheckButton* buttonCAR = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_car"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttonCAR), m_carEnabled);

    // activate/deactivate channel, noise reduction and CAR checkboxes in dialog_select_channels_bipolar_car_noise
    // as well as bipolar combo boxes according to available channels
    const gboolean noiseReduction = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttonNoiseReduction));
    const gboolean car            = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttonCAR));
    char tmp[45];
    for (uint16_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        sprintf_s(&tmp[0], 45, "checkbutton_channel_%d", (i + 1));
        GtkCheckButton* button = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        if ((button) && (m_availableChannels[i])) gtk_widget_set_sensitive(GTK_WIDGET(button), true);
        else if (button) gtk_widget_set_sensitive(GTK_WIDGET(button), false);

        sprintf_s(&tmp[0], 45, "checkbutton_noise_channel_%d", (i + 1));
        button = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        if ((button) && (m_availableChannels[i])) gtk_widget_set_sensitive(GTK_WIDGET(button), noiseReduction);
        else if (button) gtk_widget_set_sensitive(GTK_WIDGET(button), false);

        sprintf_s(&tmp[0], 45, "checkbutton_car_channel_%d", (i + 1));
        button = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
        if ((button) && (m_availableChannels[i])) gtk_widget_set_sensitive(GTK_WIDGET(button), car);
        else if (button) gtk_widget_set_sensitive(GTK_WIDGET(button), false);

        sprintf_s(&tmp[0], 45, "combobox_channel_%d", (i + 1));
        GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,tmp));
        if ((comboBox) && (m_availableChannels[i])) gtk_widget_set_sensitive(GTK_WIDGET(comboBox), true);
        else if (comboBox) gtk_widget_set_sensitive(GTK_WIDGET(comboBox), false);
    }

    GtkCheckButton* accelerationData = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_acceleration_data"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(accelerationData), m_accelerationDataEnabled);

    GtkCheckButton* counter = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_counter"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(counter), m_counterEnabled);

    GtkCheckButton* linkQuality = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_link_quality"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linkQuality), m_linkQualityEnabled);

    GtkCheckButton* batteryLevel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_battery_level"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(batteryLevel), m_batteryLevelEnabled);

    GtkCheckButton* validationIndicator = GTK_CHECK_BUTTON(
        gtk_builder_get_object(m_builder, "checkbutton_validation_indicator"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(validationIndicator), m_validationIndicatorEnabled);

    g_signal_connect(gtk_builder_get_object(m_builder,"button_select_channels_bipolar_car_noise"), "pressed", G_CALLBACK(ChannelSettingsCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder,"button_select_sensitivity_filters"), "pressed", G_CALLBACK(SensitivityFiltersCB), this);

    g_signal_connect(gtk_builder_get_object(m_builder,"button_select_sensitivity_filters_apply"), "pressed", G_CALLBACK(SensitivityFiltersApplyCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder,"button_channel_apply"), "pressed", G_CALLBACK(ChannelSettingsApplyCB), this);

    // set device serial in device serial text entry in g.Nautilus Configuration dialog
    GtkEntry* deviceSerial = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_device_serial"));
    gtk_entry_set_text(deviceSerial, m_deviceSerial.c_str());
    gtk_entry_set_editable(deviceSerial, false);

    // deactivate buttons if no device is detected
    if (gtk_entry_get_text_length(deviceSerial) < 13)
    {
        GtkWidget* button = GTK_WIDGET(gtk_builder_get_object(m_builder,"button_select_channels_bipolar_car_noise"));
        gtk_widget_set_sensitive(button, false);

        button = GTK_WIDGET(gtk_builder_get_object(m_builder,"button_select_sensitivity_filters"));
        gtk_widget_set_sensitive(button, false);
    }

    // catch event when sample rate is changed to adjust available filters
    g_signal_connect(gtk_builder_get_object(m_builder, "combobox_sampling_frequency"), "changed", G_CALLBACK(sample_rate_changed_cb), this);

    // catch events when car and noise reduction checkboxes are toggled in main config dialog to enable/disable corresponding checkboxes in related dialog
    g_signal_connect(gtk_builder_get_object(m_builder, "checkbutton_noise_reduction"), "toggled", G_CALLBACK(noise_reduction_changed_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "checkbutton_car"), "toggled", G_CALLBACK(car_changed_cb), this);

    return true;
}

bool CConfigurationgNautilusInterface::postConfigure()
{
    if (m_applyConfig)
    {
        // If the user pressed the "apply" button, you need to save the changes made in the configuration.
        // For example, you can save the connection ID of the selected device:
        // m_connectionID = <value-from-gtk-widget>
        GtkCheckButton* checkButton;
        uint16_t i;
        gboolean buttonValue = false;
        gdouble nChannels    = 0;

        // get serial number from configuration dialog
        GtkEntry* entrySerial = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_device_serial"));
        m_deviceSerial        = gtk_entry_get_text(entrySerial);

        // get selected input source from dialog
        GtkComboBox* inputSources = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_input_source"));
        size_t idx                = gtk_combo_box_get_active(inputSources);
        m_inputSource             = m_comboBoxInputSources[idx];

        // get selected channels
        char tmp[45];
        m_selectedChannels.clear();
        for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
        {
            sprintf_s(&tmp[0], 45, "checkbutton_channel_%d", (i + 1));
            checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
            if (checkButton)
            {
                if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)))
                {
                    m_selectedChannels.push_back(i + 1);
                    nChannels += 1;
                }
                else
                {
                    m_selectedChannels.push_back(0);
                    m_channelNames.erase(i);
                }
            }
        }

        // get bipolar channels
        m_bipolarChannels.clear();
        for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
        {
            sprintf_s(&tmp[0], 45, "combobox_channel_%d", (i + 1));
            GtkComboBox* bipolarChannels = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,tmp));
            if (bipolarChannels)
            {
                // bipolar is 0 based for the channels, -1 indicates no bipolar derivation
                m_bipolarChannels.push_back(gtk_combo_box_get_active(bipolarChannels) - 1);
            }
        }

        // set bandpass and notch filter indices to correponding variables
        GtkComboBox* bandpassFilters = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_select_bandpass_filter"));
        idx                          = gtk_combo_box_get_active(bandpassFilters);
        m_bandpassFilterIdx          = m_comboBoxBandpassFilterIdx.at(idx);

        GtkComboBox* notchFilters = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_select_notch_filter"));
        idx                       = gtk_combo_box_get_active(notchFilters);
        m_notchFilterIdx          = m_comboBoxNotchFilterIdx.at(idx);

        // sensitivity
        GtkComboBox* sensitivities = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_select_sensitivity"));
        idx                        = gtk_combo_box_get_active(sensitivities);
        m_sensitivity              = m_comboBoxSensitivityValues.at(idx);

        // digital inputs
        checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_event_channel"));
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) { m_digitalInputEnabled = true; }
        else { m_digitalInputEnabled = false; }

        // noise reduction
        checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_noise_reduction"));
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) { m_noiseReductionEnabled = true; }
        else { m_noiseReductionEnabled = false; }

        // if noise reduction is active, check for which channels noise reduction is enabled
        m_noiseReduction.clear();
        for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
        {
            sprintf_s(&tmp[0], 45, "checkbutton_noise_channel_%d", (i + 1));
            checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
            if (checkButton)
            {
                if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) { m_noiseReduction.push_back(true); }
                else { m_noiseReduction.push_back(false); }
            }
        }

        // CAR
        checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_car"));
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) m_carEnabled = true;
        else m_carEnabled                                                                   = false;

        // if CAR is active, check for which channels CAR is enabled
        m_cars.clear();
        for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
        {
            sprintf_s(&tmp[0], 45, "checkbutton_car_channel_%d", (i + 1));
            checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,tmp));
            if (checkButton)
            {
                if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) m_cars.push_back(true);
                else m_cars.push_back(false);
            }
        }

        // acceleration data
        checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_acceleration_data"));
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) { m_accelerationDataEnabled = true; }
        else { m_accelerationDataEnabled = false; }

        // counter
        checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_counter"));
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) { m_counterEnabled = true; }
        else { m_counterEnabled = false; }

        // link quality
        checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_link_quality"));
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) { m_linkQualityEnabled = true; }
        else { m_linkQualityEnabled = false; }

        // battery level
        checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_battery_level"));
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) { m_batteryLevelEnabled = true; }
        else { m_batteryLevelEnabled = false; }

        // validation indicator
        checkButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_builder,"checkbutton_validation_indicator"));
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton)) == 1) { m_validationIndicatorEnabled = true; }
        else { m_validationIndicatorEnabled = false; }

        // network channel
        GtkComboBox* comboBoxNetworkChannel = GTK_COMBO_BOX(gtk_builder_get_object(m_builder,"combobox_network_channel"));
        const char* networkChannel          = gtk_combo_box_get_active_text(comboBoxNetworkChannel);
        m_networkChannel                    = uint32_t(atoi(networkChannel));

        // set number of channels in main configuration dialog (spinbutton still remains disabled, user cannot change value there)
        GtkSpinButton* buttonChannels = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels"));
        gtk_spin_button_set_value(buttonChannels, nChannels);
        gtk_spin_button_set_range(buttonChannels, nChannels, nChannels);
    }
    if (m_connectionOpen)
    {
        // close connection handle
        const bool functionReturn = closeDevice();
        if (!functionReturn)
        {
            // error logged in closeDevice
            return false;
        }
    }

    if (! CConfigurationBuilder::postConfigure()
    ) { return false; } // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed

    return true;
}

bool CConfigurationgNautilusInterface::openDevice()
{
    GDS_ENDPOINT hostEp, localEp;
    GDS_DEVICE_CONNECTION_INFO* connectedDevicesInfo;
    size_t nDevices;
    std::string deviceSerial;
    BOOL openExclusively = true;
    BOOL isCreator;
    m_deviceNames = new char[1][DEVICE_NAME_LENGTH_MAX];

    uint8_t byteIP[4];
    byteIP[0] = 1;
    byteIP[1] = 0;
    byteIP[2] = 0;
    byteIP[3] = 127;
    char tmpIP[16];

    _snprintf_s(tmpIP, IP_ADDRESS_LENGTH_MAX, "%d.%d.%d.%d", byteIP[3], byteIP[2], byteIP[1], byteIP[0]);

    for (size_t i = 0; i < IP_ADDRESS_LENGTH_MAX; ++i)
    {
        hostEp.IpAddress[i]  = tmpIP[i];
        localEp.IpAddress[i] = tmpIP[i];
    }

    hostEp.Port  = 50223;
    localEp.Port = 50224;

    // get connected device
    m_gdsResult = GDS_GetConnectedDevices(hostEp, localEp, &connectedDevicesInfo, &nDevices);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    for (size_t i = 0; i < nDevices; ++i)
    {
        // if devices are in use they cannot be used for a new acquisition
        if ((connectedDevicesInfo[i].InUse) && (i < nDevices)) continue;
        // only one device can be used for data acquisition, as g.Nautilus cannot be synchronized
        if ((connectedDevicesInfo[i].InUse) && (connectedDevicesInfo[i].ConnectedDevicesLength > 1)) continue;
        GDS_DEVICE_INFO* deviceInfo = connectedDevicesInfo[i].ConnectedDevices;
        if (deviceInfo[0].DeviceType == GDS_DEVICE_TYPE_GNAUTILUS)
        {
            deviceSerial = deviceInfo[0].Name;
            nDevices     = 1;
            break;
        }
    }

    if (nDevices == 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "No g.Nautilus connected\n";
        return false;
    }

    strncpy_s(m_deviceNames[0], deviceSerial.c_str(), DEVICE_NAME_LENGTH_MAX);
    m_deviceSerial = deviceSerial;

    // connect to device
    m_gdsResult = GDS_Connect(hostEp, localEp, m_deviceNames, nDevices, openExclusively, &m_deviceHandle, &isCreator);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // free connected devices list allocated by GDS_GetConnectedDevices
    m_gdsResult = GDS_FreeConnectedDevicesList(&connectedDevicesInfo, nDevices);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    m_connectionOpen = true;

    return true;
}

bool CConfigurationgNautilusInterface::closeDevice()
{
    // disconnect device
    m_gdsResult = GDS_Disconnect(&m_deviceHandle);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    m_connectionOpen = false;
    delete [] m_deviceNames;

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGNEEDaccessAPI
