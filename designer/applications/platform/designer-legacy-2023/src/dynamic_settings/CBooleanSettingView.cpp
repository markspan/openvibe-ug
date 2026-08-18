///-------------------------------------------------------------------------------------------------
/// 
/// \file CBooleanSettingView.cpp
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

#include "CBooleanSettingView.hpp"
#include "../base.hpp"

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void OnCheckbuttonSettingBooleanPressed(GtkToggleButton* /*button*/, gpointer data) { static_cast<CBooleanSettingView*>(data)->ToggleButtonClick(); }

static void OnInsertion(GtkEntry* /*entry*/, gpointer data) { static_cast<CBooleanSettingView*>(data)->OnChange(); }

CBooleanSettingView::CBooleanSettingView(Kernel::IBox& box, const size_t index, const CString& builderName)
    : CAbstractSettingView(box, index, builderName, "settings_collection-hbox_setting_boolean")
{
    GtkWidget* settingWidget = CAbstractSettingView::getEntryFieldWidget();

    std::vector<GtkWidget*> widgets;
    CAbstractSettingView::extractWidget(settingWidget, widgets);
    m_toggle = GTK_TOGGLE_BUTTON(widgets[1]);
    m_entry  = GTK_ENTRY(widgets[0]);

    g_signal_connect(G_OBJECT(m_entry), "changed", G_CALLBACK(OnInsertion), this);
    g_signal_connect(G_OBJECT(m_toggle), "toggled", G_CALLBACK(OnCheckbuttonSettingBooleanPressed), this);

    CAbstractSettingView::initializeValue();
}

void CBooleanSettingView::GetValue(CString& value) const { value = CString(gtk_entry_get_text(m_entry)); }

void CBooleanSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;
    if (value == CString("true")) {
        gtk_toggle_button_set_active(m_toggle, true);
        gtk_toggle_button_set_inconsistent(m_toggle, false);
    }
    else if (value == CString("false")) {
        gtk_toggle_button_set_active(m_toggle, false);
        gtk_toggle_button_set_inconsistent(m_toggle, false);
    }
    else { gtk_toggle_button_set_inconsistent(m_toggle, true); }

    gtk_entry_set_text(m_entry, value);
    m_onValueSetting = false;
}


void CBooleanSettingView::ToggleButtonClick()
{
    if (!m_onValueSetting) {
        if (gtk_toggle_button_get_active(m_toggle)) {
            getBox().setSettingValue(GetSettingIndex(), "true");
            SetValue("true");
        }
        else {
            getBox().setSettingValue(GetSettingIndex(), "false");
            SetValue("false");
        }
    }
}

void CBooleanSettingView::OnChange()
{
    if (!m_onValueSetting) {
        const gchar* value = gtk_entry_get_text(m_entry);
        getBox().setSettingValue(GetSettingIndex(), value);
    }
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
