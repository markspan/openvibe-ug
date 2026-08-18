///-------------------------------------------------------------------------------------------------
/// 
/// \file CEnumerationSettingView.cpp
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

#include "CEnumerationSettingView.hpp"
#include "../base.hpp"

#include <algorithm> // std::sort
#include <map>

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void OnChangeCB(GtkEntry* /*entry*/, gpointer data) { static_cast<CEnumerationSettingView*>(data)->OnChange(); }

CEnumerationSettingView::CEnumerationSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx,
                                                 const CIdentifier& typeID)
    : CAbstractSettingView(box, index, builderName, "settings_collection-comboboxentry_setting_enumeration"), m_typeID(typeID), m_kernelCtx(ctx)
{
    GtkWidget* setting = CAbstractSettingView::getEntryFieldWidget();

    m_comboBox = GTK_COMBO_BOX(setting);

    std::vector<std::string> entries;

    for (size_t i = 0; i < m_kernelCtx.getTypeManager().getEnumerationEntryCount(m_typeID); ++i) {
        CString name;
        uint64_t value;
        if (m_kernelCtx.getTypeManager().getEnumerationEntry(m_typeID, i, name, value)) { entries.push_back(name.toASCIIString()); }
    }

    std::sort(entries.begin(), entries.end());

    GtkTreeIter it;
    GtkListStore* list = GTK_LIST_STORE(gtk_combo_box_get_model(m_comboBox));
    gtk_combo_box_set_wrap_width(m_comboBox, 0);
    gtk_list_store_clear(list);

    for (size_t i = 0; i < entries.size(); ++i) {
        gtk_list_store_append(list, &it);
        gtk_list_store_set(list, &it, 0, entries[i].c_str(), -1);

        m_entriesIdx[CString(entries[i].c_str())] = uint64_t(i);
    }

    CString value;
    box.getSettingValue(index, value);
    if (m_entriesIdx.count(value.toASCIIString()) == 0) {
        gtk_list_store_append(list, &it);
        gtk_list_store_set(list, &it, 0, value.toASCIIString(), -1);
    }

    CAbstractSettingView::initializeValue();

    g_signal_connect(G_OBJECT(m_comboBox), "changed", G_CALLBACK(OnChangeCB), this);
}


void CEnumerationSettingView::GetValue(CString& value) const { value = CString(gtk_combo_box_get_active_text(m_comboBox)); }


void CEnumerationSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;

    // If the current value of the setting is not in the enumeration list, we will add or replace the last value in the list, so it can be set to this value
    if (m_entriesIdx.count(value) == 0) {
        GtkTreeIter it;
        GtkListStore* list = GTK_LIST_STORE(gtk_combo_box_get_model(m_comboBox));
        int valuesInModel  = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list), nullptr);
        if (valuesInModel == int(m_entriesIdx.size())) {
            gtk_list_store_append(list, &it);
            valuesInModel += 1;
        }
        else {
            // We just set the iterator at the end
            GtkTreePath* treePath = gtk_tree_path_new_from_indices(valuesInModel - 1, -1);
            gtk_tree_model_get_iter(GTK_TREE_MODEL(list), &it, treePath);
            gtk_tree_path_free(treePath);
        }
        gtk_list_store_set(list, &it, 0, value.toASCIIString(), -1);
        gtk_combo_box_set_active(m_comboBox, valuesInModel - 1);
    }
    else { gtk_combo_box_set_active(m_comboBox, gint(m_entriesIdx[value])); }
    m_onValueSetting = false;
}

void CEnumerationSettingView::OnChange()
{
    if (!m_onValueSetting) {
        gchar* value = gtk_combo_box_get_active_text(m_comboBox);
        getBox().setSettingValue(GetSettingIndex(), value);
        g_free(value);
    }
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
