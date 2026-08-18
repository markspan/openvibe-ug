///-------------------------------------------------------------------------------------------------
/// 
/// \file CColorGradientSettingView.cpp
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

#include "CColorGradientSettingView.hpp"
#include "../base.hpp"
#include <visualization-toolkit/ovvizColorGradient.h>

#include <cmath>

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void OnColorGradientColorButtonPressed(GtkColorButton* button, gpointer data) { static_cast<CColorGradientSettingView*>(data)->ColorChange(button); }

static void OnButtonSettingColorGradientConfigurePressed(GtkButton* /*button*/, gpointer data)
{
    static_cast<CColorGradientSettingView*>(data)->ConfigurePressed();
}

static void OnRefreshColorGradient(GtkWidget* /*widget*/, GdkEventExpose* /*event*/, gpointer data)
{
    static_cast<CColorGradientSettingView*>(data)->RefreshColorGradient();
}

static void OnGtkWidgetDestroy(GtkWidget* widget, gpointer /*data*/) { gtk_widget_destroy(widget); }
static void OnInitializeColorGradient(GtkWidget* /*widget*/, gpointer data) { static_cast<CColorGradientSettingView*>(data)->InitializeGradient(); }
static void OnButtonColorGradientAddPressed(GtkButton* /*button*/, gpointer data) { static_cast<CColorGradientSettingView*>(data)->AddColor(); }
static void OnButtonColorGradientRemovePressed(GtkButton* /*button*/, gpointer data) { static_cast<CColorGradientSettingView*>(data)->RemoveColor(); }
static void OnColorGradientSpinButtonValueChanged(GtkSpinButton* button, gpointer data) { static_cast<CColorGradientSettingView*>(data)->SpinChange(button); }
static void OnChangeCB(GtkEntry* /*entry*/, gpointer data) { static_cast<CColorGradientSettingView*>(data)->OnChange(); }

CColorGradientSettingView::CColorGradientSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx)
    : CAbstractSettingView(box, index, builderName, "settings_collection-hbox_setting_color_gradient"), m_kernelCtx(ctx), m_builderName(builderName)
{
    GtkWidget* settingWidget = CAbstractSettingView::getEntryFieldWidget();

    std::vector<GtkWidget*> widgets;
    CAbstractSettingView::extractWidget(settingWidget, widgets);
    m_entry = GTK_ENTRY(widgets[0]);

    g_signal_connect(G_OBJECT(m_entry), "changed", G_CALLBACK(OnChangeCB), this);
    g_signal_connect(G_OBJECT(widgets[1]), "clicked", G_CALLBACK(OnButtonSettingColorGradientConfigurePressed), this);

    CAbstractSettingView::initializeValue();
}

void CColorGradientSettingView::GetValue(CString& value) const { value = CString(gtk_entry_get_text(m_entry)); }

void CColorGradientSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;
    gtk_entry_set_text(m_entry, value);
    m_onValueSetting = false;
}

void CColorGradientSettingView::ConfigurePressed()
{
    GtkBuilder* builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, m_builderName.toASCIIString(), nullptr);
    gtk_builder_connect_signals(builder, nullptr);

    m_dialog = GTK_WIDGET(gtk_builder_get_object(builder, "setting_editor-color_gradient-dialog"));

    const CString initialGradient = m_kernelCtx.getConfigurationManager().expand(gtk_entry_get_text(m_entry));
    CMatrix colorGradient;

    VisualizationToolkit::ColorGradient::parse(colorGradient, initialGradient);
    m_colorGradient.resize(std::max<size_t>(colorGradient.getDimensionSize(1), 2));
    for (size_t i = 0; i < colorGradient.getDimensionSize(1); ++i) {
        const size_t idx               = i * 4;
        m_colorGradient[i].percent     = colorGradient[idx];
        m_colorGradient[i].color.red   = guint16(colorGradient[idx + 1] * 655.35);
        m_colorGradient[i].color.green = guint16(colorGradient[idx + 2] * 655.35);
        m_colorGradient[i].color.blue  = guint16(colorGradient[idx + 3] * 655.35);
    }

    m_container   = GTK_WIDGET(gtk_builder_get_object(builder, "setting_editor-color_gradient-vbox"));
    m_drawingArea = GTK_WIDGET(gtk_builder_get_object(builder, "setting_editor-color_gradient-drawingarea"));

    g_signal_connect(G_OBJECT(m_dialog), "show", G_CALLBACK(OnInitializeColorGradient), this);
    g_signal_connect(G_OBJECT(m_drawingArea), "expose-event", G_CALLBACK(OnRefreshColorGradient), this);
    g_signal_connect(G_OBJECT(gtk_builder_get_object(builder, "setting_editor-color_gradient-add_button")), "pressed",
                     G_CALLBACK(OnButtonColorGradientAddPressed), this);
    g_signal_connect(G_OBJECT(gtk_builder_get_object(builder, "setting_editor-color_gradient-remove_button")), "pressed",
                     G_CALLBACK(OnButtonColorGradientRemovePressed), this);

    if (gtk_dialog_run(GTK_DIALOG(m_dialog)) == GTK_RESPONSE_APPLY) {
        CString finalGradient;
        CMatrix finalColorGradient(4, m_colorGradient.size());
        size_t idx = 0;
        for (size_t i = 0; i < m_colorGradient.size(); ++i) {
            finalColorGradient[idx++] = m_colorGradient[i].percent;
            finalColorGradient[idx++] = round(m_colorGradient[i].color.red * 100. / 65535.);
            finalColorGradient[idx++] = round(m_colorGradient[i].color.green * 100. / 65535.);
            finalColorGradient[idx++] = round(m_colorGradient[i].color.blue * 100. / 65535.);
        }
        VisualizationToolkit::ColorGradient::format(finalGradient, finalColorGradient);
        if (!m_onValueSetting) { getBox().setSettingValue(GetSettingIndex(), finalGradient.toASCIIString()); }
        SetValue(finalGradient.toASCIIString());
    }

    gtk_widget_destroy(m_dialog);
    g_object_unref(builder);
}


void CColorGradientSettingView::InitializeGradient()
{
    gtk_widget_hide(m_container);

    gtk_container_foreach(GTK_CONTAINER(m_container), OnGtkWidgetDestroy, nullptr);

    size_t i           = 0;
    const size_t count = m_colorGradient.size();
    m_colorButtons.clear();
    m_spinButtons.clear();
    for (auto& cg : m_colorGradient) {
        GtkBuilder* builder = gtk_builder_new();
        gtk_builder_add_from_file(builder, m_builderName.toASCIIString(), nullptr);
        gtk_builder_connect_signals(builder, nullptr);

        GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(builder, "setting_editor-color_gradient-hbox"));

        cg.colorButton = GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "setting_editor-color_gradient-colorbutton"));
        cg.spinButton  = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "setting_editor-color_gradient-spinbutton"));

        gtk_color_button_set_color(cg.colorButton, &cg.color);
        gtk_spin_button_set_value(cg.spinButton, cg.percent);

        g_signal_connect(G_OBJECT(cg.colorButton), "color-set", G_CALLBACK(OnColorGradientColorButtonPressed), this);
        g_signal_connect(G_OBJECT(cg.spinButton), "value-changed", G_CALLBACK(OnColorGradientSpinButtonValueChanged), this);

        g_object_ref(widget);
        gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(widget)), widget);
        gtk_container_add(GTK_CONTAINER(m_container), widget);
        g_object_unref(widget);

        g_object_unref(builder);

        m_colorButtons[cg.colorButton] = i;
        m_spinButtons[cg.spinButton]   = i;
        i++;
    }

    gtk_spin_button_set_value(m_colorGradient[0].spinButton, 0);
    gtk_spin_button_set_value(m_colorGradient[count - 1].spinButton, 100);

    gtk_widget_show(m_container);
}

void CColorGradientSettingView::RefreshColorGradient() const
{
    const size_t steps = 100;
    gint sizex         = 0;
    gint sizey         = 0;
    gdk_drawable_get_size(m_drawingArea->window, &sizex, &sizey);

    CMatrix gradient(4, m_colorGradient.size());
    for (size_t i = 0; i < m_colorGradient.size(); ++i) {
        const size_t idx  = i * 4;
        gradient[idx]     = m_colorGradient[i].percent;
        gradient[idx + 1] = m_colorGradient[i].color.red * 100. / 65535.;
        gradient[idx + 2] = m_colorGradient[i].color.green * 100. / 65535.;
        gradient[idx + 3] = m_colorGradient[i].color.blue * 100. / 65535.;
    }

    CMatrix interpolated;
    VisualizationToolkit::ColorGradient::interpolate(interpolated, gradient, steps);

    GdkGC* gc = gdk_gc_new(m_drawingArea->window);
    GdkColor color;

    for (size_t i = 0; i < steps; ++i) {
        color.red   = guint(interpolated[i * 4 + 1] * 65535 * .01);
        color.green = guint(interpolated[i * 4 + 2] * 65535 * .01);
        color.blue  = guint(interpolated[i * 4 + 3] * 65535 * .01);
        gdk_gc_set_rgb_fg_color(gc, &color);
        gdk_draw_rectangle(m_drawingArea->window, gc, TRUE, gint((sizex * i) / steps), 0, gint((sizex * (i + 1)) / steps), sizey);
    }
    g_object_unref(gc);
}

void CColorGradientSettingView::AddColor()
{
    m_colorGradient.resize(m_colorGradient.size() + 1);
    m_colorGradient[m_colorGradient.size() - 1].percent = 100;
    InitializeGradient();
    RefreshColorGradient();
}

void CColorGradientSettingView::RemoveColor()
{
    if (m_colorGradient.size() > 2) {
        m_colorGradient.resize(m_colorGradient.size() - 1);
        m_colorGradient[m_colorGradient.size() - 1].percent = 100;
        InitializeGradient();
        RefreshColorGradient();
    }
}

void CColorGradientSettingView::SpinChange(GtkSpinButton* button)
{
    gtk_spin_button_update(button);

    const size_t i                = m_spinButtons[button];
    GtkSpinButton* prevSpinButton = i > 0 ? m_colorGradient[i - 1].spinButton : nullptr;
    GtkSpinButton* nextSpinButton = i < m_colorGradient.size() - 1 ? m_colorGradient[i + 1].spinButton : nullptr;
    if (!prevSpinButton) { gtk_spin_button_set_value(button, 0); }
    if (!nextSpinButton) { gtk_spin_button_set_value(button, 100); }
    if (prevSpinButton && gtk_spin_button_get_value(button) < gtk_spin_button_get_value(prevSpinButton)) {
        gtk_spin_button_set_value(button, gtk_spin_button_get_value(prevSpinButton));
    }
    if (nextSpinButton && gtk_spin_button_get_value(button) > gtk_spin_button_get_value(nextSpinButton)) {
        gtk_spin_button_set_value(button, gtk_spin_button_get_value(nextSpinButton));
    }

    m_colorGradient[i].percent = gtk_spin_button_get_value(button);

    RefreshColorGradient();
}

void CColorGradientSettingView::ColorChange(GtkColorButton* button)
{
    GdkColor color;
    gtk_color_button_get_color(button, &color);

    m_colorGradient[m_colorButtons[button]].color = color;

    RefreshColorGradient();
}

void CColorGradientSettingView::OnChange()
{
    if (!m_onValueSetting) {
        const gchar* value = gtk_entry_get_text(m_entry);
        getBox().setSettingValue(GetSettingIndex(), value);
    }
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
