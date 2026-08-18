///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCConfigurationBrainProductsBrainampSeries.cpp
/// \brief Brain Products Brainamp Series driver for OpenViBE
/// \author Yann Renard
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "ovasCConfigurationBrainProductsBrainampSeries.h"

#include "ovasIHeader.h"

#if defined TARGET_OS_Windows

#include "ovasCDriverBrainProductsBrainampSeries.h"

#include <iostream>
#include <windows.h>

namespace OpenViBE {
namespace AcquisitionServer {


static EParameter toIndex(const uint32_t value)
{
    switch (value) {
        case 1: return DecimationFactor_None;
        case 2: return DecimationFactor_2;
        case 4: return DecimationFactor_4;
        case 5: return DecimationFactor_5;
        case 8: return DecimationFactor_8;
        case 10: return DecimationFactor_10;
        default: return DecimationFactor_None;
    }
}

static uint32_t toValue(const EParameter parameter)
{
    switch (parameter) {
        case DecimationFactor_None: return 1;
        case DecimationFactor_2: return 2;
        case DecimationFactor_4: return 4;
        case DecimationFactor_5: return 5;
        case DecimationFactor_8: return 8;
        case DecimationFactor_10: return 10;
        default: return 1;
    }
}


static void ChannelDetailsPressedCB(GtkButton* /*button*/, void* data)
{
    static_cast<CConfigurationBrainProductsBrainampSeries*>(data)->buttonChannelDetailsPressedCB();
}

static void ComboboxDeviceChangedCB(GtkComboBox* /*comboBox*/, void* data)
{
    static_cast<CConfigurationBrainProductsBrainampSeries*>(data)->comboBoxDeviceChangedCB();
}

CConfigurationBrainProductsBrainampSeries::CConfigurationBrainProductsBrainampSeries(
    CDriverBrainProductsBrainampSeries& driver, const char* gtkBuilderFileName, uint32_t& usbIdx, uint32_t& decimationFactor,
    EParameter* channelSelected, EParameter* lowPassFilterFull, EParameter* resolutionFull, EParameter* dcCouplingFull,
    EParameter& lowPass, EParameter& resolution, EParameter& dcCoupling, EParameter& impedance)
    : CConfigurationBuilder(gtkBuilderFileName), m_driver(driver), m_usbIdx(usbIdx),
      m_decimationFactor(decimationFactor), m_channelSelected(channelSelected),
      m_lowPassFilterFull(lowPassFilterFull), m_resolutionFull(resolutionFull),
      m_dcCouplingFull(dcCouplingFull), m_lowPass(lowPass), m_resolution(resolution),
      m_dcCoupling(dcCoupling), m_impedance(impedance) {}

bool CConfigurationBrainProductsBrainampSeries::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    GtkComboBox* device     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));
    GtkComboBox* lowPass    = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_lowpass_filter"));
    GtkComboBox* decimation = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_decimation_factor"));
    GtkComboBox* resolution = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_resolution"));
    GtkComboBox* dcCoupling = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_dc_coupling"));
    GtkComboBox* impedance  = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_impedance"));

    g_signal_connect(gtk_builder_get_object(m_builder, "button_channel_details"), "pressed", G_CALLBACK(ChannelDetailsPressedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "combobox_device"), "changed", G_CALLBACK(ComboboxDeviceChangedCB), this);

    char buffer[1024];
    int count     = 0;
    bool selected = false;

    // autodetection of the connected device(s)
    for (uint32_t i = 0; i < 16; ++i) {
        if (m_driver.getDeviceDetails(i, nullptr, nullptr)) {
            sprintf(buffer, "USB port %i", i);
            gtk_combo_box_append_text(device, buffer);
            {
                if (m_usbIdx == i) {
                    gtk_combo_box_set_active(device, count);
                    selected = true;
                }
            }
            count++;
        }
    }

    if (!selected && count != 0) { gtk_combo_box_set_active(device, 0); }

    gtk_combo_box_set_active(lowPass, m_lowPass);
    gtk_combo_box_set_active(decimation, toIndex(m_decimationFactor));
    gtk_combo_box_set_active(resolution, m_resolution);
    gtk_combo_box_set_active(dcCoupling, m_dcCoupling);
    gtk_combo_box_set_active(impedance, m_impedance);

    return true;
}

bool CConfigurationBrainProductsBrainampSeries::postConfigure()
{
    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    if (m_applyConfig) {
        GtkComboBox* device     = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));
        GtkComboBox* lowPass    = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_lowpass_filter"));
        GtkComboBox* decimation = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_decimation_factor"));
        GtkComboBox* resolution = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_resolution"));
        GtkComboBox* dcCoupling = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_dc_coupling"));
        GtkComboBox* impedance  = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_impedance"));

        uint32_t usbIdx      = 0;
        const char* usbIndex = gtk_combo_box_get_active_text(device);
        if (usbIndex) { if (sscanf(usbIndex, "USB port %i", &usbIdx) == 1) { m_usbIdx = usbIdx; } }

        m_lowPass          = EParameter(gtk_combo_box_get_active(lowPass));
        m_decimationFactor = (toValue(EParameter(gtk_combo_box_get_active(decimation))));
        m_resolution       = EParameter(gtk_combo_box_get_active(resolution));
        m_dcCoupling       = EParameter(gtk_combo_box_get_active(dcCoupling));
        m_impedance        = EParameter(gtk_combo_box_get_active(impedance));
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

void CConfigurationBrainProductsBrainampSeries::buttonChannelDetailsPressedCB()
{
    GtkBuilder* builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, m_gtkBuilderFilename.c_str(), nullptr);

    GtkDialog* dialogChannelDetails     = GTK_DIALOG(gtk_builder_get_object(builder, "dialog_channel_details"));
    GtkTable* dialogChannelDetailsTable = GTK_TABLE(gtk_builder_get_object(builder, "table_content"));

    GtkTreeModel* treeModelLowPassFilterFull = GTK_TREE_MODEL(gtk_builder_get_object(builder, "model_lowpass_filter_full"));
    GtkTreeModel* treeModelResolutionFull    = GTK_TREE_MODEL(gtk_builder_get_object(builder, "model_resolution_full"));
    GtkTreeModel* treeModelDcCouplingFull    = GTK_TREE_MODEL(gtk_builder_get_object(builder, "model_dc_coupling_full"));

    gtk_table_resize(dialogChannelDetailsTable, 1, 5);
    gtk_table_resize(dialogChannelDetailsTable, 1 + m_header->getChannelCount(), 5);
    for (size_t i = 1; i < 1 + m_header->getChannelCount(); ++i) {
        GtkLabel* widgetIdx = GTK_LABEL(gtk_label_new((std::to_string(i) + ":").c_str()));
        gtk_label_set_justify(widgetIdx, GTK_JUSTIFY_LEFT);
        gtk_widget_show(GTK_WIDGET(widgetIdx));

        GtkLabel* widgetChannelName = GTK_LABEL(gtk_label_new(m_header->getChannelName(i-1)));
        gtk_label_set_justify(widgetChannelName, GTK_JUSTIFY_LEFT);
        // We hide this label as the channel name is not accurate
        // The update of the channel rename configuration (moved to the main application)
        // has broken this feature in the driver: real channel names are not passed to the driver
        // but are only kept at the server level. This problem should be adressed in future version.
        gtk_widget_hide(GTK_WIDGET(widgetChannelName));
        //::gtk_widget_show(GTK_WIDGET(l_pWidgetChannelName));

        GtkBox* widgetChannel = GTK_BOX(gtk_hbox_new(false, 2));
        gtk_box_pack_start(widgetChannel, GTK_WIDGET(widgetIdx), true, true, 0);
        gtk_box_pack_start(widgetChannel, GTK_WIDGET(widgetChannelName), true, true, 0);
        gtk_widget_show(GTK_WIDGET(widgetChannel));
        gtk_table_attach_defaults(dialogChannelDetailsTable, GTK_WIDGET(widgetChannel), 0, 1, i, i + 1);

        GtkToggleButton* widgetChannelSelected = GTK_TOGGLE_BUTTON(gtk_check_button_new());
        gtk_toggle_button_set_active(widgetChannelSelected, m_channelSelected[i - 1] == Channel_Selected);
        gtk_widget_show(GTK_WIDGET(widgetChannelSelected));
        gtk_table_attach_defaults(dialogChannelDetailsTable, GTK_WIDGET(widgetChannelSelected), 1, 2, i, i + 1);

        GtkComboBox* widgetLowPassFilterFull = GTK_COMBO_BOX(gtk_combo_box_new_text());
        gtk_combo_box_set_model(widgetLowPassFilterFull, treeModelLowPassFilterFull);
        gtk_combo_box_set_active(widgetLowPassFilterFull, gint(m_lowPassFilterFull[i - 1] + 1));
        gtk_widget_show(GTK_WIDGET(widgetLowPassFilterFull));
        gtk_table_attach_defaults(dialogChannelDetailsTable, GTK_WIDGET(widgetLowPassFilterFull), 2, 3, i, i + 1);

        GtkComboBox* widgetResolutionFull = GTK_COMBO_BOX(gtk_combo_box_new_text());
        gtk_combo_box_set_model(widgetResolutionFull, treeModelResolutionFull);
        gtk_combo_box_set_active(widgetResolutionFull, gint(m_resolutionFull[i - 1] + 1));
        gtk_widget_show(GTK_WIDGET(widgetResolutionFull));
        gtk_table_attach_defaults(dialogChannelDetailsTable, GTK_WIDGET(widgetResolutionFull), 3, 4, i, i + 1);

        GtkComboBox* widgetDcCouplingFull = GTK_COMBO_BOX(gtk_combo_box_new_text());
        gtk_combo_box_set_model(widgetDcCouplingFull, treeModelDcCouplingFull);
        gtk_combo_box_set_active(widgetDcCouplingFull, gint(m_dcCouplingFull[i - 1] + 1));
        gtk_widget_show(GTK_WIDGET(widgetDcCouplingFull));
        gtk_table_attach_defaults(dialogChannelDetailsTable, GTK_WIDGET(widgetDcCouplingFull), 4, 5, i, i + 1);
    }

    if (gtk_dialog_run(dialogChannelDetails) == GTK_RESPONSE_APPLY) {
        for (GList* list = dialogChannelDetailsTable->children; list; list = list->next) {
            GtkTableChild* tableChild = (GtkTableChild*)list->data;
            if (tableChild) {
                const gint i = tableChild->top_attach - 1;
                if (i >= 0) {
                    switch (tableChild->left_attach) {
                        case 1: m_channelSelected[i] = EParameter(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tableChild->widget)));
                            break;
                        case 2: m_lowPassFilterFull[i] = EParameter(gtk_combo_box_get_active(GTK_COMBO_BOX(tableChild->widget)) - 1);
                            break;
                        case 3: m_resolutionFull[i] = EParameter(gtk_combo_box_get_active(GTK_COMBO_BOX(tableChild->widget)) - 1);
                            break;
                        case 4: m_dcCouplingFull[i] = EParameter(gtk_combo_box_get_active(GTK_COMBO_BOX(tableChild->widget)) - 1);
                            break;
                        default: break;
                    }
                }
            }
        }
    }

    gtk_widget_hide(GTK_WIDGET(dialogChannelDetails));

    g_object_unref(builder);
}

void CConfigurationBrainProductsBrainampSeries::comboBoxDeviceChangedCB()
{
    GtkComboBox* comboBoxDevice = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));
    const char* usbIndex        = gtk_combo_box_get_active_text(comboBoxDevice);
    if (usbIndex) {
        uint32_t usbIdx = -1;
        if (sscanf(usbIndex, "USB port %i", &usbIdx) == 1) {
            uint32_t amplifierType[4];
            uint32_t nAmplifier = 0;
            if (m_driver.getDeviceDetails(usbIdx, &nAmplifier, amplifierType)) {
                bool correctMontage = true;
                m_header->setChannelCount(nAmplifier * 32);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_nChannels), nAmplifier * 32);

                for (uint32_t i = 0; i < BrainAmp_MaximumAmplifierCount; ++i) {
                    const std::string name    = "label_multiamp_montage_" + std::to_string(i + 1);
                    GtkLabel* multiampMontage = GTK_LABEL(gtk_builder_get_object(m_builder, name.c_str()));
                    gtk_label_set_label(multiampMontage, getDeviceType(amplifierType[i]));
                    if (i >= 1 && amplifierType[i - 1] == AmplifierType_None && amplifierType[i] != AmplifierType_None) { correctMontage = false; }
                }

                GtkWidget* montageTable = GTK_WIDGET(gtk_builder_get_object(m_builder, "table_multiamp_montage"));
                GtkWidget* montageIcon  = GTK_WIDGET(gtk_builder_get_object(m_builder, "image_montage_status_icon"));
                if (correctMontage) {
                    const char* text = "The multi amplifier montage is correct";
                    gtk_image_set_from_stock(GTK_IMAGE(montageIcon), GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON);
                    gtk_widget_set_tooltip_markup(montageIcon, text);
                    gtk_widget_set_tooltip_markup(montageTable, text);
                }
                else {
                    const char* text = "The multi amplifier montage is not correct - avoid empty slots before filled slots";
                    gtk_image_set_from_stock(GTK_IMAGE(montageIcon), GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON);
                    gtk_widget_set_tooltip_markup(montageIcon, text);
                    gtk_widget_set_tooltip_markup(montageTable, text);
                }
            }
        }
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_OS_Windows
