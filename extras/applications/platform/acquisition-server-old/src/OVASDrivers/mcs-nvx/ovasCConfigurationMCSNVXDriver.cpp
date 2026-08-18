#if defined(TARGET_HAS_ThirdPartyMCS)

#include "ovasCConfigurationMCSNVXDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:

//Callback connected to a dedicated gtk button:
static void button_calibrate_pressed_cb(GtkButton* button, void* data)
{
    CConfigurationMKSNVXDriver* config=static_cast<CConfigurationMKSNVXDriver*>(data);
    config->buttonCalibratePressedCB();
}

//Callback actually called:
void CConfigurationGTecGUSBamp::buttonCalibratePressedCB()
{
    // Connect to the hardware, ask for calibration, verify the return code, etc.
}
_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationMKSNVXDriver::CConfigurationMKSNVXDriver(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& dataMode, bool& auxChannels)
    : CConfigurationBuilder(gtkBuilderFilename), dataMode_(dataMode), showAuxChannels_(auxChannels), m_driverCtx(ctx) {}

bool CConfigurationMKSNVXDriver::preConfigure()
{
    if (! CConfigurationBuilder::preConfigure()) { return false; }

    // Connect here all callbacks
    // Example:
    // g_signal_connect(gtk_builder_get_object(m_builder, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

    // Insert here the pre-configure code.
    // For example, you may want to check if a device is currently connected
    // and if more than one are connected. Then you can list in a dedicated combo-box 
    // the device currently connected so the user can choose which one he wants to acquire from.
    GtkComboBox* comboDataMode = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_mode"));
    gtk_combo_box_set_active(comboDataMode, dataMode_);
    GtkToggleButton* toggleShowAuxChannels = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_show_aux_channels"));
    gtk_toggle_button_set_active(toggleShowAuxChannels, showAuxChannels_);
    return true;
}

bool CConfigurationMKSNVXDriver::postConfigure()
{
    if (m_applyConfig)
    {
        // If the user pressed the "apply" button, you need to save the changes made in the configuration.
        // For example, you can save the connection ID of the selected device:
        // m_connectionID = <value-from-gtk-widget>
        GtkComboBox* comboDataMode             = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_mode"));
        dataMode_                              = uint32_t(gtk_combo_box_get_active(comboDataMode));
        GtkToggleButton* toggleShowAuxChannels = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_show_aux_channels"));
        showAuxChannels_                       = (gtk_toggle_button_get_active(toggleShowAuxChannels) > 0);
    }

    // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
    if (! CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
