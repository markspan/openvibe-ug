/*
* NeuroServo driver for OpenViBE
*
* \author (NeuroServo)
* \date Wed Nov 23 00:24:00 2016
*
*/

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include "ovasCConfigurationNeuroServoHid.h"

namespace OpenViBE {
namespace AcquisitionServer {

// Automatic Shutdown callback
static void AutomaticShutdownCB(GtkToggleButton* button, CConfigurationNeuroServoHid* data)
{
    data->checkRadioAutomaticShutdown(gtk_toggle_button_get_active(button) == 1);
}

// Shutdown on driver disconnect callback
static void ShutdownOnDriverDisconnectCB(GtkToggleButton* button, CConfigurationNeuroServoHid* data)
{
    data->checkRadioShutdownOnDriverDisconnect(gtk_toggle_button_get_active(button) == 1);
}

// Device Light Enable  callback
static void DeviceLightEnableCB(GtkToggleButton* button, CConfigurationNeuroServoHid* data)
{
    data->checkRadioDeviceLightEnable(gtk_toggle_button_get_active(button) == 1);
}

// If you added more reference attribute, initialize them here
CConfigurationNeuroServoHid::CConfigurationNeuroServoHid(IDriverContext& ctx, const char* gtkBuilderFilename)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx) {}

bool CConfigurationNeuroServoHid::preConfigure()
{
    if (! CConfigurationBuilder::preConfigure()) { return false; }

    // callbacks connection

    // Connection of "Automatic Shutdown" toggle button
    GtkToggleButton* buttonAutomaticShutdown = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_automatic_shutdown"));
    gtk_toggle_button_set_active(buttonAutomaticShutdown, m_automaticShutdown ? true : false);

    g_signal_connect(gtk_builder_get_object(m_builder, "checkbutton_automatic_shutdown"), "toggled", G_CALLBACK(AutomaticShutdownCB), this);
    this->checkRadioAutomaticShutdown(m_automaticShutdown);

    // Connection of "Shutdown on driver disconnect" toggle button
    GtkToggleButton* buttonShutdownOnDriverDisconnect = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_shutdown_on_driver_disconnect"));
    gtk_toggle_button_set_active(buttonShutdownOnDriverDisconnect, m_shutdownOnDriverDisconnect ? true : false);

    g_signal_connect(gtk_builder_get_object(m_builder, "checkbutton_shutdown_on_driver_disconnect"), "toggled", G_CALLBACK(ShutdownOnDriverDisconnectCB), this);
    this->checkRadioShutdownOnDriverDisconnect(m_shutdownOnDriverDisconnect);

    // Connection of "Device Light Enable" toggle button
    GtkToggleButton* buttonDeviceLightEnable = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_device_light_enable"));
    gtk_toggle_button_set_active(buttonDeviceLightEnable, m_deviceLightEnable ? true : false);

    g_signal_connect(gtk_builder_get_object(m_builder, "checkbutton_device_light_enable"), "toggled", G_CALLBACK(DeviceLightEnableCB), this);
    this->checkRadioDeviceLightEnable(m_deviceLightEnable);

    return true;
}

bool CConfigurationNeuroServoHid::postConfigure()
{
    if (m_applyConfig) {
        // Automatic Shutdown
        GtkToggleButton* buttonAutomaticShutdown = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_automatic_shutdown"));
        m_automaticShutdown                      = gtk_toggle_button_get_active(buttonAutomaticShutdown) ? true : false;

        // Shutdown on driver disconnect
        GtkToggleButton* buttonShutdownOnDriverDisconnect = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_shutdown_on_driver_disconnect"));
        m_shutdownOnDriverDisconnect                      = gtk_toggle_button_get_active(buttonShutdownOnDriverDisconnect) ? true : false;

        // Device Light Enable
        GtkToggleButton* buttonDeviceLightEnable = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_device_light_enable"));
        m_deviceLightEnable                      = gtk_toggle_button_get_active(buttonDeviceLightEnable) ? true : false;
    }

    if (! CConfigurationBuilder::postConfigure()) { return false; }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
#endif // TARGET_OS_Windows
