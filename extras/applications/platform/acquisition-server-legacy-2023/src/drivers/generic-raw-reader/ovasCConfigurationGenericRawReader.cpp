#include "ovasCConfigurationGenericRawReader.h"

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationGenericRawReader::CConfigurationGenericRawReader(const char* gtkBuilderFilename, bool& limitSpeed, uint32_t& sampleFormat, uint32_t& sampleEndian,
                                                               uint32_t& startSkip, uint32_t& headerSkip, uint32_t& footerSkip, CString& filename)
    : CConfigurationNetworkBuilder(gtkBuilderFilename), m_limitSpeed(limitSpeed), m_sampleFormat(sampleFormat), m_sampleEndian(sampleEndian),
      m_startSkip(startSkip), m_headerSkip(headerSkip), m_footerSkip(footerSkip), m_filename(filename) {}

bool CConfigurationGenericRawReader::preConfigure()
{
    if (!CConfigurationNetworkBuilder::preConfigure()) { return false; }

    GtkToggleButton* speedLimit = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_limit_speed"));
    GtkEntry* filename          = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_filename"));
    GtkComboBox* endianness     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_endianness"));
    GtkComboBox* sampleType     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sample_type"));
    GtkSpinButton* startSize    = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_start_size"));
    GtkSpinButton* headerSize   = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_header_size"));
    GtkSpinButton* footerSize   = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_footer_size"));

    gtk_toggle_button_set_active(speedLimit, m_limitSpeed ? TRUE : FALSE);
    gtk_entry_set_text(filename, m_filename.toASCIIString());
    gtk_combo_box_set_active(endianness, m_sampleEndian);
    gtk_combo_box_set_active(sampleType, m_sampleFormat);
    gtk_spin_button_set_value(startSize, m_startSkip);
    gtk_spin_button_set_value(headerSize, m_headerSkip);
    gtk_spin_button_set_value(footerSize, m_footerSkip);

    return true;
}

bool CConfigurationGenericRawReader::postConfigure()
{
    if (m_applyConfig) {
        GtkToggleButton* speedLimit = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_limit_speed"));
        GtkEntry* filename          = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_filename"));
        GtkComboBox* endianness     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_endianness"));
        GtkComboBox* sampleType     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sample_type"));
        GtkSpinButton* startSize    = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_start_size"));
        GtkSpinButton* headerSize   = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_header_size"));
        GtkSpinButton* footerSize   = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_footer_size"));

        gtk_spin_button_update(startSize);
        gtk_spin_button_update(headerSize);
        gtk_spin_button_update(footerSize);

        m_limitSpeed   = gtk_toggle_button_get_active(speedLimit) ? true : false;
        m_filename     = gtk_entry_get_text(filename);
        m_sampleEndian = uint32_t(gtk_combo_box_get_active(endianness));
        m_sampleFormat = uint32_t(gtk_combo_box_get_active(sampleType));
        m_startSkip    = uint32_t(gtk_spin_button_get_value(startSize));
        m_headerSkip   = uint32_t(gtk_spin_button_get_value(headerSize));
        m_footerSkip   = uint32_t(gtk_spin_button_get_value(footerSize));
    }

    if (!CConfigurationNetworkBuilder::postConfigure()) { return false; }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
