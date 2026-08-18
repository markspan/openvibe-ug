/*
 * \author Christoph Veigl, Yann Renard
 *
 * \copyright AGPL3
 *
 */
#include "ovasCConfigurationOpenEEGModularEEG.h"
#include <sstream>

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationOpenEEGModularEEG::CConfigurationOpenEEGModularEEG(const char* gtkBuilderFilename, uint32_t& usbIdx)
    : CConfigurationBuilder(gtkBuilderFilename), m_usbIdx(usbIdx) { m_listStore = gtk_list_store_new(1, G_TYPE_STRING); }

CConfigurationOpenEEGModularEEG::~CConfigurationOpenEEGModularEEG() { g_object_unref(m_listStore); }

bool CConfigurationOpenEEGModularEEG::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkComboBox* box = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    g_object_unref(m_listStore);
    m_listStore = gtk_list_store_new(1, G_TYPE_STRING);

    gtk_combo_box_set_model(box, GTK_TREE_MODEL(m_listStore));

    bool selected = false;

    for (uint32_t i = 1; i < 17; ++i)
    {
        std::stringstream ss;
#if defined TARGET_OS_Windows
        ss << "\\\\.\\COM" << i;
#elif defined TARGET_OS_Linux
        if(i<10) { ss <<"/dev/ttyS" << i; }
        else { ss << "/dev/ttyUSB%d"<< i-10; }
#endif
        gtk_combo_box_append_text(box, ss.str().c_str());
        if (m_usbIdx == i)
        {
            gtk_combo_box_set_active(box, i - 1);
            selected = true;
        }
    }

    if (!selected) { gtk_combo_box_set_active(box, 0); }

    return true;
}

bool CConfigurationOpenEEGModularEEG::postConfigure()
{
    GtkComboBox* box = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    if (m_applyConfig)
    {
        const int usbIndex = gtk_combo_box_get_active(box);
        if (usbIndex >= 0) { m_usbIdx = uint32_t(usbIndex) + 1; }
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
