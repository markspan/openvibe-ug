///-------------------------------------------------------------------------------------------------
/// 
/// \file CBitMaskSettingView.cpp
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

#include "CBitMaskSettingView.hpp"
#include "../base.hpp"

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void OnCheckbuttonPressed(GtkToggleButton* /*button*/, gpointer data) { static_cast<CBitMaskSettingView*>(data)->OnChange(); }

CBitMaskSettingView::CBitMaskSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx,
                                         const CIdentifier& typeID)
    : CAbstractSettingView(box, index, builderName, "settings_collection-table_setting_bitmask"), m_typeID(typeID), m_kernelCtx(ctx)
{
    GtkWidget* settingWidget = CAbstractSettingView::getEntryFieldWidget();

    const gint tableSize   = guint((m_kernelCtx.getTypeManager().getBitMaskEntryCount(m_typeID) + 1) >> 1);
    GtkTable* bitMaskTable = GTK_TABLE(settingWidget);
    gtk_table_resize(bitMaskTable, 2, tableSize);

    for (size_t i = 0; i < m_kernelCtx.getTypeManager().getBitMaskEntryCount(m_typeID); ++i) {
        CString name;
        uint64_t value;
        if (m_kernelCtx.getTypeManager().getBitMaskEntry(m_typeID, i, name, value)) {
            GtkWidget* button = gtk_check_button_new();
            gtk_table_attach_defaults(bitMaskTable, button, guint(i & 1), guint((i & 1) + 1), guint(i >> 1), guint((i >> 1) + 1));
            gtk_button_set_label(GTK_BUTTON(button), name.toASCIIString());
            m_toggleButton.push_back(GTK_TOGGLE_BUTTON(button));
            g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(OnCheckbuttonPressed), this);
        }
    }
    gtk_widget_show_all(GTK_WIDGET(bitMaskTable));

    CAbstractSettingView::initializeValue();
}


void CBitMaskSettingView::GetValue(CString& value) const
{
    std::string res;
    for (auto& toggle : m_toggleButton) {
        if (gtk_toggle_button_get_active(toggle)) {
            if (!res.empty()) { res += ':'; }
            res += gtk_button_get_label(GTK_BUTTON(toggle));
        }
    }
    value = res.c_str();
}

void CBitMaskSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;
    const std::string str(value);

    for (auto& toggle : m_toggleButton) {
        const gchar* label = gtk_button_get_label(GTK_BUTTON(toggle));
        if (str.find(label) != std::string::npos) { gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), true); }
        else { gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), false); }
    }

    m_onValueSetting = false;
}

void CBitMaskSettingView::OnChange()
{
    if (!m_onValueSetting) {
        CString value;
        this->GetValue(value);
        getBox().setSettingValue(GetSettingIndex(), value);
    }
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
