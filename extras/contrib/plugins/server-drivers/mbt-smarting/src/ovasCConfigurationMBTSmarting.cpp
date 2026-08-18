#include "ovasCConfigurationMBTSmarting.h"

#include <string.h>
#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:

//Callback connected to a dedicated gtk button:
static void button_calibrate_pressed_cb(GtkButton* button, void* data)
{
    CConfigurationMBTSmarting* config=static_cast<CConfigurationMBTSmarting*>(data);
    config->buttonCalibratePressedCB();
}

//Callback actually called:
void CConfigurationGTecGUSBamp::buttonCalibratePressedCB()
{
    // Connect to the hardware, ask for calibration, verify the return code, etc.
}
_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationMBTSmarting::CConfigurationMBTSmarting(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& rConnectionId)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_connectionID(rConnectionId)
{
    //m_listStore = gtk_list_store_new(1, G_TYPE_STRING);
}

CConfigurationMBTSmarting::~CConfigurationMBTSmarting()
{
    //g_object_unref(m_listStore);
}

bool CConfigurationMBTSmarting::preConfigure()
{
    if (! CConfigurationBuilder::preConfigure()) { return false; }

    // Apply previous port number
    GtkSpinButton* buttonPortNumber = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_port_number"));
    gtk_spin_button_set_value(buttonPortNumber, m_connectionID);

    // Connect here all callbacks
    // Example:
    // g_signal_connect(gtk_builder_get_object(m_builder, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

    // Insert here the pre-configure code.
    // For example, you may want to check if a device is currently connected
    // and if more than one are connected. Then you can list in a dedicated combo-box 
    // the device currently connected so the user can choose which one he wants to acquire from.

    return true;
}

bool CConfigurationMBTSmarting::postConfigure()
{
    if (m_applyConfig)
    {
        // If the user pressed the "apply" button, you need to save the changes made in the configuration.
        // For example, you can save the connection ID of the selected device:
        // m_connectionID = <value-from-gtk-widget>
        GtkSpinButton* buttonPortNumber = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_port_number"));
        gtk_spin_button_update(buttonPortNumber);
        m_connectionID = gtk_spin_button_get_value_as_int(buttonPortNumber);
    }

    // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
    return CConfigurationBuilder::postConfigure();
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
