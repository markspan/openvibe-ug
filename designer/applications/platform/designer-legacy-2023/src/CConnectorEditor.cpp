///-------------------------------------------------------------------------------------------------
/// 
/// \file CConnectorEditor.cpp
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

#include "CConnectorEditor.hpp"

#include <map>
#include <string>

namespace OpenViBE {
namespace Designer {

static void ResetScenarioConnectorIdentifier(GtkWidget* /*widget*/, CConnectorEditor* self)
{
    const CIdentifier newID = self->m_Box.getUnusedInputIdentifier(CIdentifier::undefined());
    if (self->m_IDEntry && newID != CIdentifier::undefined()) { gtk_entry_set_text(self->m_IDEntry, newID.str().c_str()); }
}

bool CConnectorEditor::Run()
{
    //get_identifier_t getID;
    set_name_t setName;
    set_type_t setType;
    is_type_supported_t isTypeSupported;
    //update_identifier_t updateID;

    Kernel::EBoxInterfacorType interfacorType;
    switch (m_type) {
        case Box_Input:
            setName = &Kernel::IBox::setInputName;
            setType         = &Kernel::IBox::setInputType;
            isTypeSupported = &Kernel::IBox::hasInputSupport;
            interfacorType  = Kernel::Input;
            break;

        case Box_Output:
            setName = &Kernel::IBox::setOutputName;
            setType         = &Kernel::IBox::setOutputType;
            isTypeSupported = &Kernel::IBox::hasOutputSupport;
            interfacorType  = Kernel::Output;
            break;

        default: return false;
    }

    CString name;
    CIdentifier typeID;
    CIdentifier id;
    m_Box.getInterfacorIdentifier(interfacorType, m_index, id);
    m_Box.getInterfacorName(interfacorType, m_index, name);
    m_Box.getInterfacorType(interfacorType, m_index, typeID);

    GtkBuilder* builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, m_guiFilename.c_str(), nullptr);
    gtk_builder_connect_signals(builder, nullptr);

    GtkWidget* dialog         = GTK_WIDGET(gtk_builder_get_object(builder, "connector_editor"));
    GtkEntry* nameEntry       = GTK_ENTRY(gtk_builder_get_object(builder, "connector_editor-connector_name_entry"));
    GtkComboBox* typeComboBox = GTK_COMBO_BOX(gtk_builder_get_object(builder, "connector_editor-connector_type_combobox"));
    m_IDEntry                 = GTK_ENTRY(gtk_builder_get_object(builder, "connector_editor-connector_identifier_entry"));
    GtkButton* idResetButton  = GTK_BUTTON(gtk_builder_get_object(builder, "connector_editor-connector_identifier_reset_button"));
    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(typeComboBox)));
    gtk_window_set_title(GTK_WINDOW(dialog), m_title.c_str());

    if (m_Box.getAlgorithmClassIdentifier() == CIdentifier::undefined()) {
        gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "connector_editor-connector_identifier_label")));
        gtk_widget_show(GTK_WIDGET(m_IDEntry));
        gtk_widget_show(GTK_WIDGET(idResetButton));
        g_signal_connect(GTK_WIDGET(idResetButton), "clicked", G_CALLBACK(ResetScenarioConnectorIdentifier), this);
    }

    //get a list of stream types and display connector type
    std::map<std::string, CIdentifier> streamTypes;
    gint active = -1;

    for (const auto& currentTypeID : m_kernelCtx.getTypeManager().getSortedTypes()) {
        //First check if the type is support by the connector
        if ((m_Box.*isTypeSupported)(currentTypeID.first)) {
            //If the input type is support by the connector, let's add it to the list
            if (m_kernelCtx.getTypeManager().isStream(currentTypeID.first)) {
                gtk_combo_box_append_text(typeComboBox, currentTypeID.second.toASCIIString());
                if (currentTypeID.first == typeID) {
                    active = gint(streamTypes.size());
                    gtk_combo_box_set_active(typeComboBox, active);
                }
                streamTypes[currentTypeID.second.toASCIIString()] = currentTypeID.first;
            }
        }
    }

    //display connector name
    gtk_entry_set_text(nameEntry, name.toASCIIString());
    gtk_entry_set_text(m_IDEntry, id.str().c_str());

    bool finished = false;
    bool res      = false;
    while (!finished) {
        const gint result = gtk_dialog_run(GTK_DIALOG(dialog));
        if (result == GTK_RESPONSE_APPLY) {
            char* activeText = gtk_combo_box_get_active_text(typeComboBox);
            if (activeText) {
                const auto newName = gtk_entry_get_text(nameEntry);
                auto newType       = streamTypes[activeText];
                const std::string newIdStr(gtk_entry_get_text(m_IDEntry));

                (m_Box.*setType)(m_index, newType);
                (m_Box.*setName)(m_index, newName);

                // If the connector identifier is valid then create a new one and swap it with the edited one
                // this is because we can not change the identifier of a setting
                CIdentifier newID;
                if (newID.fromString(newIdStr) && (newID != id)) { m_Box.updateInterfacorIdentifier(interfacorType, m_index, newID); }
                // (m_Box.*addConnector)(newName, newType);
                finished = true;
                res      = true;
            }
        }
        else if (result == 2) // revert
        {
            m_Box.getInterfacorName(interfacorType, m_index, name);
            m_Box.getInterfacorType(interfacorType, m_index, typeID);

            gtk_entry_set_text(nameEntry, name.toASCIIString());
            gtk_combo_box_set_active(typeComboBox, active);
        }
        else {
            finished = true;
            res      = false;
        }
    }

    gtk_widget_destroy(dialog);
    g_object_unref(builder);

    return res;
}

}  //namespace Designer
}  //namespace OpenViBE
