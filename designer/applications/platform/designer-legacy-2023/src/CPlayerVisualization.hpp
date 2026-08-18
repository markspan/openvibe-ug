///-------------------------------------------------------------------------------------------------
/// 
/// \file CPlayerVisualization.hpp
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

#include "base.hpp"

#include <string>
#include <vector>
#include <map>

namespace OpenViBE {
namespace Designer {
class CInterfacedScenario;

class CPlayerVisualization final : public VisualizationToolkit::ITreeViewCB
{
public:
    CPlayerVisualization(const Kernel::IKernelContext& ctx, VisualizationToolkit::IVisualizationTree& tree, CInterfacedScenario& interfacedScenario)
        : m_kernelCtx(ctx), m_visualizationTree(tree), m_interfacedScenario(interfacedScenario) { }

    ~CPlayerVisualization() override;

    void Init();

    /** \name ITreeViewCB interface implementation */
    //@{
    GtkWidget* LoadTreeWidget(VisualizationToolkit::IVisualizationWidget* widget) override;
    void EndLoadTreeWidget(VisualizationToolkit::IVisualizationWidget* widget) override;
    bool SetToolbar(const CIdentifier& boxID, GtkWidget* widget) override;
    bool SetWidget(const CIdentifier& boxID, GtkWidget* widget) override;
    //@}

    void ShowTopLevelWindows();
    void HideTopLevelWindows();
    CInterfacedScenario& GetInterfacedScenario() const { return m_interfacedScenario; }

protected:
    bool parentWidgetBox(VisualizationToolkit::IVisualizationWidget* widget, GtkBox* widgetBox);

    static gboolean configureEventCB(GtkWidget* widget, GdkEventConfigure* event, gpointer data);
    static gboolean widgetExposeEventCB(GtkWidget* widget, GdkEventExpose* event, gpointer data);
    void resizeCB(GtkContainer* container);

    //callbacks for DND
    static void dragDataGetFromWidgetCB(GtkWidget* srcWidget, GdkDragContext* dc, GtkSelectionData* selectionData, guint info, guint time, gpointer data);
    static void dragDataReceivedInWidgetCB(GtkWidget* dstWidget, GdkDragContext* dc, gint x, gint y, GtkSelectionData* selectionData, guint info, guint time,
                                           gpointer data);

    //callback for toolbar
    static void toolbarButtonToggledCB(GtkToggleButton* button, gpointer data);
    bool toggleToolbarCB(GtkToggleButton* button);
    static gboolean toolbarDeleteEventCB(GtkWidget* widget, GdkEvent* event, gpointer data);
    bool deleteToolbarCB(GtkWidget* widget);

private:
    const Kernel::IKernelContext& m_kernelCtx;
    VisualizationToolkit::IVisualizationTree& m_visualizationTree;
    CInterfacedScenario& m_interfacedScenario;

    /**
     * \brief Vector of top level windows
     */
    std::vector<GtkWindow*> m_windows;

    /**
     * \brief Map of split (paned) widgets associated to their identifiers
     * This map is used to retrieve size properties of split widgets upon window resizing,
     * so as to keep the relative sizes of a hierarchy of widgets
     */
    std::map<GtkPaned*, CIdentifier> m_splitWidgets;

    /**
     * \brief Map associating toolbar buttons to toolbar windows
     */
    std::map<GtkToggleButton*, GtkWidget*> m_toolbars;

    /**
     * \brief Pointer to active toolbar button
     */
    GtkToggleButton* m_activeToolbarButton = nullptr;

    class CPluginWidgets
    {
    public:
        CPluginWidgets()  = default;
        ~CPluginWidgets() = default;
        GtkWidget* m_Widget              = nullptr;
        GtkToggleButton* m_ToolbarButton = nullptr;
        GtkWidget* m_Toolbar             = nullptr;
    };

    /**
     * \brief Map of visualization plugins
     */
    std::map<CIdentifier, CPluginWidgets> m_plugins;
};
}  // namespace Designer
}  // namespace OpenViBE
