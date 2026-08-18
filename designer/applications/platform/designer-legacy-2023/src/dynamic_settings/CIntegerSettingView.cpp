///-------------------------------------------------------------------------------------------------
/// 
/// \file CIntegerSettingView.cpp
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

#include "CIntegerSettingView.hpp"
#include "../base.hpp"

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void OnButtonSettingIntegerUpPressed(GtkButton* /*button*/, gpointer data) { static_cast<CIntegerSettingView*>(data)->AdjustValue(1); }
static void OnButtonSettingIntegerDownPressed(GtkButton* /*button*/, gpointer data) { static_cast<CIntegerSettingView*>(data)->AdjustValue(-1); }
static void OnInsertion(GtkEntry* /*entry*/, gpointer data) { static_cast<CIntegerSettingView*>(data)->OnChange(); }

CIntegerSettingView::CIntegerSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx)
    : CAbstractSettingView(box, index, builderName, "settings_collection-hbox_setting_integer"), m_kernelCtx(ctx)
{
    GtkWidget* settingWidget = CAbstractSettingView::getEntryFieldWidget();

    std::vector<GtkWidget*> widgets;
    CAbstractSettingView::extractWidget(settingWidget, widgets);
    m_entry = GTK_ENTRY(widgets[0]);

    g_signal_connect(G_OBJECT(m_entry), "changed", G_CALLBACK(OnInsertion), this);

    g_signal_connect(G_OBJECT(widgets[1]), "clicked", G_CALLBACK(OnButtonSettingIntegerUpPressed), this);
    g_signal_connect(G_OBJECT(widgets[2]), "clicked", G_CALLBACK(OnButtonSettingIntegerDownPressed), this);

    CAbstractSettingView::initializeValue();
}

void CIntegerSettingView::GetValue(CString& value) const { value = CString(gtk_entry_get_text(m_entry)); }

void CIntegerSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;
    gtk_entry_set_text(m_entry, value);
    m_onValueSetting = false;
}

void CIntegerSettingView::AdjustValue(const int amount)
{
    const int64_t value   = m_kernelCtx.getConfigurationManager().expandAsInteger(gtk_entry_get_text(m_entry), 0) + amount;
    const std::string res = std::to_string(value);

    getBox().setSettingValue(GetSettingIndex(), res.c_str());
    SetValue(res.c_str());
}

void CIntegerSettingView::OnChange()
{
    if (!m_onValueSetting) {
        const gchar* value = gtk_entry_get_text(m_entry);
        getBox().setSettingValue(GetSettingIndex(), value);
    }
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
