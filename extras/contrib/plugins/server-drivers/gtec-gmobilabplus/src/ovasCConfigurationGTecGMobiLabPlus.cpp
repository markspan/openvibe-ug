/**
 * The gMobilab driver was contributed
 * by Lucie Daubigney from Supelec Metz
 */

#include "ovasCConfigurationGTecGMobiLabPlus.h"

#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationGTecGMobiLabPlus::CConfigurationGTecGMobiLabPlus(const char* gtkBuilderFilename, std::string& portName, bool& testMode)
    : CConfigurationBuilder(gtkBuilderFilename), m_portName(portName), m_testMode(testMode) {}

bool CConfigurationGTecGMobiLabPlus::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkEntry* port = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_port"));
    gtk_entry_set_text(port, m_portName.c_str());

    GtkToggleButton* testMode = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_testmode"));
    gtk_toggle_button_set_active(testMode, m_testMode);

    return true;
}


bool CConfigurationGTecGMobiLabPlus::postConfigure()
{
    if (m_applyConfig)
    {
        GtkEntry* port = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_port"));
        m_portName     = gtk_entry_get_text(port);

        GtkToggleButton* testMode = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_testmode"));
        m_testMode                = (gtk_toggle_button_get_active(testMode) > 0);
    }

    return CConfigurationBuilder::postConfigure();
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGMobiLabPlusAPI
