#if defined(WIN32)

#include "ovasCConfigurationCognionics.h"

namespace OpenViBE {
namespace AcquisitionServer {

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:

//Callback connected to a dedicated gtk button:
static void button_calibrate_pressed_cb(GtkButton* button, void* data)
{
    CConfigurationCognionics* config=static_cast<CConfigurationCognionics*>(data);
    config->buttonCalibratePressedCB();
}

//Callback actually called:
void CConfigurationGTecGUSBamp::buttonCalibratePressedCB()
{
    // Connect to the hardware, ask for calibration, verify the return code, etc.
}
_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationCognionics::CConfigurationCognionics(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& comport)
    : CConfigurationBuilder(gtkBuilderFilename), m_COMPORT(comport), m_driverCtx(ctx) {}

bool CConfigurationCognionics::preConfigure()
{
    if (! CConfigurationBuilder::preConfigure()) { return false; }

    GtkSpinButton* comport = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_comport"));

    gtk_spin_button_set_value(comport, m_COMPORT);

    // Connect here all callbacks
    // Example:
    // g_signal_connect(gtk_builder_get_object(m_builder, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

    // Insert here the pre-configure code.
    // For example, you may want to check if a device is currently connected
    // and if more than one are connected. Then you can list in a dedicated combo-box 
    // the device currently connected so the user can choose which one he wants to acquire from.

    return true;
}

bool CConfigurationCognionics::postConfigure()
{
    if (m_applyConfig)
    {
        GtkSpinButton* comport = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_comport"));

        m_COMPORT = uint32_t(gtk_spin_button_get_value(comport));
        //printf("Selected COM Port: %d", m_COMPORT);
        // If the user pressed the "apply" button, you need to save the changes made in the configuration.
        // For example, you can save the connection ID of the selected device:
        // m_connectionID = <value-from-gtk-widget>
    }

    if (! CConfigurationBuilder::postConfigure()
    ) { return false; } // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // WIN32
