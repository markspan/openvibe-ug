#include "ovasCConfigurationEGIAmpServer.h"

#include <toolkit/ovtk_all.h>

#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {

bool CConfigurationEGIAmpServer::preConfigure()
{
    const bool res = CConfigurationBuilder::preConfigure();

    m_pHostName    = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_host_name"));
    m_pCommandPort = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_command_port"));
    m_pStreamPort  = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_stream_port"));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_pCommandPort), m_commandPort);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_pStreamPort), m_streamPort);

    gtk_entry_set_text(GTK_ENTRY(m_pHostName), m_hostName.toASCIIString());

    return res;
}

bool CConfigurationEGIAmpServer::postConfigure()
{
    if (m_applyConfig) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_pStreamPort));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_pCommandPort));

        m_streamPort  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pStreamPort));
        m_commandPort = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pCommandPort));
        m_hostName    = gtk_entry_get_text(GTK_ENTRY(m_pHostName));
    }

    return CConfigurationBuilder::postConfigure();
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
