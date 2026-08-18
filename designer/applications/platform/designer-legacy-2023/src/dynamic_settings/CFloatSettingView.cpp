///-------------------------------------------------------------------------------------------------
/// 
/// \file CFloatSettingView.cpp
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

#include "CFloatSettingView.hpp"
#include "../base.hpp"

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void OnButtonSettingFloatUpPressed(GtkButton* /*button*/, gpointer data) { static_cast<CFloatSettingView*>(data)->AdjustValue(1.0); }
static void OnButtonSettingFloatDownPressed(GtkButton* /*button*/, gpointer data) { static_cast<CFloatSettingView*>(data)->AdjustValue(-1.0); }
static void OnChangeCB(GtkEntry* /*entry*/, gpointer data) { static_cast<CFloatSettingView*>(data)->OnChange(); }

CFloatSettingView::CFloatSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx)
    : CAbstractSettingView(box, index, builderName, "settings_collection-hbox_setting_float"), m_kernelCtx(ctx)
{
    GtkWidget* settingWidget = CAbstractSettingView::getEntryFieldWidget();

    std::vector<GtkWidget*> widgets;
    CAbstractSettingView::extractWidget(settingWidget, widgets);
    m_entry = GTK_ENTRY(widgets[0]);

    g_signal_connect(G_OBJECT(m_entry), "changed", G_CALLBACK(OnChangeCB), this);

    g_signal_connect(G_OBJECT(widgets[1]), "clicked", G_CALLBACK(OnButtonSettingFloatUpPressed), this);
    g_signal_connect(G_OBJECT(widgets[2]), "clicked", G_CALLBACK(OnButtonSettingFloatDownPressed), this);

    CAbstractSettingView::initializeValue();
}

void CFloatSettingView::GetValue(CString& value) const { value = CString(gtk_entry_get_text(m_entry)); }

void CFloatSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;
    gtk_entry_set_text(m_entry, value);
    m_onValueSetting = false;
}

void CFloatSettingView::AdjustValue(const double amount)
{
    const double value    = m_kernelCtx.getConfigurationManager().expandAsFloat(gtk_entry_get_text(m_entry), 0) + amount;
    const std::string str = std::to_string(value);
    getBox().setSettingValue(GetSettingIndex(), str.c_str());
    SetValue(str.c_str());
}

void CFloatSettingView::OnChange()
{
    if (!m_onValueSetting) {
        const gchar* value = gtk_entry_get_text(m_entry);
        getBox().setSettingValue(GetSettingIndex(), value);
    }
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
