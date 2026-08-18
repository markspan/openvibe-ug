#include "ovasCConfigurationEncephalan.h"
#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {

//---------------------------------------------------------------------------------------------------
CConfigurationEncephalan::CConfigurationEncephalan(IDriverContext& driverContext, const char* gtkFileName, uint32_t& connectionPort, const std::string& connectionIp)
    : CConfigurationBuilder(gtkFileName), m_driverContext(driverContext), m_connectionPort(connectionPort), m_connectionIp(connectionIp) { }
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CConfigurationEncephalan::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkSpinButton* port = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_port"));
    gtk_spin_button_set_value(port, m_connectionPort);
    GtkEntry* ip = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_ip"));
    gtk_entry_set_text(ip, m_connectionIp.c_str());

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CConfigurationEncephalan::postConfigure()
{
    if (m_applyConfig) {
        GtkSpinButton* port = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_port"));
        m_connectionPort    = uint32_t(gtk_spin_button_get_value_as_int(port));
        GtkEntry* ip        = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_ip"));
        m_connectionIp      = gtk_entry_get_text(ip);
    }

    return CConfigurationBuilder::postConfigure();
}
//---------------------------------------------------------------------------------------------------

}  // namespace AcquisitionServer
}  // namespace OpenViBE
