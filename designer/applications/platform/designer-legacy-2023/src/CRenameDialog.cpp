///-------------------------------------------------------------------------------------------------
/// 
/// \file CRenameDialog.cpp
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

#include "CRenameDialog.hpp"

namespace OpenViBE {
namespace Designer {

bool CRenameDialog::Run()
{
    GtkBuilder* interface = gtk_builder_new(); // glade_xml_new(m_guiFilename.toASCIIString(), "rename", nullptr);
    gtk_builder_add_from_file(interface, m_guiFilename.toASCIIString(), nullptr);
    gtk_builder_connect_signals(interface, nullptr);

    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(interface, "rename"));
    GtkWidget* name   = GTK_WIDGET(gtk_builder_get_object(interface, "rename-entry"));
    g_object_unref(interface);

    gtk_entry_set_text(GTK_ENTRY(name), m_initialName.toASCIIString());

    bool finished = false;
    bool res      = false;
    while (!finished) {
        const gint valid = gtk_dialog_run(GTK_DIALOG(dialog));
        if (valid == GTK_RESPONSE_APPLY) {
            m_result = gtk_entry_get_text(GTK_ENTRY(name));
            finished = true;
            res      = true;
        }
        else if (valid == 1) { gtk_entry_set_text(GTK_ENTRY(name), m_defaultName.toASCIIString()); } // default
        else if (valid == 2) { gtk_entry_set_text(GTK_ENTRY(name), m_initialName.toASCIIString()); } // revert
        else {
            finished = true;
            res      = false;
        }
    }

    gtk_widget_destroy(dialog);

    return res;
}

}  // namespace Designer
}  // namespace OpenViBE
