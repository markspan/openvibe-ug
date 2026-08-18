///-------------------------------------------------------------------------------------------------
/// 
/// \file CAboutScenarioDialog.cpp
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

#include "CAboutScenarioDialog.hpp"

namespace OpenViBE {
namespace Designer {

static void ButtonMetaboxResetClicked(GtkWidget* /*widget*/, gpointer data) { gtk_entry_set_text(GTK_ENTRY(data), CIdentifier::random().str().c_str()); }

bool CAboutScenarioDialog::Run() const

{
    GtkBuilder* interface = gtk_builder_new(); // glade_xml_new(m_guiFilename.toASCIIString(), "scenario_about", nullptr);
    gtk_builder_add_from_file(interface, m_guiFilename.toASCIIString(), nullptr);
    gtk_builder_connect_signals(interface, nullptr);

    GtkWidget* dialog            = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about"));
    GtkWidget* name              = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-entry_name"));
    GtkWidget* authorName        = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-entry_author_name"));
    GtkWidget* authorCompanyName = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-entry_company_name"));
    GtkWidget* category          = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-entry_category"));
    GtkWidget* version           = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-entry_version"));
    GtkWidget* documentationPage = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-entry_documentation_page"));

    GtkWidget* metaboxId = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-entry_metabox_id"));

    GtkWidget* resetMetaboxId    = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-button_reset_metabox_id"));
    const gulong signalHandlerId = g_signal_connect(G_OBJECT(resetMetaboxId), "clicked", G_CALLBACK(ButtonMetaboxResetClicked), metaboxId);

    GtkWidget* shortDesc    = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-textview_short_description"));
    GtkWidget* detailedDesc = GTK_WIDGET(gtk_builder_get_object(interface, "scenario_about-textview_detailed_description"));

    g_object_unref(interface);

    gtk_entry_set_text(GTK_ENTRY(name), m_scenario.getAttributeValue(OV_AttributeId_Scenario_Name).toASCIIString());
    gtk_entry_set_text(GTK_ENTRY(authorName), m_scenario.getAttributeValue(OV_AttributeId_Scenario_Author).toASCIIString());
    gtk_entry_set_text(GTK_ENTRY(authorCompanyName), m_scenario.getAttributeValue(OV_AttributeId_Scenario_Company).toASCIIString());
    gtk_entry_set_text(GTK_ENTRY(category), m_scenario.getAttributeValue(OV_AttributeId_Scenario_Category).toASCIIString());
    gtk_entry_set_text(GTK_ENTRY(version), m_scenario.getAttributeValue(OV_AttributeId_Scenario_Version).toASCIIString());
    gtk_entry_set_text(GTK_ENTRY(documentationPage), m_scenario.getAttributeValue(OV_AttributeId_Scenario_DocumentationPage).toASCIIString());

    if (m_scenario.isMetabox()) { gtk_entry_set_text(GTK_ENTRY(metaboxId), m_scenario.getAttributeValue(OVP_AttributeId_Metabox_ID).toASCIIString()); }
    else {
        gtk_widget_set_sensitive(metaboxId, FALSE);
        gtk_widget_set_sensitive(resetMetaboxId, FALSE);
    }

    GtkTextBuffer* shortDescBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(shortDesc));
    gtk_text_buffer_set_text(shortDescBuffer, m_scenario.getAttributeValue(OV_AttributeId_Scenario_ShortDescription).toASCIIString(), -1);
    GtkTextBuffer* detailedDescBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(detailedDesc));
    gtk_text_buffer_set_text(detailedDescBuffer, m_scenario.getAttributeValue(OV_AttributeId_Scenario_DetailedDescription).toASCIIString(), -1);

    gtk_dialog_run(GTK_DIALOG(dialog));

    g_signal_handler_disconnect(G_OBJECT(resetMetaboxId), signalHandlerId);
    m_scenario.setAttributeValue(OV_AttributeId_Scenario_Name, gtk_entry_get_text(GTK_ENTRY(name)));
    m_scenario.setAttributeValue(OV_AttributeId_Scenario_Author, gtk_entry_get_text(GTK_ENTRY(authorName)));
    m_scenario.setAttributeValue(OV_AttributeId_Scenario_Company, gtk_entry_get_text(GTK_ENTRY(authorCompanyName)));
    m_scenario.setAttributeValue(OV_AttributeId_Scenario_Category, gtk_entry_get_text(GTK_ENTRY(category)));
    m_scenario.setAttributeValue(OV_AttributeId_Scenario_Version, gtk_entry_get_text(GTK_ENTRY(version)));
    m_scenario.setAttributeValue(OV_AttributeId_Scenario_DocumentationPage, gtk_entry_get_text(GTK_ENTRY(documentationPage)));

    if (m_scenario.isMetabox()) {
        const std::string id(gtk_entry_get_text(GTK_ENTRY(metaboxId)));
        CIdentifier tmp;
        if (!tmp.fromString(id)) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Invalid identifier " << id
                    << " is not in the \"(0x[0-9a-f]{1-8}, 0x[0-9a-f]{1-8})\" format. ";
            m_kernelCtx.getLogManager() << "Reverting to " << m_scenario.getAttributeValue(OVP_AttributeId_Metabox_ID).toASCIIString() << ".\n";
        }
        else { m_scenario.setAttributeValue(OVP_AttributeId_Metabox_ID, id.c_str()); }
    }

    GtkTextIter start, end;

    gtk_text_buffer_get_start_iter(shortDescBuffer, &start);
    gtk_text_buffer_get_end_iter(shortDescBuffer, &end);
    m_scenario.setAttributeValue(OV_AttributeId_Scenario_ShortDescription,
                                 gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(shortDesc)), &start, &end, FALSE));

    gtk_text_buffer_get_start_iter(detailedDescBuffer, &start);
    gtk_text_buffer_get_end_iter(detailedDescBuffer, &end);
    m_scenario.setAttributeValue(OV_AttributeId_Scenario_DetailedDescription,
                                 gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(detailedDesc)), &start, &end, FALSE));

    gtk_widget_destroy(dialog);

    return true;
}

}  // namespace Designer
}  // namespace OpenViBE
