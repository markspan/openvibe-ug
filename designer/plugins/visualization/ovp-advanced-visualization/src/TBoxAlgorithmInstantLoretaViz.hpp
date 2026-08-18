///-------------------------------------------------------------------------------------------------
/// 
/// \file TBoxAlgorithmInstantLoretaViz.hpp
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

#pragma once

#include "TBoxAlgorithmInstantViz.hpp"
#include <array>

namespace OpenViBE {
namespace AdvancedVisualization {
template <class TRendererFactoryClass, class TRulerClass>
class TBoxAlgorithmInstantLoretaViz final : public TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>
{
public:
    TBoxAlgorithmInstantLoretaViz(const CIdentifier& classID, const std::vector<int>& parameters)
        : TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>(classID, parameters) { }

    static void Callback(GtkTreeSelection* selection, TBoxAlgorithmInstantLoretaViz<TRendererFactoryClass, TRulerClass>* box)
    {
        box->RegionSelectionChanged(selection);
    }

    void RegionSelectionChanged(GtkTreeSelection* treeSelection) const
    {
        m_Renderer->ClearRegionSelection();

        GtkTreeIter iter;

        char* value = nullptr;
        for (size_t i = 0; i < 3; ++i) {
            GtkTreeSelection* selection = gtk_tree_view_get_selection(m_LookupTreeView[i]);

            if (selection == treeSelection) {
                if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m_LookupListStore[i]), &iter)) {
                    do {
                        if (gtk_tree_selection_iter_is_selected(selection, &iter)) {
                            gtk_tree_model_get(GTK_TREE_MODEL(m_LookupListStore[i]), &iter, 0, &value, -1);
                            m_Renderer->SelectRegion(i, value);
                        }
                    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(m_LookupListStore[i]), &iter));
                }
            }
            else { gtk_tree_selection_unselect_all(selection); }
        }
    }

    void FillRegion(GtkListStore* listStore, GtkTreeView* view, IRenderer* renderer, const size_t category) const
    {
        GtkTreeIter gtkTreeIter;

        gtk_list_store_clear(listStore);
        gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view), GTK_SELECTION_MULTIPLE);
        for (size_t i = 0; i < renderer->GetRegionCount(category); ++i) {
            const char* name = m_Renderer->GetRegionName(category, i);
            gtk_list_store_append((listStore), &gtkTreeIter);
            gtk_list_store_set(listStore, &gtkTreeIter, 0, name ? name : "", -1);
            gtk_tree_selection_select_iter(gtk_tree_view_get_selection(view), &gtkTreeIter);
        }
    }

    using TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>::m_Renderers;
    using TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>::m_Builder;

    bool initialize() override

    {
        TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>::initialize();

        m_Renderer = m_Renderers[0];

        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(m_Builder, "expander_select")));
        gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(m_Builder, "expander_select_sLORETA")));

        m_LookupTreeView[0]  = GTK_TREE_VIEW(gtk_builder_get_object(m_Builder, "expander_select_broadmann_treeview"));
        m_LookupTreeView[1]  = GTK_TREE_VIEW(gtk_builder_get_object(m_Builder, "expander_select_neuro_1_treeview"));
        m_LookupTreeView[2]  = GTK_TREE_VIEW(gtk_builder_get_object(m_Builder, "expander_select_neuro_2_treeview"));
        m_LookupListStore[0] = GTK_LIST_STORE(gtk_builder_get_object(m_Builder, "liststore_select_broadmann"));
        m_LookupListStore[1] = GTK_LIST_STORE(gtk_builder_get_object(m_Builder, "liststore_select_neuro_1"));
        m_LookupListStore[2] = GTK_LIST_STORE(gtk_builder_get_object(m_Builder, "liststore_select_neuro_2"));

        for (size_t i = 0; i < m_Renderer->GetRegionCategoryCount() && i < 3; ++i) {
            this->FillRegion(m_LookupListStore[i], m_LookupTreeView[i], m_Renderer, i);
        }

        g_signal_connect(gtk_tree_view_get_selection(m_LookupTreeView[0]), "changed", G_CALLBACK(Callback), this);
        g_signal_connect(gtk_tree_view_get_selection(m_LookupTreeView[1]), "changed", G_CALLBACK(Callback), this);
        g_signal_connect(gtk_tree_view_get_selection(m_LookupTreeView[2]), "changed", G_CALLBACK(Callback), this);

        return true;
    }

    IRenderer* m_Renderer = nullptr;
    std::array<GtkTreeView*, 3> m_LookupTreeView;
    std::array<GtkListStore*, 3> m_LookupListStore;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
