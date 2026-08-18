#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

#include <bitset>
#include <sstream>

#include "ovasCConfigurationEEGO.h"

namespace OpenViBE {
namespace AcquisitionServer {

// Function to set a predefined string in the combobox.
// Copied from ovasCConfigurationBuilder. Seems to be OK, albeit it is strange to have the code duplication,
// but other amplifier drivers do it too.
// Would be nicer if the method would move to some utilities provider.
static void GTKComboBoxSetActiveText(GtkComboBox* box, const gchar* text)
{
    GtkTreeModel* treeModel = gtk_combo_box_get_model(box);
    GtkTreeIter it;
    int index   = 0;
    gchar* name = nullptr;
    if (gtk_tree_model_get_iter_first(treeModel, &it))
    {
        do
        {
            gtk_tree_model_get(treeModel, &it, 0, &name, -1);
            if (std::string(name) == std::string(text))
            {
                gtk_combo_box_set_active(box, index);
                return;
            }
            index++;
        } while (gtk_tree_model_iter_next(treeModel, &it));
    }
}

// If you added more reference attribute, initialize them here
CConfigurationEEGO::CConfigurationEEGO(IDriverContext& ctx, const char* gtkBuilderFilename, CHeaderEEGO& eegoHeader)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_eegoHeader(eegoHeader) {}

bool CConfigurationEEGO::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    // Insert here the pre-configure code.
    // For example, you may want to check if a device is currently connected
    // and if more than one are connected. Then you can list in a dedicated combo-box
    // the device currently connected so the user can choose which one he wants to acquire from.
    // Steffen Heimes: This is actually a TODO:

    GtkWidget* widget  = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_signal_range_eeg"));
    m_eegRangeComboBox = GTK_COMBO_BOX(widget);

    widget             = GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_signal_range_bip"));
    m_bipRangeComboBox = GTK_COMBO_BOX(widget);

    widget         = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_eeg_mask"));
    m_eegEntryMask = GTK_ENTRY(widget);
    g_signal_connect(widget, "changed", G_CALLBACK(updateChannelNumCB), this);

    widget         = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_bip_mask"));
    m_bipEntryMask = GTK_ENTRY(widget);
    g_signal_connect(widget, "changed", G_CALLBACK(updateChannelNumCB), this);

    widget          = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_num_channels"));
    m_nChannelEntry = GTK_ENTRY(widget);

    if (!m_eegRangeComboBox) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not connect to range widget \n"; }
    else
    {
        if (m_eegoHeader.isEEGRangeSet())
        {
            const std::string range = std::to_string(m_eegoHeader.getEEGRange());
            GTKComboBoxSetActiveText(m_eegRangeComboBox, range.c_str());
        }
        else { gtk_combo_box_set_active(m_eegRangeComboBox, 0); }

        if (m_eegoHeader.isBIPRangeSet())
        {
            const std::string range = std::to_string(m_eegoHeader.getBIPRange());
            GTKComboBoxSetActiveText(m_bipRangeComboBox, range.c_str());
        }
        else { gtk_combo_box_set_active(m_bipRangeComboBox, 0); }
    }

    if (m_eegoHeader.isBIPMaskSet()) { gtk_entry_set_text(m_bipEntryMask, m_eegoHeader.getBIPMask()); }
    if (m_eegoHeader.isEEGMaskSet()) { gtk_entry_set_text(m_eegEntryMask, m_eegoHeader.getEEGMask()); }

    return true;
}

bool CConfigurationEEGO::postConfigure()
{
    if (m_applyConfig)
    {
        const gchar* rangeEEG = gtk_combo_box_get_active_text(m_eegRangeComboBox);
        const gchar* rangeBip = gtk_combo_box_get_active_text(m_bipRangeComboBox);
        const gchar* maskBip  = gtk_entry_get_text(m_bipEntryMask);
        const gchar* maskEEG  = gtk_entry_get_text(m_eegEntryMask);

        m_eegoHeader.setBIPMask(maskBip);
        m_eegoHeader.setEEGMask(maskEEG);
        m_eegoHeader.setEEGRange(rangeEEG ? atoi(rangeEEG) : 0);
        m_eegoHeader.setBIPRange(rangeBip ? atoi(rangeBip) : 0);
    }

    if (!CConfigurationBuilder::postConfigure()
    ) { return false; } // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are released

    // get sum of max active channels. It would be a good idea to use the amplifier connected as a source of the maximum of available channels.
    const uint64_t bip = m_eegoHeader.getBIPMaskInt();
    const uint64_t eeg = m_eegoHeader.getEEGMaskInt();

    const std::bitset<64> bitsetEEG(eeg);
    const std::bitset<24> bitsetBip(bip);

    m_header->setChannelCount(bitsetEEG.count() + bitsetBip.count() + 2);
    // Plus status channels: trigger and sample counter

    return true;
}

/// GTK Callbacks
/* static */
void CConfigurationEEGO::updateChannelNumCB(GtkWidget* /*widget*/, CConfigurationEEGO* pThis)
{
    // get the values
    const gchar* maskBip = gtk_entry_get_text(pThis->m_bipEntryMask);
    const gchar* maskEEG = gtk_entry_get_text(pThis->m_eegEntryMask);

    uint64_t bip;
    uint64_t eeg;

    const bool bipSuccess = CHeaderEEGO::convertMask(maskBip, bip);
    const bool eegSuccess = CHeaderEEGO::convertMask(maskEEG, eeg);

    const std::bitset<64> bitsetEEG(eeg);
    const std::bitset<24> bitsetBip(bip);

    // format them
    std::stringstream ss;
    if (eegSuccess) { ss << bitsetEEG.count(); }
    else { ss << "Error"; }

    ss << " + ";

    if (bipSuccess) { ss << bitsetBip.count(); }
    else { ss << "Error"; }

    ss << " + 2; (EEG + BIP + STATUS)";

    // set text
    gtk_entry_set_text(pThis->m_nChannelEntry, ss.str().c_str());

    const uint32_t nChannel = bitsetEEG.count() + bitsetBip.count() + 2;

    pThis->m_header->setChannelCount(nChannel);

    // Workaround! The current channel number is not derived from the channel count. It is retrieved from the /here hidden/
    // widget when the channel editing window is opening. Thus we have to set the value there too.
    if (GTK_SPIN_BUTTON(pThis->m_nChannels)) { gtk_spin_button_set_value(GTK_SPIN_BUTTON(pThis->m_nChannels), nChannel); }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyEEGOAPI
