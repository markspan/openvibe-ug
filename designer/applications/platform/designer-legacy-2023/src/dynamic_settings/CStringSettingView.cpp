///-------------------------------------------------------------------------------------------------
/// 
/// \file CStringSettingView.cpp
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

#include "CStringSettingView.hpp"
#include "../base.hpp"

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void OnChangeCB(GtkEntry* /*entry*/, gpointer data) { static_cast<CStringSettingView*>(data)->OnChange(); }

CStringSettingView::CStringSettingView(Kernel::IBox& box, const size_t index, const CString& builderName)
    : CAbstractSettingView(box, index, builderName, "settings_collection-entry_setting_string")
{
    GtkWidget* settingWidget = CAbstractSettingView::getEntryFieldWidget();

    m_entry = GTK_ENTRY(settingWidget);
    g_signal_connect(G_OBJECT(m_entry), "changed", G_CALLBACK(OnChangeCB), this);

    CAbstractSettingView::initializeValue();
}

void CStringSettingView::GetValue(CString& value) const { value = CString(gtk_entry_get_text(m_entry)); }

void CStringSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;
    gtk_entry_set_text(m_entry, value);
    m_onValueSetting = false;
}

void CStringSettingView::OnChange()
{
    if (!m_onValueSetting) {
        const gchar* value = gtk_entry_get_text(m_entry);
        getBox().setSettingValue(GetSettingIndex(), value);
    }
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
