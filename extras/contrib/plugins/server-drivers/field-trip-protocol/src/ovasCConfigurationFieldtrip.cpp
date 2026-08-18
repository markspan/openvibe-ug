#if defined(TARGET_HAS_PThread)

#include "ovasCConfigurationFieldtrip.h"

#include <toolkit/ovtk_all.h>

#include <iostream>
#include <fstream>
#include <list>

namespace OpenViBE {
namespace AcquisitionServer {

bool CConfigurationFieldtrip::preConfigure()
{
    const bool res = CConfigurationBuilder::preConfigure();

    m_pHostName     = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_host_name"));
    m_pHostPort     = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_host_port"));
    m_pMinSamples   = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_minSamples"));
    m_pSRCorrection = GTK_WIDGET(gtk_builder_get_object(m_builder, "checkbutton_SRCorrection"));

    gtk_spin_button_set_range(GTK_SPIN_BUTTON(m_pMinSamples), 1.0, 10000.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_pMinSamples), m_minSamples);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_pHostPort), m_hostPort);
    gtk_entry_set_text(GTK_ENTRY(m_pHostName), m_hostName.toASCIIString());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pSRCorrection), m_srCorrection);

    return res;
}

bool CConfigurationFieldtrip::postConfigure()
{
    if (m_applyConfig)
    {
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_pMinSamples));
        gtk_spin_button_update(GTK_SPIN_BUTTON(m_pHostPort));

        m_minSamples    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pMinSamples));
        m_hostPort      = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pHostPort));
        m_hostName      = gtk_entry_get_text(GTK_ENTRY(m_pHostName));
        m_srCorrection = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_pSRCorrection)) > 0);
    }

    return CConfigurationBuilder::postConfigure();
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif  //#if defined(TARGET_HAS_PThread)