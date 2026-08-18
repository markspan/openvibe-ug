///-------------------------------------------------------------------------------------------------
/// 
/// \file CAbstractSettingView.cpp
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

#include "CAbstractSettingView.hpp"

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void CollectWidgetCB(GtkWidget* widget, gpointer data) { static_cast<std::vector<GtkWidget*>*>(data)->push_back(widget); }

CAbstractSettingView::~CAbstractSettingView()
{
    if (GTK_IS_WIDGET(m_nameWidget)) { gtk_widget_destroy(m_nameWidget); }
    if (GTK_IS_WIDGET(m_entryNameWidget)) { gtk_widget_destroy(m_entryNameWidget); }
    if (G_IS_OBJECT(m_builder)) { g_object_unref(m_builder); }
}

CAbstractSettingView::CAbstractSettingView(Kernel::IBox& box, const size_t index, const char* builderName, const char* widgetName)
    : m_box(box), m_index(index), m_settingWidgetName("")
{
    if (builderName != nullptr) {
        m_builder = gtk_builder_new();
        gtk_builder_add_from_file(m_builder, builderName, nullptr);
        gtk_builder_connect_signals(m_builder, nullptr);

        if (widgetName != nullptr) {
            m_settingWidgetName = widgetName;
            CAbstractSettingView::generateNameWidget();
            m_entryFieldWidget = CAbstractSettingView::generateEntryWidget();
        }
    }
}

void CAbstractSettingView::setNameWidget(GtkWidget* widget)
{
    if (m_nameWidget) { gtk_widget_destroy(m_nameWidget); }
    m_nameWidget = widget;
}

void CAbstractSettingView::setEntryWidget(GtkWidget* widget)
{
    if (m_entryNameWidget) { gtk_widget_destroy(m_entryNameWidget); }
    m_entryNameWidget = widget;
}

void CAbstractSettingView::generateNameWidget()
{
    GtkWidget* settingName = GTK_WIDGET(gtk_builder_get_object(m_builder, "settings_collection-label_setting_name"));
    gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(settingName)), settingName);
    setNameWidget(settingName);

    CString name;
    getBox().getSettingName(m_index, name);
    gtk_label_set_text(GTK_LABEL(settingName), name);
}

GtkWidget* CAbstractSettingView::generateEntryWidget()
{
    GtkTable* table = GTK_TABLE(gtk_table_new(1, 3, false));

    GtkWidget* settingWidget  = GTK_WIDGET(gtk_builder_get_object(m_builder, m_settingWidgetName.toASCIIString()));
    GtkWidget* settingRevert  = GTK_WIDGET(gtk_builder_get_object(m_builder, "settings_collection-button_setting_revert"));
    GtkWidget* settingDefault = GTK_WIDGET(gtk_builder_get_object(m_builder, "settings_collection-button_setting_default"));

    gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(settingWidget)), settingWidget);
    gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(settingRevert)), settingRevert);
    gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(settingDefault)), settingDefault);

    gtk_table_attach(table, settingWidget, 0, 1, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
    //gtk_table_attach(m_table, settingDefault, 1, 2, 0, 1, GtkAttachOptions(GTK_SHRINK), GtkAttachOptions(GTK_SHRINK), 0, 0);
    //gtk_table_attach(m_table, settingRevert, 2, 3, 0, 1, GtkAttachOptions(GTK_SHRINK), GtkAttachOptions(GTK_SHRINK), 0, 0);

    setEntryWidget(GTK_WIDGET(table));
    gtk_widget_set_visible(GetEntryWidget(), true);
    //If we don't increase the ref counter it will cause trouble when we gonna move it later
    g_object_ref(G_OBJECT(table));
    return settingWidget;
}

void CAbstractSettingView::initializeValue()
{
    CString value;
    getBox().getSettingValue(m_index, value);
    SetValue(value);
}

void CAbstractSettingView::extractWidget(GtkWidget* widget, std::vector<GtkWidget*>& widgets)
{
    gtk_container_foreach(GTK_CONTAINER(widget), CollectWidgetCB, &widgets);
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
