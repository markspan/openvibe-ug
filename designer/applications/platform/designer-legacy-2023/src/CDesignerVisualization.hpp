///-------------------------------------------------------------------------------------------------
/// 
/// \file CDesignerVisualization.hpp
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

#include <string>
#include <vector>

#include <gtk/gtk.h>
#include <visualization-toolkit/ovvizIVisualizationTree.h>

namespace OpenViBE {
namespace Designer {
typedef void (*visualization_delete_event_cb_t)(gpointer data);

class CInterfacedScenario;

class CDesignerVisualization final : public VisualizationToolkit::ITreeViewCB
{
public:
    CDesignerVisualization(const Kernel::IKernelContext& ctx, VisualizationToolkit::IVisualizationTree& tree, CInterfacedScenario& scenario)
        : m_kernelCtx(ctx), m_tree(tree), m_scenario(scenario) { }

    ~CDesignerVisualization() override;

    void Init(const std::string& guiFile);
    void Load();
    void Show() const;
    void Hide();

    void SetDeleteEventCB(visualization_delete_event_cb_t cb, gpointer data);

    void OnVisualizationBoxAdded(const Kernel::IBox* box);
    void OnVisualizationBoxRemoved(const CIdentifier& boxID);
    void OnVisualizationBoxRenamed(const CIdentifier& boxID);

    //ITreeViewCB callbacks overloading
    void CreateTreeWidget(VisualizationToolkit::IVisualizationWidget* widget) override;
    GtkWidget* LoadTreeWidget(VisualizationToolkit::IVisualizationWidget* widget) override;
    void EndLoadTreeWidget(VisualizationToolkit::IVisualizationWidget* widget) override;
    GtkWidget* GetTreeWidget(GtkWidget* widget) override;
    GtkWidget* GetVisualizationWidget(GtkWidget* widget) override;
    const char* GetTreeWidgetIcon(VisualizationToolkit::EVisualizationTreeNode type) override;

    //callbacks for dialog
#ifdef HANDLE_MIN_MAX_EVENTS
        static gboolean window_state_event_cb(GtkWidget* widget, GdkEventWindowState* event, gpointer data);
#endif
    static gboolean ConfigureEventCB(GtkWidget* widget, GdkEventConfigure* event, gpointer data);
    static gboolean WidgetExposeEventCB(GtkWidget* widget, GdkEventExpose* event, gpointer data);
    void ResizeCB(VisualizationToolkit::IVisualizationWidget* widget);

    static void NotebookPageSwitchCB(GtkNotebook* notebook, GtkNotebookPage* page, guint pagenum, gpointer data);

    //callback for paned handle position changes
    static gboolean NotifyPositionPanedCB(GtkWidget* widget, GParamSpec* spec, gpointer data);

    static void AskNewVisualizationWindowCB(gpointer data, guint action, GtkWidget* widget);
    static void NewVisualizationWindowCB(GtkWidget* widget, gpointer data);
    static void AskRenameVisualizationWindowCB(gpointer data, guint action, GtkWidget* widget);
    static void RenameVisualizationWindowCB(GtkWidget* widget, gpointer data);
    static void RemoveVisualizationWindowCB(gpointer data, guint action, GtkWidget* widget);

    static void AskNewVisualizationPanelCB(gpointer data, guint action, GtkWidget* widget);
    static void NewVisualizationPanelCB(GtkWidget* widget, gpointer data);
    static void AskRenameVisualizationPanelCB(gpointer data, guint action, GtkWidget* widget);
    static void RenameVisualizationPanelCB(GtkWidget* widget, gpointer data);
    static void RemoveVisualizationPanelCB(gpointer data, guint action, GtkWidget* widget);

    static void RemoveVisualizationWidgetCB(gpointer data, guint action, GtkWidget* widget);

private:
    static gboolean deleteEventCB(GtkWidget* widget, GdkEvent* event, gpointer data);
    bool deleteEvent() const;

    void refreshActiveVisualization(GtkTreePath* selectedItemPath);

    void setActiveVisualization(const char* activeWindow, const char* activePanel);

    GtkTable* newWidgetsTable();
    void setupNewEventBoxTable(GtkBuilder* xml);

    //visualization windows
    void askNewVisualizationWindow();
public:
    bool NewVisualizationWindow(const char* label);
private:
    void askRenameVisualizationWindow();
    bool renameVisualizationWindow(const char* label);
    bool removeVisualizationWindow();

    //visualization panels
    void askNewVisualizationPanel();
    bool newVisualizationPanel(const char* label);
    void askRenameVisualizationPanel();
    bool renameVisualizationPanel(const char* label);
    bool removeVisualizationPanel();

    //visualization widgets
    bool removeVisualizationWidget();
    bool removeVisualizationWidget(const CIdentifier& identifier);
    bool destroyVisualizationWidget(const CIdentifier& identifier);

    void enableNotebookSignals(GtkWidget* notebook, bool b);
    void notebookPageSelectedCB(GtkNotebook* notebook, guint pagenum);

    void enablePanedSignals(GtkWidget* paned, bool b);
    void notifyPositionPaned(GtkWidget* widget);

    //Mouse/Key event callbacks
    static void widgetKeyPressEventCB(GtkWidget* widget, GdkEventKey* event, gpointer data);
    static gboolean widgetEnterNotifyEventCB(GtkWidget* widget, GdkEventCrossing* event, gpointer data);
    static gboolean widgetLeaveNotifyEventCB(GtkWidget* widget, GdkEventCrossing* event, gpointer data);

    //Mouse/Key event methods
    void widgetKeyPressEvent(GtkWidget* widget, GdkEventKey* event);
    void widgetEnterNotifyEvent(GtkWidget* widget, GdkEventCrossing* event);
    void widgetLeaveNotifyEvent(GtkWidget* widget, GdkEventCrossing* event);

    //Tree management callbacks
    static gboolean buttonReleaseCB(GtkWidget* widget, GdkEventButton* event, gpointer data);
    static void cursorChangedCB(GtkTreeView* treeView, gpointer data);

    //Tree management methods
    void buttonRelease(GtkWidget* widget, GdkEventButton* event) const;
    void cursorChanged(GtkTreeView* treeView);

    //Drag methods
    static void dragDataGetFromTreeCB(GtkWidget* srcWidget, GdkDragContext* dc, GtkSelectionData* selection, guint info, guint time, gpointer data);
    static void dragDataGetFromWidgetCB(GtkWidget* srcWidget, GdkDragContext* dc, GtkSelectionData* selection, guint info, guint time, gpointer data);

    //Drop methods
    static void dragDataReceivedInWidgetCB(GtkWidget* dstWidget, GdkDragContext* dc, gint x, gint y, GtkSelectionData* selection, guint info, guint time,
                                           gpointer data);
    static void dataReceivedInEventBoxCB(GtkWidget* dstWidget, GdkDragContext* dc, gint x, gint y, GtkSelectionData* selection, guint info, guint time,
                                         gpointer data);
    void dragDataReceivedInWidget(GtkWidget* dstWidget, GtkSelectionData* selection);
    void dragDataReceivedInEventBox(GtkWidget* dstWidget, GtkSelectionData* selection, VisualizationToolkit::EDragLocation location);

    const Kernel::IKernelContext& m_kernelCtx;
    VisualizationToolkit::IVisualizationTree& m_tree;
    CInterfacedScenario& m_scenario;
    visualization_delete_event_cb_t m_deleteEventCB = nullptr;
    gpointer m_deleteEventUserData                  = nullptr;
    std::string m_guiFile;
    GtkTreeView* m_treeView = nullptr;
    GtkWidget* m_dialog     = nullptr;
    GtkWidget* m_pane       = nullptr;

    //highlighted widget
    GtkWidget* m_highlightedWidget = nullptr;

    //active items
    CString m_activeVisualizationWindowName, m_activeVisualizationPanelName;
    CIdentifier m_activeVisualizationBoxID = CIdentifier::undefined();

    //preview window visibility flag
    bool m_previewWindowVisible = false;
    size_t m_previewWindowW     = 0;
    size_t m_previewWindowH     = 0;

    //Factories
    GtkItemFactory* m_unaffectedItemFactory          = nullptr;
    GtkItemFactory* m_visualizationWindowItemFactory = nullptr;
    GtkItemFactory* m_visualizationPanelItemFactory  = nullptr;
    GtkItemFactory* m_visualizationBoxItemFactory    = nullptr;
    GtkItemFactory* m_undefinedItemFactory           = nullptr;
    GtkItemFactory* m_splitItemFactory               = nullptr;

    std::string m_topEventBoxData;
    std::string m_leftEventBoxData;
    std::string m_rightEventBoxData;
    std::string m_bottomEventBoxData;
};
}  // namespace Designer
}  // namespace OpenViBE
