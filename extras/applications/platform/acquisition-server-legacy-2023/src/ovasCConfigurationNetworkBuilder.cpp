#include "ovasCConfigurationNetworkBuilder.h"

#include <toolkit/ovtk_all.h>

#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {

bool CConfigurationNetworkBuilder::setHostName(const CString& hostName)
{
    m_hostName = hostName;
    return true;
}

bool CConfigurationNetworkBuilder::setHostPort(const uint32_t hostPort)
{
    m_hostPort = hostPort;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CConfigurationNetworkBuilder::preConfigure()
{
    const bool res = CConfigurationBuilder::preConfigure();

    m_pHostName = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_host_name"));
    m_pHostPort = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_host_port"));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_pHostPort), m_hostPort);
    gtk_entry_set_text(GTK_ENTRY(m_pHostName), m_hostName.toASCIIString());

    return res;
}

bool CConfigurationNetworkBuilder::postConfigure()
{
    if (m_applyConfig) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_pHostPort));

        m_hostPort = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pHostPort));
        m_hostName = gtk_entry_get_text(GTK_ENTRY(m_pHostName));
    }

    return CConfigurationBuilder::postConfigure();
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
