#include "ovasCConfigurationDriverGenericOscillator.h"

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationDriverGenericOscillator::CConfigurationDriverGenericOscillator(IDriverContext& ctx, const char* gtkBuilderFilename,
                                                                             bool& sendPeriodicStimulations, double& stimulationInterval)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_sendPeriodicStimulations(sendPeriodicStimulations),
      m_stimulationInterval(stimulationInterval) {}

bool CConfigurationDriverGenericOscillator::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkToggleButton* sendPeriodicStims = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_send_periodic_stimulations"));
    GtkSpinButton* stimulationInterval = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_stimulation_interval"));

    gtk_toggle_button_set_active(sendPeriodicStims, m_sendPeriodicStimulations);
    gtk_spin_button_set_value(stimulationInterval, m_stimulationInterval);

    return true;
}

bool CConfigurationDriverGenericOscillator::postConfigure()
{
    if (m_applyConfig) {
        GtkToggleButton* sendPeriodicStims = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_send_periodic_stimulations"));
        GtkSpinButton* stimulationInterval = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_stimulation_interval"));

        m_sendPeriodicStimulations = (gtk_toggle_button_get_active(sendPeriodicStims) > 0);

        gtk_spin_button_update(stimulationInterval);
        m_stimulationInterval = gtk_spin_button_get_value(stimulationInterval);
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
