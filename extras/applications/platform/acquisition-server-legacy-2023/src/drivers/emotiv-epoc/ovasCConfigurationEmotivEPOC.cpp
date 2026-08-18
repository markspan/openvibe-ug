#if defined TARGET_HAS_ThirdPartyEmotivAPI

#include "ovasCConfigurationEmotivEPOC.h"

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationEmotivEPOC::CConfigurationEmotivEPOC(IDriverContext& ctx, const char* gtkBuilderFilename, bool& rUseGyroscope, CString& rPathToEmotivResearchSDK, uint32_t& rUserID)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_useGyroscope(rUseGyroscope), m_rPathToEmotivResearchSDK(rPathToEmotivResearchSDK), m_userID(rUserID) {}

bool CConfigurationEmotivEPOC::preConfigure()
{
    if (! CConfigurationBuilder::preConfigure()) { return false; }

    GtkToggleButton* gyro = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_gyro"));
    gtk_toggle_button_set_active(gyro, m_useGyroscope);

    GtkFileChooser* fileChooser = GTK_FILE_CHOOSER(gtk_builder_get_object(m_builder, "filechooserbutton"));
    gtk_file_chooser_set_current_folder(fileChooser, m_rPathToEmotivResearchSDK.toASCIIString());

    GtkSpinButton* userID = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_userid"));
    gtk_spin_button_set_value(userID, m_userID);
    return true;
}

bool CConfigurationEmotivEPOC::postConfigure()
{
    if (m_applyConfig)
    {
        GtkToggleButton* gyro = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_gyro"));
        m_useGyroscope        = gtk_toggle_button_get_active(gyro) ? true : false;

        GtkSpinButton* userID = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_userid"));
        gtk_spin_button_update(userID);
        m_userID = uint32_t(gtk_spin_button_get_value(userID));

        GtkFileChooser* fileChooser = GTK_FILE_CHOOSER(gtk_builder_get_object(m_builder, "filechooserbutton"));
        gchar* dir                  = gtk_file_chooser_get_filename(fileChooser);
        std::string tmpDstDir(dir);
        for (auto it = tmpDstDir.begin(); it < tmpDstDir.end(); ++it) { if ((*it) == '\\') { tmpDstDir.replace(it, it + 1, 1, '/'); } }
        tmpDstDir.push_back('/');
        m_rPathToEmotivResearchSDK = tmpDstDir.c_str();

        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Path to Emotiv Research SDK is set to [" << m_rPathToEmotivResearchSDK.toASCIIString() << "]\n";
    }

    if (! CConfigurationBuilder::postConfigure()) { return false; } // normal header is filled, ressources are realesed 

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyEmotivAPI
