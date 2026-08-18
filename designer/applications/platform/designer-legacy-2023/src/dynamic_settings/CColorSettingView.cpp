///-------------------------------------------------------------------------------------------------
/// 
/// \file CColorSettingView.cpp
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

#include "CColorSettingView.hpp"
#include "../base.hpp"

#include <cmath>

namespace OpenViBE {
namespace Designer {
namespace Setting {

static int Color2Percent(const guint16 color) { return int(round(color / 655.350)); }   // c * 100 / 65535
static guint16 Percent2Color(const int color) { return guint16(color * 655.35); }       // c * 65535 / 100

static void OnButtonSettingColorChoosePressed(GtkColorButton* /*button*/, gpointer data) { static_cast<CColorSettingView*>(data)->SelectColor(); }
static void OnChangeCB(GtkEntry* /*entry*/, gpointer data) { static_cast<CColorSettingView*>(data)->OnChange(); }

CColorSettingView::CColorSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx)
    : CAbstractSettingView(box, index, builderName, "settings_collection-hbox_setting_color"), m_kernelCtx(ctx)
{
    GtkWidget* settingWidget = CAbstractSettingView::getEntryFieldWidget();

    std::vector<GtkWidget*> widgets;
    CAbstractSettingView::extractWidget(settingWidget, widgets);
    m_entry  = GTK_ENTRY(widgets[0]);
    m_button = GTK_COLOR_BUTTON(widgets[1]);

    g_signal_connect(G_OBJECT(m_entry), "changed", G_CALLBACK(OnChangeCB), this);
    g_signal_connect(G_OBJECT(m_button), "color-set", G_CALLBACK(OnButtonSettingColorChoosePressed), this);

    CAbstractSettingView::initializeValue();
}


void CColorSettingView::GetValue(CString& value) const { value = CString(gtk_entry_get_text(m_entry)); }

void CColorSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;
    int r            = 0, g = 0, b = 0;
    sscanf(m_kernelCtx.getConfigurationManager().expand(value).toASCIIString(), "%i,%i,%i", &r, &g, &b);

    GdkColor color;
    color.red   = Percent2Color(r);
    color.green = Percent2Color(g);
    color.blue  = Percent2Color(b);
    gtk_color_button_set_color(m_button, &color);

    gtk_entry_set_text(m_entry, value);
    m_onValueSetting = false;
}

void CColorSettingView::SelectColor()
{
    GdkColor color;
    gtk_color_button_get_color(m_button, &color);
    const std::string value = std::to_string(Color2Percent(color.red)) + "," + std::to_string(Color2Percent(color.green)) + ","
                              + std::to_string(Color2Percent(color.blue));
    getBox().setSettingValue(GetSettingIndex(), value.c_str());
    SetValue(value.c_str());
}

void CColorSettingView::OnChange()
{
    if (!m_onValueSetting) {
        const gchar* value = gtk_entry_get_text(m_entry);
        getBox().setSettingValue(GetSettingIndex(), value);
    }
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
