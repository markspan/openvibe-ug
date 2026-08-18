///-------------------------------------------------------------------------------------------------
/// 
/// \file CPlayerVisualization.cpp
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

#include "base.hpp"
#include "CApplication.hpp"
#include "CInterfacedScenario.hpp"
#include "CPlayerVisualization.hpp"

#include <cstring>
#include <array>

namespace OpenViBE {
namespace Designer {

using EVTColumn = VisualizationToolkit::EVisualizationTreeColumn;
using EVTNode = VisualizationToolkit::EVisualizationTreeNode;
using EVWidget = VisualizationToolkit::EVisualizationWidget;
using IVWidget = VisualizationToolkit::IVisualizationWidget;

static std::array<GtkTargetEntry, 2> targets = { { { const_cast<gchar*>("STRING"), 0, 0 }, { const_cast<gchar*>("text/plain"), 0, 0 } } };

static void DeleteWindowManagerWindowCB(GtkWidget* widget, GdkEvent* /*event*/, gpointer data)
{
    CPlayerVisualization* visualization = reinterpret_cast<CPlayerVisualization*>(data);
    const CInterfacedScenario& scenario = visualization->GetInterfacedScenario();
    GtkWidget* dialog                   = gtk_message_dialog_new(GTK_WINDOW(widget), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                                                 "Would you like to stop the scenario execution?");
    const gint res = gtk_dialog_run(GTK_DIALOG(dialog));

    if (res == GTK_RESPONSE_YES) { if (visualization != nullptr) { scenario.m_Player->stop(); } }
    else if (res == GTK_RESPONSE_NO) {}

    gtk_widget_destroy(dialog);
}

CPlayerVisualization::~CPlayerVisualization()
{
    //HideTopLevelWindows();

    m_activeToolbarButton = nullptr;

    for (const auto& window : m_windows) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(window), G_CALLBACK2(CPlayerVisualization::configureEventCB), this);
        gtk_widget_destroy(GTK_WIDGET(window));
    }

    m_visualizationTree.setTreeViewCB(nullptr);
}

void CPlayerVisualization::Init()
{
    m_windows.clear();      // empty windows vector
    m_splitWidgets.clear(); // empty split widgets map
    m_toolbars.clear();     // empty toolbars map
    m_plugins.clear();      // empty plugin widgets map
    m_activeToolbarButton = nullptr;
    m_visualizationTree.setTreeViewCB(this);                // register towards tree store
    m_visualizationTree.reloadTree();                       // rebuild widgets
    m_interfacedScenario.SetModifiableSettingsWidgets();    // must be called after the previous call to reload tree
}

GtkWidget* CPlayerVisualization::LoadTreeWidget(IVWidget* widget)
{
    GtkWidget* treeWidget = nullptr;

    if (widget->getType() == EVWidget::Panel) {
        //retrieve panel index
        IVWidget* window = m_visualizationTree.getVisualizationWidget(widget->getParentIdentifier());
        if (window != nullptr) {
            size_t index;
            window->getChildIndex(widget->getIdentifier(), index);

            //create notebook if this is the first panel
            if (index == 0) { treeWidget = gtk_notebook_new(); }
            else //otherwise retrieve it from first panel
            {
                CIdentifier id;
                window->getChildIdentifier(0, id);
                GtkTreeIter iter;
                m_visualizationTree.findChildNodeFromRoot(&iter, id);
                void* notebookWidget = nullptr;
                m_visualizationTree.getPointerValueFromTreeIter(&iter, notebookWidget, EVTColumn::PointerWidget);
                treeWidget = static_cast<GtkWidget*>(notebookWidget);
            }
        }
    }
    else if (widget->getType() == EVWidget::VerticalSplit || widget->getType() == EVWidget::HorizontalSplit
             || widget->getType() == EVWidget::Undefined || widget->getType() == EVWidget::Box) {
        if (widget->getType() == EVWidget::Box) {
            if (widget->getParentIdentifier() != CIdentifier::undefined()) {
                //dummy widget (actual one will be created at plugin initialization time)
                treeWidget = gtk_button_new();
            }
            else { }    //widget will be added to a top level window in SetWidget()
        }
        else if (widget->getType() == EVWidget::Undefined) {
            treeWidget = gtk_button_new();
            gtk_button_set_label(GTK_BUTTON(treeWidget), widget->getName().toASCIIString());
        }
        else if (widget->getType() == EVWidget::HorizontalSplit || widget->getType() == EVWidget::VerticalSplit) {
            treeWidget = (widget->getType() == EVWidget::HorizontalSplit) ? gtk_hpaned_new() : gtk_vpaned_new();

            //store paned widget in paned map
            m_splitWidgets[GTK_PANED(treeWidget)] = widget->getIdentifier();

            //retrieve its attributes
            const int handlePos = widget->getDividerPosition();

            //initialize paned handle position
            gtk_paned_set_position(GTK_PANED(treeWidget), handlePos);
        }

        //parent widget to its parent
        //---------------------------
        IVWidget* parentWidget = m_visualizationTree.getVisualizationWidget(widget->getParentIdentifier());

        if (parentWidget != nullptr) //unparented visualization boxes don't have a parent
        {
            GtkTreeIter parentIter;
            m_visualizationTree.findChildNodeFromRoot(&parentIter, parentWidget->getIdentifier());

            if (parentWidget->getType() == EVWidget::Panel) {
                //parent widget to notebook as a new page
                void* notebook = nullptr;
                m_visualizationTree.getPointerValueFromTreeIter(&parentIter, notebook, EVTColumn::PointerWidget);
                char* name = nullptr;
                m_visualizationTree.getStringValueFromTreeIter(&parentIter, name, EVTColumn::StringName);
                gtk_notebook_append_page(GTK_NOTEBOOK(notebook), treeWidget, gtk_label_new(name));
            }
            else if (parentWidget->getType() == EVWidget::VerticalSplit || parentWidget->getType() == EVWidget::HorizontalSplit) {
                //insert widget in parent paned
                void* paned = nullptr;
                m_visualizationTree.getPointerValueFromTreeIter(&parentIter, paned, EVTColumn::PointerWidget);
                if (paned != nullptr && GTK_IS_PANED(paned)) {
                    size_t index;
                    if (parentWidget->getChildIndex(widget->getIdentifier(), index)) {
                        if (index == 0) { gtk_paned_pack1(GTK_PANED(paned), treeWidget, TRUE, TRUE); }
                        else { gtk_paned_pack2(GTK_PANED(paned), treeWidget, TRUE, TRUE); }
                    }
                }
            }
        }
    }
    else if (widget->getType() == EVWidget::Window) {
        //create this window only if it contains at least one visualization box
        CIdentifier identifier = CIdentifier::undefined();
        bool createWindow      = false;

        //for all visualization boxes
        while (m_visualizationTree.getNextVisualizationWidgetIdentifier(identifier, EVWidget::Box)) {
            //retrieve window containing current visualization box
            CIdentifier parentID;
            const IVWidget* visualizationWidget = m_visualizationTree.getVisualizationWidget(identifier);
            while (visualizationWidget->getParentIdentifier() != CIdentifier::undefined()) {
                parentID            = visualizationWidget->getParentIdentifier();
                visualizationWidget = m_visualizationTree.getVisualizationWidget(parentID);
            }

            //if current box is parented to window passed in parameter, break and create it
            if (m_visualizationTree.getVisualizationWidget(parentID) == widget) {
                createWindow = true;
                break;
            }
        }

        if (createWindow) {
            //create new top level window
            treeWidget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            m_windows.push_back(GTK_WINDOW(treeWidget));

            //retrieve its size
            gtk_window_set_default_size(GTK_WINDOW(treeWidget), int(widget->getWidth()), int(widget->getHeight()));
            //set its title
            gtk_window_set_title(GTK_WINDOW(treeWidget), static_cast<const char*>(widget->getName()));

            //set it transient for main window
            //gtk_window_set_transient_for(GTK_WINDOW(treeWidget), GTK_WINDOW(m_interfacedScenario.m_rApplication.m_MainWindow));

            //centered on the main window
            if (m_kernelCtx.getConfigurationManager().expandAsBoolean("${Designer_WindowManager_Center}", false)) {
                gtk_window_set_position(GTK_WINDOW(treeWidget), GTK_WIN_POS_CENTER_ON_PARENT);
            }

            //FIXME wrong spelling (-)
            gtk_signal_connect(GTK_OBJECT(treeWidget), "configure_event", G_CALLBACK(configureEventCB), this);
            //FIXME wrong spelling (-)
            g_signal_connect(treeWidget, "delete_event", G_CALLBACK(DeleteWindowManagerWindowCB), this);
        }
    }

    //show newly created widget
    if (treeWidget != nullptr && widget->getType() != EVWidget::Window) { gtk_widget_show(treeWidget); }

    return treeWidget;
}

void CPlayerVisualization::EndLoadTreeWidget(IVWidget* widget)
{
    //retrieve tree widget
    GtkTreeIter iter;
    m_visualizationTree.findChildNodeFromRoot(&iter, widget->getIdentifier());
    void* treeWidget;
    m_visualizationTree.getPointerValueFromTreeIter(&iter, treeWidget, EVTColumn::PointerWidget);

    if (treeWidget != nullptr && widget->getType() == EVWidget::Window) {
        //retrieve notebook
        CIdentifier id;
        widget->getChildIdentifier(0, id);
        GtkTreeIter childIter;
        m_visualizationTree.findChildNodeFromRoot(&childIter, id);
        void* childTree;
        m_visualizationTree.getPointerValueFromTreeIter(&childIter, childTree, EVTColumn::PointerWidget);

        //insert notebook in window
        if (childTree != nullptr && GTK_IS_NOTEBOOK(static_cast<GtkWidget*>(childTree))) {
            gtk_container_add(GTK_CONTAINER(static_cast<GtkWidget*>(treeWidget)), static_cast<GtkWidget*>(childTree));
        }
    }
}

bool CPlayerVisualization::SetToolbar(const CIdentifier& boxID, GtkWidget* widget)
{
    //retrieve visualization widget
    IVWidget* visuWidget = m_visualizationTree.getVisualizationWidgetFromBoxIdentifier(boxID);
    if (visuWidget == nullptr) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "CPlayerVisualization::SetToolbar FAILED : couldn't retrieve simulated box with identifier "
                << boxID << "\n";
        return false;
    }

    //ensure toolbar pointer is not null
    if (widget == nullptr) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "CPlayerVisualization::SetToolbar FAILED : toolbar pointer is nullptr for plugin "
                << visuWidget->getName() << "\n";
        return false;
    }

    //ensure toolbar pointer is a window
    if (GTK_IS_WINDOW(widget) == 0) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "CPlayerVisualization::SetToolbar FAILED : toolbar pointer is not a GtkWindow for plugin "
                << visuWidget->getName() << "\n";
        return false;
    }

    //retrieve identifier
    const CIdentifier id = visuWidget->getIdentifier();

    //store toolbar
    m_plugins[id].m_Toolbar = widget;

    //ensure it is open at mouse position
    gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_MOUSE);

    //if toolbar button has been created, set it sensitive (otherwise it will be set active later)
    if (m_plugins[id].m_ToolbarButton != nullptr) {
        gtk_widget_set_sensitive(GTK_WIDGET(m_plugins[id].m_ToolbarButton), 1);

        //associate toolbar button to toolbar window
        m_toolbars[m_plugins[id].m_ToolbarButton] = widget;
    }

    //catch delete events
    g_signal_connect(G_OBJECT(widget), "delete-event", G_CALLBACK(toolbarDeleteEventCB), this);

    return true;
}

bool CPlayerVisualization::SetWidget(const CIdentifier& boxID, GtkWidget* widget)
{
    //retrieve visualization widget
    IVWidget* visuWidget = m_visualizationTree.getVisualizationWidgetFromBoxIdentifier(boxID);
    if (visuWidget == nullptr) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "CPlayerVisualization::SetWidget FAILED : couldn't retrieve simulated box with identifier "
                << boxID << "\n";
        return false;
    }

    //ensure widget pointer is not null
    if (widget == nullptr) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "CPlayerVisualization::SetWidget FAILED : widget pointer is nullptr for plugin "
                << visuWidget->getName() << "\n";
        return false;
    }

    //unparent top widget, if necessary
    GtkWidget* parent = gtk_widget_get_parent(widget);
    if (GTK_IS_CONTAINER(parent)) {
        gtk_object_ref(GTK_OBJECT(widget));
        gtk_container_remove(GTK_CONTAINER(parent), widget);
    }

    //create a box to store toolbar button and plugin widget
    GtkBox* box = GTK_BOX(gtk_vbox_new(FALSE, 0));
    //gtk_widget_set_size_request(GTK_WIDGET(vBox), 0, 0);

    //create toolbar button
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(gtk_toggle_button_new());
    {
        //horizontal container : icon + label
        GtkBox* hBox = GTK_BOX(gtk_hbox_new(FALSE, 0));
        //gtk_widget_set_size_request(GTK_WIDGET(hBox), 0, 0);

        //retrieve icon name
        GtkTreeIter iter;
        char* iconString = nullptr;
        if (m_visualizationTree.findChildNodeFromRoot(&iter, static_cast<const char*>(visuWidget->getName()), EVTNode::VisualizationBox)) {
            m_visualizationTree.getStringValueFromTreeIter(&iter, iconString, EVTColumn::StringStockIcon);
        }

        //create icon
        GtkWidget* icon = gtk_image_new_from_stock(iconString != nullptr ? iconString : GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON);
        //gtk_widget_set_size_request(icon, 0, 0);
        gtk_box_pack_start(hBox, icon, TRUE, TRUE, 0);

        //create label
        GtkWidget* label = gtk_label_new(static_cast<const char*>(visuWidget->getName()));
        //gtk_widget_set_size_request(label, 0, 0);
        gtk_box_pack_start(hBox, label, TRUE, TRUE, 0);

        //add box to button
        gtk_container_add(GTK_CONTAINER(button), GTK_WIDGET(hBox));
    }

    //detect toolbar button toggle events
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(toolbarButtonToggledCB), this);

    //set up toolbar button as drag destination
    gtk_drag_dest_set(GTK_WIDGET(button), GTK_DEST_DEFAULT_ALL, targets.data(), gint(targets.size()), GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(button), "drag-data-received", G_CALLBACK(dragDataReceivedInWidgetCB), this);

    //set up toolbar button as drag source as well
    gtk_drag_source_set(GTK_WIDGET(button), GDK_BUTTON1_MASK, targets.data(), gint(targets.size()), GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(button), "drag-data-get", G_CALLBACK(dragDataGetFromWidgetCB), this);

    //store plugin widget and toolbar button
    const CIdentifier id          = visuWidget->getIdentifier();
    m_plugins[id].m_Widget        = widget;
    m_plugins[id].m_ToolbarButton = button;

    //if a toolbar was registered for this widget, set its button sensitive
    if (m_plugins[id].m_Toolbar != nullptr) {
        gtk_widget_set_sensitive(GTK_WIDGET(button), 1);

        //associate toolbar button to toolbar window
        m_toolbars[button] = m_plugins[id].m_Toolbar;
    }
    else { gtk_widget_set_sensitive(GTK_WIDGET(button), 0); }

    //vertical container : button on top, visualization box below
    gtk_box_pack_start(box, GTK_WIDGET(button), FALSE, TRUE, 0);
    gtk_box_pack_start(box, widget, TRUE, TRUE, 0);

    //show vbox hierarchy
    gtk_widget_show_all(GTK_WIDGET(box));

    //parent box at the appropriate location
    parentWidgetBox(visuWidget, box);

    return true;
}

bool CPlayerVisualization::parentWidgetBox(IVWidget* widget, GtkBox* widgetBox)
{
    //if widget is unaffected, open it in its own window
    if (widget->getParentIdentifier() == CIdentifier::undefined()) {
        //create a top level window
        GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        m_windows.push_back(GTK_WINDOW(window));
        const uint64_t width  = m_kernelCtx.getConfigurationManager().expandAsUInteger("${Designer_UnaffectedVisualizationWindowWidth}", 400);
        const uint64_t height = m_kernelCtx.getConfigurationManager().expandAsUInteger("${Designer_UnaffectedVisualizationWindowHeight}", 400);

        gtk_window_set_default_size(GTK_WINDOW(window), gint(width), gint(height));
        //set its title
        gtk_window_set_title(GTK_WINDOW(window), static_cast<const char*>(widget->getName()));
        //set it transient for main window
        //gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(m_interfacedScenario.m_rApplication.m_MainWindow));
        //insert box in top level window
        gtk_container_add(GTK_CONTAINER(window), reinterpret_cast<GtkWidget*>(widgetBox));
        //prevent user from closing this window
        g_signal_connect(window, "delete_event", G_CALLBACK(DeleteWindowManagerWindowCB), this);

        //position: centered in the main window
        if (m_kernelCtx.getConfigurationManager().expandAsBoolean("${Designer_WindowManager_Center}", false)) {
            gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
        }

        //show window (and realize widget in doing so)
        gtk_widget_show(window);
    }
    else //retrieve parent widget in which to insert current widget
    {
        GtkTreeIter iter;
        if (m_visualizationTree.findChildNodeFromRoot(&iter, widget->getParentIdentifier())) {
            void* parent = nullptr;
            m_visualizationTree.getPointerValueFromTreeIter(&iter, parent, EVTColumn::PointerWidget);
            CIdentifier id;
            m_visualizationTree.getIdentifierFromTreeIter(&iter, id, EVTColumn::StringIdentifier);

            //widget is to be parented to a paned widget
            if (GTK_IS_PANED(parent)) {
                //retrieve index at which to insert child
                const IVWidget* parentWidget = m_visualizationTree.getVisualizationWidget(id);
                size_t index;
                parentWidget->getChildIndex(widget->getIdentifier(), index);
                //insert visualization box in paned
                if (index == 0) {
                    gtk_container_remove(GTK_CONTAINER(parent), gtk_paned_get_child1(GTK_PANED(parent)));
                    gtk_paned_pack1(GTK_PANED(parent), GTK_WIDGET(widgetBox), TRUE, TRUE);
                }
                else {
                    gtk_container_remove(GTK_CONTAINER(parent), gtk_paned_get_child2(GTK_PANED(parent)));
                    gtk_paned_pack2(GTK_PANED(parent), GTK_WIDGET(widgetBox), TRUE, TRUE);
                }
            }
            else if (GTK_IS_NOTEBOOK(parent)) //widget is to be added to a notebook page
            {
                //retrieve notebook page index
                const IVWidget* parentWidget = m_visualizationTree.getVisualizationWidget(id);
                const IVWidget* parentWindow = m_visualizationTree.getVisualizationWidget(parentWidget->getParentIdentifier());
                size_t index;
                parentWindow->getChildIndex(parentWidget->getIdentifier(), index);

                //remove temporary page
                gtk_notebook_remove_page(GTK_NOTEBOOK(parent), gint(index));

                //insert final page
                gtk_notebook_insert_page(GTK_NOTEBOOK(parent), GTK_WIDGET(widgetBox), gtk_label_new(parentWidget->getName().toASCIIString()), gint(index));
            }

            //resize widgets once they are allocated : this is the case when they are shown on an expose event
            //FIXME : perform resizing only once (when it is done as many times as there are widgets in the tree here)
            if (parent != nullptr) { gtk_signal_connect(GTK_OBJECT(parent), "expose-event", G_CALLBACK(widgetExposeEventCB), this); }

            //show window (and realize widget if it owns a 3D context)
            //--------------------------------------------------------
            //get panel containing widget
            GtkTreeIter panelIter = iter;
            if (m_visualizationTree.findParentNode(&panelIter, EVTNode::VisualizationPanel)) {
                //get panel identifier
                m_visualizationTree.getIdentifierFromTreeIter(&panelIter, id, EVTColumn::StringIdentifier);

                //get panel index in window
                size_t index;
                m_visualizationTree.getVisualizationWidgetIndex(id, index);

                //get notebook pointer
                void* panelWidget = nullptr;
                m_visualizationTree.getPointerValueFromTreeIter(&panelIter, panelWidget, EVTColumn::PointerWidget);

                //get parent window
                GtkTreeIter windowIter = panelIter;
                if (m_visualizationTree.findParentNode(&windowIter, EVTNode::VisualizationWindow)) {
                    //get parent window pointer
                    void* windowWidget = nullptr;
                    m_visualizationTree.getPointerValueFromTreeIter(&windowIter, windowWidget, EVTColumn::PointerWidget);

                    //show parent window
                    gtk_widget_show(GTK_WIDGET(windowWidget));
                    // gtk_widget_realize(GTK_WIDGET(windowWidget));
                    // gdk_flush();

                    //set panel containing widget as current (this realizes the widget)
                    gtk_notebook_set_current_page(GTK_NOTEBOOK(panelWidget), gint(index));

                    //then reset first panel as current
                    gtk_notebook_set_current_page(GTK_NOTEBOOK(panelWidget), 0);
                }
            }
        }
    }

    return true;
}

//called upon Player start
void CPlayerVisualization::ShowTopLevelWindows()

{
    for (const auto& window : m_windows) { gtk_widget_show(GTK_WIDGET(window)); }
    if (m_activeToolbarButton != nullptr) {
        //show active toolbar
        gtk_widget_show(m_toolbars[m_activeToolbarButton]);
    }
    auto it = m_plugins.begin();
    while (it != m_plugins.end()) {
        if (GTK_IS_WIDGET(it->second.m_Widget)) { gtk_widget_show(it->second.m_Widget); }
        ++it;
    }
}

//called upon Player stop
void CPlayerVisualization::HideTopLevelWindows()

{
    auto it = m_plugins.begin();
    while (it != m_plugins.end()) {
        if (GTK_IS_WIDGET(it->second.m_Widget)) { gtk_widget_hide(it->second.m_Widget); }
        ++it;
    }

    for (const auto& window : m_windows) { gtk_widget_hide(GTK_WIDGET(window)); }

    //hide active toolbar
    if (m_activeToolbarButton != nullptr) { gtk_widget_hide(m_toolbars[m_activeToolbarButton]); }
}

//event generated whenever window changes size, including when it is first created
gboolean CPlayerVisualization::configureEventCB(GtkWidget* widget, GdkEventConfigure* /*event*/, gpointer data)
{
    //paned positions aren't to be saved, they are to be read once only
    g_signal_handlers_disconnect_by_func(G_OBJECT(widget), G_CALLBACK2(CPlayerVisualization::configureEventCB), data);

    if (GTK_IS_CONTAINER(widget)) { static_cast<CPlayerVisualization*>(data)->resizeCB(GTK_CONTAINER(widget)); }

    return FALSE;
}

gboolean CPlayerVisualization::widgetExposeEventCB(GtkWidget* widget, GdkEventExpose* /*event*/, gpointer data)
{
    g_signal_handlers_disconnect_by_func(G_OBJECT(widget), G_CALLBACK2(CPlayerVisualization::widgetExposeEventCB), data);
    /*
        //retrieve topmost widget
        while(gtk_widget_get_parent(widget) != nullptr && GTK_IS_CONTAINER(gtk_widget_get_parent(widget))) { widget = gtk_widget_get_parent(widget); }
    */
    if (GTK_IS_CONTAINER(widget)) { static_cast<CPlayerVisualization*>(data)->resizeCB(GTK_CONTAINER(widget)); }

    return FALSE;
}

void CPlayerVisualization::resizeCB(GtkContainer* container)
{
    if (GTK_IS_WINDOW(container)) {
        const gpointer data = g_list_first(gtk_container_get_children(container))->data;

        if (GTK_IS_CONTAINER(data)) { resizeCB(GTK_CONTAINER(data)); }
    }
    else if (GTK_IS_NOTEBOOK(container)) {
        GtkNotebook* notebook = GTK_NOTEBOOK(container);

        for (int i = 0; i < gtk_notebook_get_n_pages(notebook); ++i) {
            GtkWidget* widget = gtk_notebook_get_nth_page(notebook, i);
            if (GTK_IS_CONTAINER(widget)) { resizeCB(GTK_CONTAINER(widget)); }
        }
    }
    else if (GTK_IS_PANED(container)) {
        GtkPaned* paned = GTK_PANED(container);

        //retrieve paned identifier from paned map
        const CIdentifier& id = m_splitWidgets[GTK_PANED(paned)];

        //retrieve its attributes
        IVWidget* widget = m_visualizationTree.getVisualizationWidget(id);
        const int pos    = widget->getDividerPosition();
        const int maxPos = widget->getMaxDividerPosition();

        if (maxPos > 0) {
            //retrieve current maximum handle position
            const int currentMaxPos = GTK_IS_VPANED(paned) ? paned->container.widget.allocation.height : paned->container.widget.allocation.width;

            //set new paned handle position
            gtk_paned_set_position(paned, pos * currentMaxPos / maxPos);
        }

        //go down each child
        GtkWidget* child = gtk_paned_get_child1(paned);
        if (GTK_IS_CONTAINER(child)) { resizeCB(GTK_CONTAINER(child)); }

        child = gtk_paned_get_child2(paned);
        if (GTK_IS_CONTAINER(child)) { resizeCB(GTK_CONTAINER(child)); }
    }
}

void CPlayerVisualization::dragDataGetFromWidgetCB(GtkWidget* srcWidget, GdkDragContext* /*dc*/, GtkSelectionData* selectionData,
                                                   guint /*info*/, guint /*time*/, gpointer /*data*/)
{
    char str[1024];
    sprintf(str, "%p", srcWidget);
    gtk_selection_data_set_text(selectionData, str, gint(strlen(str)));
}

void CPlayerVisualization::dragDataReceivedInWidgetCB(GtkWidget* dstWidget, GdkDragContext* /*dc*/, gint /*x*/, gint /*y*/, GtkSelectionData* selectionData,
                                                      guint /*info*/, guint /*time*/, gpointer /*data*/)
{
    void* srcWidget = nullptr;
    sscanf(reinterpret_cast<const char*>(gtk_selection_data_get_text(selectionData)), "%p", &srcWidget);

    //retrieve source box and parent widgets
    GtkWidget* srcBoxWidget;
    do { srcBoxWidget = gtk_widget_get_parent(GTK_WIDGET(srcWidget)); } while (srcBoxWidget != nullptr && !GTK_IS_VBOX(srcBoxWidget));

    if (srcBoxWidget == nullptr) { return; }

    GtkWidget* srcParentWidget = gtk_widget_get_parent(srcBoxWidget);

    if (srcParentWidget == nullptr) { return; }

    //retrieve dest box and parent widgets
    GtkWidget* dstBoxWidget;
    do { dstBoxWidget = gtk_widget_get_parent(dstWidget); } while (dstBoxWidget != nullptr && !GTK_IS_VBOX(dstBoxWidget));

    if (dstBoxWidget == nullptr) { return; }

    GtkWidget* dstParentWidget = gtk_widget_get_parent(dstBoxWidget);

    if (dstParentWidget == nullptr) { return; }

    //ensure src and dst widgets are different
    if (srcBoxWidget == dstBoxWidget) { return; }

    //remove src box from parent
    int srcIndex;
    GtkWidget* srcTabLabel = nullptr;

    if (GTK_IS_WINDOW(srcParentWidget)) { srcIndex = 0; }
    else if (GTK_IS_NOTEBOOK(srcParentWidget)) {
        srcIndex    = gtk_notebook_page_num(GTK_NOTEBOOK(srcParentWidget), srcBoxWidget);
        srcTabLabel = gtk_label_new(gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(srcParentWidget), srcBoxWidget));
    }
    else if (GTK_IS_PANED(srcParentWidget)) { srcIndex = reinterpret_cast<GtkPaned*>(srcParentWidget)->child1 == srcBoxWidget ? 1 : 2; }
    else { return; }

    //remove dst box from parent
    int dstIndex;
    GtkWidget* dstTabLabel = nullptr;
    if (GTK_IS_WINDOW(dstParentWidget)) { dstIndex = 0; }
    else if (GTK_IS_NOTEBOOK(dstParentWidget)) {
        dstIndex    = gtk_notebook_page_num(GTK_NOTEBOOK(dstParentWidget), dstBoxWidget);
        dstTabLabel = gtk_label_new(gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(dstParentWidget), dstBoxWidget));
    }
    else if (GTK_IS_PANED(dstParentWidget)) { dstIndex = reinterpret_cast<GtkPaned*>(dstParentWidget)->child1 == dstBoxWidget ? 1 : 2; }
    else { return; }

    gtk_object_ref(GTK_OBJECT(srcBoxWidget));
    gtk_container_remove(GTK_CONTAINER(srcParentWidget), srcBoxWidget);

    gtk_object_ref(GTK_OBJECT(dstBoxWidget));
    gtk_container_remove(GTK_CONTAINER(dstParentWidget), dstBoxWidget);

    //parent src box to dst parent
    if (GTK_IS_WINDOW(dstParentWidget)) { gtk_container_add(GTK_CONTAINER(dstParentWidget), srcBoxWidget); }
    else if (GTK_IS_NOTEBOOK(dstParentWidget)) { gtk_notebook_insert_page(GTK_NOTEBOOK(dstParentWidget), srcBoxWidget, dstTabLabel, dstIndex); }
    else //dst parent is a paned
    {
        if (dstIndex == 1) { gtk_paned_pack1(GTK_PANED(dstParentWidget), srcBoxWidget, TRUE, TRUE); }
        else { gtk_paned_pack2(GTK_PANED(dstParentWidget), srcBoxWidget, TRUE, TRUE); }
    }

    //parent dst box to src parent
    if (GTK_IS_WINDOW(srcParentWidget)) { gtk_container_add(GTK_CONTAINER(srcParentWidget), dstBoxWidget); }
    else if (GTK_IS_NOTEBOOK(srcParentWidget)) { gtk_notebook_insert_page(GTK_NOTEBOOK(srcParentWidget), dstBoxWidget, srcTabLabel, srcIndex); }
    else //src parent is a paned
    {
        if (srcIndex == 1) { gtk_paned_pack1(GTK_PANED(srcParentWidget), dstBoxWidget, TRUE, TRUE); }
        else { gtk_paned_pack2(GTK_PANED(srcParentWidget), dstBoxWidget, TRUE, TRUE); }
    }
}

void CPlayerVisualization::toolbarButtonToggledCB(GtkToggleButton* button, gpointer data) { static_cast<CPlayerVisualization*>(data)->toggleToolbarCB(button); }

bool CPlayerVisualization::toggleToolbarCB(GtkToggleButton* button)
{
    //retrieve toolbar
    if (m_toolbars.find(button) == m_toolbars.end()) { return false; }
    GtkWidget* toolbar = m_toolbars[button];

    //if current toolbar is toggled on or off, update toolbar state accordingly
    if (button == m_activeToolbarButton) {
        //hiding active toolbar
        if (gtk_toggle_button_get_active(button) == FALSE) {
            gtk_widget_hide(toolbar);
            m_activeToolbarButton = nullptr;
        }
        else { gtk_widget_show(toolbar); } //showing active toolbar 
    }
    else //a new toolbar is to be shown
    {
        //hide previously active toolbar, if any
        if (m_activeToolbarButton != nullptr) {
            gtk_widget_hide(m_toolbars[m_activeToolbarButton]);

            g_signal_handlers_disconnect_by_func(m_activeToolbarButton, G_CALLBACK2(toolbarButtonToggledCB), this);
            gtk_toggle_button_set_active(m_activeToolbarButton, FALSE);
            g_signal_connect(m_activeToolbarButton, "toggled", G_CALLBACK(toolbarButtonToggledCB), this);
        }
        /*
        //set toolbar transient for plugin window
        ::GtkWidget* widget = GTK_WIDGET(pToolbarButton);
        while(widget!=nullptr && !GTK_IS_WINDOW(widget)) { widget = gtk_widget_get_parent(widget); }
        if(widget != nullptr && GTK_IS_WINDOW(toolbar)) { gtk_window_set_transient_for(GTK_WINDOW(toolbar), GTK_WINDOW(widget)); }
        */
        //show new toolbar
        gtk_widget_show(toolbar);

        //update active toolbar button
        m_activeToolbarButton = button;
    }

    return true;
}

gboolean CPlayerVisualization::toolbarDeleteEventCB(GtkWidget* widget, GdkEvent* /*event*/, gpointer data)
{
    if (data != nullptr) { static_cast<CPlayerVisualization*>(data)->deleteToolbarCB(widget); }
    return TRUE;
}

bool CPlayerVisualization::deleteToolbarCB(GtkWidget* widget)
{
    if (m_activeToolbarButton == nullptr || m_toolbars[m_activeToolbarButton] != widget) {
        //error : active toolbar isn't the one registered as such
        gtk_widget_hide(widget);
        return FALSE;
    }

    //toggle toolbar button off
    g_signal_handlers_disconnect_by_func(m_activeToolbarButton, G_CALLBACK2(toolbarButtonToggledCB), this);
    gtk_toggle_button_set_active(m_activeToolbarButton, FALSE);
    g_signal_connect(m_activeToolbarButton, "toggled", G_CALLBACK(toolbarButtonToggledCB), this);

    //hide toolbar
    gtk_widget_hide(widget);

    //clear active toolbar button pointer
    m_activeToolbarButton = nullptr;

    return TRUE;
}

}  // namespace Designer
}  // namespace OpenViBE
