///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Lesser General Public
/// License as published by the Free Software Foundation; either
/// version 2.1 of the License, or (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
/// MA 02110-1301  USA
///-------------------------------------------------------------------------------------------------

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#include "ovasCConfigurationBioSemiActiveTwo.h"
#include "ovasIHeader.h"
#include "mCBridgeBioSemiActiveTwo.h"

#include <system/ovCTime.h>

#include <cstring>

namespace OpenViBE {
namespace AcquisitionServer {

static void gtk_combo_box_set_active_text(GtkComboBox* pComboBox, const gchar* sActiveText)
{
    GtkTreeModel* treeModel = gtk_combo_box_get_model(pComboBox);
    GtkTreeIter it;
    int index        = 0;
    gchar* entryName = nullptr;
    if (gtk_tree_model_get_iter_first(treeModel, &it))
    {
        do
        {
            gtk_tree_model_get(treeModel, &it, 0, &entryName, -1);
            if (std::string(entryName) == std::string(sActiveText))
            {
                gtk_combo_box_set_active(pComboBox, index);
                return;
            }
            index++;
        } while (gtk_tree_model_iter_next(treeModel, &it));
    }
}

CConfigurationBioSemiActiveTwo::CConfigurationBioSemiActiveTwo(const char* gtkBuilderFilename, bool& useExChannels)
    : CConfigurationBuilder(gtkBuilderFilename), m_useEXChannels(useExChannels) {}

bool CConfigurationBioSemiActiveTwo::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }

    // deduced from connection to device, cannot be edited.
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_sampling_frequency")), false);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_speed_mode")), false);

    CBridgeBioSemiActiveTwo bridge;
    bool status = false;
    if (bridge.open() && bridge.start())
    {
        // to let the device send a first packet from which the bridge can deduce the SF and channel count
        System::Time::sleep(500);
        if (bridge.read() > 0)
        {
            gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(m_builder, "image_status")),GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON);

            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels")), true);
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_apply")), true);
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_change_channel_names")), true);

            // If this option is selected then 8 channels of the header are dedicated to the EX channels and they should be removed of the displayed channel count
            if (m_useEXChannels)
            {
                gtk_adjustment_set_value(
                    GTK_ADJUSTMENT(gtk_builder_get_object(m_builder, "adjustment_channel_count")),
                    gtk_adjustment_get_value(GTK_ADJUSTMENT(gtk_builder_get_object(m_builder, "adjustment_channel_count"))) - 8);
            }
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "checkbutton_ex_channel")), true);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_ex_channel")), m_useEXChannels);

            //Set maximum in function of the speedmode used
            gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(m_builder, "adjustment_channel_count")), bridge.getElectrodeChannelCount());

            std::stringstream ss;
            ss << bridge.getSamplingFrequency();
            gtk_combo_box_set_active_text(GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency")), ss.str().c_str());

            ss.str("");
            ss << bridge.getSpeedMode();
            gtk_combo_box_set_active_text(GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_speed_mode")), ss.str().c_str());

            gtk_label_set_markup(
                GTK_LABEL(gtk_builder_get_object(m_builder, "label_device_mark")),
                (bridge.isDeviceMarkII() ? "- <i>ActiveTwo Mark II</i> -" : "- <i>ActiveTwo Mark I</i> -"));
            gtk_label_set_markup(
                GTK_LABEL(gtk_builder_get_object(m_builder, "label_device_battery")),
                (bridge.isBatteryLow() ? "<span color=\"red\"><b>LOW BATTERY</b></span> -" : "<b>BATTERY OK</b>"));

            // discard any data.
            bridge.discard();

            status = true;
        }
        bridge.stop();
        bridge.close();
    }

    if (!status)
    {
        gtk_combo_box_set_active_text(GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sampling_frequency")), "");
        gtk_combo_box_set_active_text(GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_speed_mode")), "");
        gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(m_builder, "image_status")),GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON);

        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_number_of_channels")), false);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_apply")), false);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_change_channel_names")), false);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "checkbutton_ex_channel")), false);
    }


    return true;
}

bool CConfigurationBioSemiActiveTwo::postConfigure()
{
    if (m_applyConfig)
    {
        m_useEXChannels = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_ex_channel"))) ? true : false;
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
