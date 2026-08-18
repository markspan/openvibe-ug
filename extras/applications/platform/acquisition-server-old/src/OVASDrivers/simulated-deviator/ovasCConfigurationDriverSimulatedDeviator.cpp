#include "ovasCConfigurationDriverSimulatedDeviator.h"

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationDriverSimulatedDeviator::CConfigurationDriverSimulatedDeviator(IDriverContext& ctx, const char* gtkBuilderFilename, bool& sendPeriodicStims,
                                                                             double& offset, double& spread, double& maxDev, double& pullback, double& update,
                                                                             uint64_t& wavetype, double& freezeFrequency, double& freezeDuration)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_sendPeriodicStimulations(sendPeriodicStims), m_Offset(offset), m_Spread(spread),
      m_MaxDev(maxDev), m_Pullback(pullback), m_Update(update), m_Wavetype(wavetype), m_FreezeFrequency(freezeFrequency), m_FreezeDuration(freezeDuration) {}

bool CConfigurationDriverSimulatedDeviator::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkToggleButton* sendPeriodicStims = GTK_TOGGLE_BUTTON(
        gtk_builder_get_object(m_builder, "checkbutton_send_periodic_stimulations"));

    gtk_toggle_button_set_active(sendPeriodicStims, m_sendPeriodicStimulations);

    GtkSpinButton* tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_offset"));
    gtk_spin_button_set_digits(tmp, 2);
    gtk_spin_button_set_value(tmp, m_Offset);

    tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_spread"));
    gtk_spin_button_set_digits(tmp, 3);
    gtk_spin_button_set_value(tmp, m_Spread);

    tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_maxdev"));
    gtk_spin_button_set_digits(tmp, 3);
    gtk_spin_button_set_value(tmp, m_MaxDev);

    tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_pullback"));
    gtk_spin_button_set_digits(tmp, 3);
    gtk_spin_button_set_value(tmp, m_Pullback);

    tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_update"));
    gtk_spin_button_set_digits(tmp, 3);
    gtk_spin_button_set_value(tmp, m_Update);

    GtkComboBox* wavetype = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_wavetype"));
    gtk_combo_box_set_active(wavetype, gint(m_Wavetype));

    tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_freeze_frequency"));
    gtk_spin_button_set_digits(tmp, 3);
    gtk_spin_button_set_value(tmp, m_FreezeFrequency);

    tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_freeze_duration"));
    gtk_spin_button_set_digits(tmp, 3);
    gtk_spin_button_set_value(tmp, m_FreezeDuration);

    return true;
}

bool CConfigurationDriverSimulatedDeviator::postConfigure()
{
    if (m_applyConfig) {
        GtkToggleButton* sendPeriodicStims = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_send_periodic_stimulations"));

        m_sendPeriodicStimulations = (gtk_toggle_button_get_active(sendPeriodicStims) > 0);

        GtkSpinButton* tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_offset"));
        gtk_spin_button_update(tmp);
        m_Offset = gtk_spin_button_get_value(tmp);
        tmp      = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_spread"));
        gtk_spin_button_update(tmp);
        m_Spread = gtk_spin_button_get_value(tmp);
        tmp      = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_maxdev"));
        gtk_spin_button_update(tmp);
        m_MaxDev = gtk_spin_button_get_value(tmp);
        tmp      = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_pullback"));
        gtk_spin_button_update(tmp);
        m_Pullback = gtk_spin_button_get_value(tmp);
        tmp        = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_update"));
        gtk_spin_button_update(tmp);
        m_Update = gtk_spin_button_get_value(tmp);

        GtkComboBox* wavetype = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_wavetype"));
        m_Wavetype            = uint64_t(gtk_combo_box_get_active(wavetype));

        tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_freeze_frequency"));
        gtk_spin_button_update(tmp);
        m_FreezeFrequency = gtk_spin_button_get_value(tmp);

        tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_freeze_duration"));
        gtk_spin_button_update(tmp);
        m_FreezeDuration = gtk_spin_button_get_value(tmp);
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
