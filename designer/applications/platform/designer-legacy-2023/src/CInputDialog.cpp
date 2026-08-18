///-------------------------------------------------------------------------------------------------
/// 
/// \file CInputDialog.cpp
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

#include "CInputDialog.hpp"
#include <gdk/gdkkeysyms.h>

namespace OpenViBE {
namespace Designer {

CInputDialog::CInputDialog(const char* gtkBuilder, const fpButtonCB okButtonCB, void* data, const char* title, const char* label, const char* entry)
    : m_userData(data), m_okButtonCB(okButtonCB)
{
    //retrieve input dialog
    GtkBuilder* builder = gtk_builder_new(); // glade_xml_new(gtkBuilder, "input", nullptr);
    gtk_builder_add_from_file(builder, gtkBuilder, nullptr);
    gtk_builder_connect_signals(builder, nullptr);

    m_dialog             = GTK_DIALOG(gtk_builder_get_object(builder, "input"));
    m_dialogLabel        = GTK_LABEL(gtk_builder_get_object(builder, "input-label"));
    m_dialogEntry        = GTK_ENTRY(gtk_builder_get_object(builder, "input-entry"));
    m_dialogOkButton     = GTK_BUTTON(gtk_builder_get_object(builder, "input-button_ok"));
    m_dialogCancelButton = GTK_BUTTON(gtk_builder_get_object(builder, "input-button_cancel"));

    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(m_dialogEntry), GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(m_dialogEntry), "key-press-event", G_CALLBACK(keyPressEventCB), m_dialog);

    if (label != nullptr) { gtk_label_set(m_dialogLabel, label); }
    if (entry != nullptr) { gtk_entry_set_text(m_dialogEntry, entry); }

    g_signal_connect(G_OBJECT(m_dialogOkButton), "clicked", G_CALLBACK(buttonClickedCB), this);
    g_signal_connect(G_OBJECT(m_dialogCancelButton), "clicked", G_CALLBACK(buttonClickedCB), this);

    gtk_window_set_position(GTK_WINDOW(m_dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_title(GTK_WINDOW(m_dialog), title);
}

CInputDialog::~CInputDialog() { gtk_widget_destroy(GTK_WIDGET(m_dialog)); }

void CInputDialog::Run()
{
    const gint res = gtk_dialog_run(m_dialog);
    if (res == GTK_RESPONSE_ACCEPT) { if (m_okButtonCB != nullptr) { m_okButtonCB(GTK_WIDGET(m_dialogOkButton), this); } }
    gtk_widget_hide_all(GTK_WIDGET(m_dialog));
}

gboolean CInputDialog::keyPressEventCB(GtkWidget* /*widget*/, GdkEventKey* eventKey, gpointer data)
{
    if (eventKey->keyval == GDK_Return || eventKey->keyval == GDK_KP_Enter) {
        gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_ACCEPT);
        return TRUE;
    }
    if (eventKey->keyval == GDK_Escape) {
        gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_REJECT);
        return TRUE;
    }

    return FALSE;
}

void CInputDialog::buttonClickedCB(GtkButton* button, gpointer data) { static_cast<CInputDialog*>(data)->buttonClicked(button); }

void CInputDialog::buttonClicked(GtkButton* button) const
{
    if (button == m_dialogOkButton) { gtk_dialog_response(m_dialog, GTK_RESPONSE_ACCEPT); }
    else { gtk_dialog_response(m_dialog, GTK_RESPONSE_REJECT); }
}

}  // namespace Designer
}  // namespace OpenViBE
