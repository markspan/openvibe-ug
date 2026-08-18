///-------------------------------------------------------------------------------------------------
/// 
/// \file CAboutPluginDialog.cpp
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

#include "CAboutPluginDialog.hpp"

namespace OpenViBE {
namespace Designer {

bool CAboutPluginDialog::Run()
{
    if (m_pods == nullptr) { m_pods = m_kernelCtx.getPluginManager().getPluginObjectDescCreating(m_pluginClassID); }
    if (!m_pods) { return false; }

    GtkBuilder* interface = gtk_builder_new(); // glade_xml_new(m_guiFilename.toASCIIString(), "plugin_about", nullptr);
    gtk_builder_add_from_file(interface, m_guiFilename.toASCIIString(), nullptr);
    gtk_builder_connect_signals(interface, nullptr);

    GtkWidget* dialog            = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about"));
    GtkWidget* type              = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about-entry_type"));
    GtkWidget* name              = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about-entry_name"));
    GtkWidget* authorName        = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about-entry_author_name"));
    GtkWidget* authorCompanyName = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about-entry_company_name"));
    GtkWidget* category          = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about-entry_category"));
    GtkWidget* version           = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about-entry_version"));
    GtkWidget* shortDesc         = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about-textview_short_description"));
    GtkWidget* detailedDesc      = GTK_WIDGET(gtk_builder_get_object(interface, "plugin_about-textview_detailed_description"));
    g_object_unref(interface);

    if (m_pods->isDerivedFromClass(OV_ClassId_Plugins_AlgorithmDesc)) { gtk_entry_set_text(GTK_ENTRY(type), "Algorithm"); }
    else if (m_pods->isDerivedFromClass(OV_ClassId_Plugins_BoxAlgorithmDesc)) { gtk_entry_set_text(GTK_ENTRY(type), "Box algorithm"); }
    else if (m_pods->isDerivedFromClass(OV_ClassId_Plugins_ScenarioImporterDesc)) { gtk_entry_set_text(GTK_ENTRY(type), "Scenario importer"); }
    else if (m_pods->isDerivedFromClass(OV_ClassId_Plugins_ScenarioExporterDesc)) { gtk_entry_set_text(GTK_ENTRY(type), "Scenario exporter"); }

    GtkTextBuffer* shortDescBuffer    = gtk_text_buffer_new(nullptr);
    GtkTextBuffer* detailedDescBuffer = gtk_text_buffer_new(nullptr);
    gtk_text_buffer_set_text(shortDescBuffer, m_pods->getShortDescription(), -1);
    gtk_text_buffer_set_text(detailedDescBuffer, m_pods->getDetailedDescription(), -1);
    gtk_entry_set_text(GTK_ENTRY(name), m_pods->getName());
    gtk_entry_set_text(GTK_ENTRY(authorName), m_pods->getAuthorName());
    gtk_entry_set_text(GTK_ENTRY(authorCompanyName), m_pods->getAuthorCompanyName());
    gtk_entry_set_text(GTK_ENTRY(category), m_pods->getCategory());
    gtk_entry_set_text(GTK_ENTRY(version), m_pods->getVersion());
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(shortDesc), shortDescBuffer);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(detailedDesc), detailedDescBuffer);
    g_object_unref(shortDescBuffer);
    g_object_unref(detailedDescBuffer);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return true;
}

}  // namespace Designer
}  // namespace OpenViBE
