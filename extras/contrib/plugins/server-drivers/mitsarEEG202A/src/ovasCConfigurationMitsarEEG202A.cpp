#if defined(TARGET_HAS_ThirdPartyMitsar)
#if defined TARGET_OS_Windows

#include "ovasCConfigurationMitsarEEG202A.h"

#include <windows.h>

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationMitsarEEG202A::CConfigurationMitsarEEG202A(const char* gtKbuilderXMLFileName, uint32_t& refIndex, bool& eventAndBioChannelsState)
    : CConfigurationBuilder(gtKbuilderXMLFileName), m_rEventAndBioChannelsState(eventAndBioChannelsState), m_rRefIdx(refIndex) {}

bool CConfigurationMitsarEEG202A::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkComboBox* ref = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_ref"));
    //::GtkComboBox* comboBox_Chan = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_channels"));

    GtkToggleButton* buttonDrift           = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_driftCorrection"));
    GtkToggleButton* buttonHardwareTagging = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_EventAndBioChannels"));

    gtk_combo_box_set_active(ref, 0);
    //::gtk_combo_box_set_active(comboBox_Chan, 0);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttonHardwareTagging), m_rEventAndBioChannelsState);
    return true;
}

bool CConfigurationMitsarEEG202A::postConfigure()
{
    GtkComboBox* ref = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_ref"));
    //::GtkComboBox* comboBox_Chan=GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_channels"));

    GtkToggleButton* buttonDrift           = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_driftCorrection"));
    GtkToggleButton* buttonHardwareTagging = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_EventAndBioChannels"));

    m_rRefIdx = uint32_t(gtk_combo_box_get_active(ref));

    m_rEventAndBioChannelsState = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttonHardwareTagging)) > 0);

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
#endif
