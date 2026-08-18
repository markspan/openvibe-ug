///-------------------------------------------------------------------------------------------------
/// 
/// \file CDesignerVisualization.cpp
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
#include "CDesignerVisualization.hpp"
#include "CInterfacedScenario.hpp"
#include "CInputDialog.hpp"

#include <gdk/gdkkeysyms.h>
#include <cstring>
#include <array>

namespace OpenViBE {
namespace Designer {

using EVTColumn = VisualizationToolkit::EVisualizationTreeColumn;
using EVTNode = VisualizationToolkit::EVisualizationTreeNode;
using EVWidget = VisualizationToolkit::EVisualizationWidget;
using IVWidget = VisualizationToolkit::IVisualizationWidget;

static const std::array<GtkTargetEntry, 2> TARGETS = { { { const_cast<gchar*>("STRING"), 0, 0 }, { const_cast<gchar*>("text/plain"), 0, 0 } } };

/**
 * \brief Display an error dialog
 * \param[in] text text to display in the dialog
 * \param[in] secondaryText additional text to display in the dialog
 */
void displayErrorDialog(const char* text, const char* secondaryText)
{
    GtkWidget* dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", text);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondaryText);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/**
 * \brief Helper function retrieving a child in a table from its attach indices
 * \param table Table parent to the child to be retrieved
 * \param leftAttach Left attach index
 * \param rightAttach Right attach index
 * \param topAttach Top attach index
 * \param bottomAttach Bottom attach index
 * \return Pointer to table child if one was found, nullptr otherwise
 */
GtkTableChild* getTableChild(const GtkTable* table, const int leftAttach, const int rightAttach, const int topAttach, const int bottomAttach)
{
    GList* list = table->children;

    do {
        GtkTableChild* pTC = static_cast<GtkTableChild*>(list->data);
        if (pTC->left_attach == leftAttach && pTC->right_attach == rightAttach &&
            pTC->top_attach == topAttach && pTC->bottom_attach == bottomAttach) { return pTC; }
        list = list->next;
    } while (list);

    return nullptr;
}

/**
 * \brief Display a yes/no question dialog
 * \param[in] pText text to display in the dialog
 * \param[in] pSecondaryText additional text to display in the dialog
 * \return identifier of the button pressed
 */
/*
gint displayQuestionDialog(const char* pText, const char* pSecondaryText)
{
    ::GtkWidget* dialog = gtk_message_dialog_new(nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, pText);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),    pSecondaryText);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gint ret = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return ret;
}*/

//Menus
//-----

static char* cNewWin = const_cast<char*>("/New window");
static char* cNewTab = const_cast<char*>("/New tab");
static char* cRename = const_cast<char*>("/Rename");
static char* cRemove = const_cast<char*>("/Remove");
static char* cStockI = const_cast<char*>("<StockItem>");
static char* cBlank  = const_cast<char*>("");

static std::array<GtkItemFactoryEntry, 1> unaffectedItems = {
    { { cNewWin, cBlank, GtkItemFactoryCallback(CDesignerVisualization::AskNewVisualizationWindowCB), 1, cStockI, GTK_STOCK_DND_MULTIPLE } }
};

static std::array<GtkItemFactoryEntry, 3> visualizationWindowItems = {
    {
        { cNewTab, cBlank, GtkItemFactoryCallback(CDesignerVisualization::AskNewVisualizationPanelCB), 1, cStockI, GTK_STOCK_DND },
        { cRename, cBlank, GtkItemFactoryCallback(CDesignerVisualization::AskRenameVisualizationWindowCB), 1, cStockI, GTK_STOCK_BOLD },
        { cRemove, cBlank, GtkItemFactoryCallback(CDesignerVisualization::RemoveVisualizationWindowCB), 1, cStockI, GTK_STOCK_DELETE }
    }
};

static std::array<GtkItemFactoryEntry, 2> visualizationPanelItems = {
    {
        { cRename, cBlank, GtkItemFactoryCallback(CDesignerVisualization::AskRenameVisualizationPanelCB), 1, cStockI, GTK_STOCK_BOLD },
        { cRemove, cBlank, GtkItemFactoryCallback(CDesignerVisualization::RemoveVisualizationPanelCB), 1, cStockI, GTK_STOCK_DELETE }
    }
};

static std::array<GtkItemFactoryEntry, 1> visualizationBoxItems = {
    { { cRemove, cBlank, GtkItemFactoryCallback(CDesignerVisualization::RemoveVisualizationWidgetCB), 1, cStockI, GTK_STOCK_DELETE } }
};

static std::array<GtkItemFactoryEntry, 1> undefinedWidgetItems = {
    { { cRemove, cBlank, GtkItemFactoryCallback(CDesignerVisualization::RemoveVisualizationWidgetCB), 1, cStockI, GTK_STOCK_DELETE } }
};

static std::array<GtkItemFactoryEntry, 1> splitWidgetItems = {
    { { cRemove, cBlank, GtkItemFactoryCallback(CDesignerVisualization::RemoveVisualizationWidgetCB), 1, cStockI, GTK_STOCK_DELETE } }
};


CDesignerVisualization::~CDesignerVisualization()
{
    g_signal_handlers_disconnect_by_func(G_OBJECT(m_dialog), G_CALLBACK2(ConfigureEventCB), this);
#ifdef HANDLE_MIN_MAX_EVENTS
    g_signal_handlers_disconnect_by_func(G_OBJECT(m_dialog), G_CALLBACK2(window_state_event_cb), this);
#endif
    gtk_widget_destroy(m_dialog);

    m_tree.setTreeViewCB(nullptr);
}

void CDesignerVisualization::Init(const std::string& guiFile)
{
    m_guiFile = guiFile;

    //create tree view
    //----------------

    //register towards tree store
    m_tree.setTreeViewCB(this);

    m_treeView = m_tree.createTreeViewWithModel();

    GtkTreeViewColumn* treeViewColumnName = gtk_tree_view_column_new();
    GtkCellRenderer* cellRendererIcon     = gtk_cell_renderer_pixbuf_new();
    GtkCellRenderer* cellRendererName     = gtk_cell_renderer_text_new();
    gtk_tree_view_column_set_title(treeViewColumnName, "Windows for current scenario");
    gtk_tree_view_column_pack_start(treeViewColumnName, cellRendererIcon, FALSE);
    gtk_tree_view_column_pack_start(treeViewColumnName, cellRendererName, TRUE);
    gtk_tree_view_column_set_attributes(treeViewColumnName, cellRendererIcon, "stock-id", EVTColumn::StringStockIcon, nullptr);
    gtk_tree_view_column_set_attributes(treeViewColumnName, cellRendererName, "text", EVTColumn::StringName, nullptr);
    //gtk_tree_view_column_set_sizing(treeViewColumnName, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_expand(treeViewColumnName, TRUE/*FALSE*/);
    gtk_tree_view_column_set_resizable(treeViewColumnName, TRUE);
    gtk_tree_view_column_set_min_width(treeViewColumnName, 64);
    gtk_tree_view_append_column(m_treeView, treeViewColumnName);

    GtkTreeViewColumn* desc = gtk_tree_view_column_new();
    gtk_tree_view_append_column(m_treeView, desc);

    gtk_tree_view_column_set_visible(desc, 0);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(m_treeView));

    //allow tree items to be dragged
    gtk_drag_source_set(GTK_WIDGET(m_treeView), GDK_BUTTON1_MASK, TARGETS.data(), gint(TARGETS.size()), GDK_ACTION_COPY);

    //require notifications upon tree item dragging, mouse button release, active item change
    g_signal_connect(G_OBJECT(m_treeView), "drag-data-get", G_CALLBACK(dragDataGetFromTreeCB), this);
    g_signal_connect(G_OBJECT(m_treeView), "button-release-event", G_CALLBACK(buttonReleaseCB), this);
    g_signal_connect(G_OBJECT(m_treeView), "cursor-changed", G_CALLBACK(cursorChangedCB), this);

    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(m_treeView), GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(m_treeView), "key-press-event", G_CALLBACK(widgetKeyPressEventCB), this);

    //create main dialog
    //------------------
    m_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    //retrieve default window size
    const size_t treeViewWidth = 200;
    m_previewWindowW           = size_t(m_kernelCtx.getConfigurationManager().expandAsUInteger("${Designer_UnaffectedVisualizationWindowWidth}", 400));
    m_previewWindowH           = size_t(m_kernelCtx.getConfigurationManager().expandAsUInteger("${Designer_UnaffectedVisualizationWindowHeight}", 400));
    CIdentifier windowID;
    //if at least one window was created, retrieve its dimensions
    if (m_tree.getNextVisualizationWidgetIdentifier(windowID, EVWidget::Window)) {
        IVWidget* window = m_tree.getVisualizationWidget(windowID);
        m_previewWindowW = window->getWidth();
        m_previewWindowH = window->getHeight();
        /* Change the way window sizes are stored in the widget
        TAttributeHandler handler(*window);
        m_previewWindowW = handler.getAttributeValue<int>(AttributeId_VisualizationWindow_Width);
        m_previewWindowH = handler.getAttributeValue<int>(AttributeId_VisualizationWindow_Height);
        */
    }
    gtk_window_set_default_size(GTK_WINDOW(m_dialog), gint(treeViewWidth + m_previewWindowW), gint(m_previewWindowH));
    //set window title
    gtk_window_set_title(GTK_WINDOW(m_dialog), " Window Manager");
    // gtk_window_set_transient_for(GTK_WINDOW(m_dialog), GTK_WINDOW(m_scenario.m_application.m_MainWindow));
    gtk_signal_connect(GTK_OBJECT(m_dialog), "configure_event", G_CALLBACK(ConfigureEventCB), this);
#ifdef HANDLE_MIN_MAX_EVENTS
    gtk_signal_connect(GTK_OBJECT(m_dialog), "window_state_event", G_CALLBACK(window_state_event_cb), this);
#endif
    g_signal_connect(G_OBJECT(m_dialog), "delete-event", G_CALLBACK(deleteEventCB), this);

    //main pane : tree view to the left, widgets table to the right
    m_pane = gtk_hpaned_new();
    gtk_container_add(GTK_CONTAINER(m_dialog), GTK_WIDGET(m_pane));

    // Add a scrollview to above the treeview

    GtkWidget* scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), GTK_WIDGET(m_treeView));

    //add tree view to pane
    gtk_paned_add1(GTK_PANED(m_pane), GTK_WIDGET(scrolledWindow));

    //set initial divider position
    gtk_paned_set_position(GTK_PANED(m_pane), gint(treeViewWidth));

    //create popup menus
    //------------------
    m_unaffectedItemFactory = gtk_item_factory_new(GTK_TYPE_MENU, "<unaffected_main>", nullptr);
    gtk_item_factory_create_items(m_unaffectedItemFactory, guint(unaffectedItems.size()), unaffectedItems.data(), this);

    m_visualizationWindowItemFactory = gtk_item_factory_new(GTK_TYPE_MENU, "<visualization_window_main>", nullptr);
    gtk_item_factory_create_items(m_visualizationWindowItemFactory, guint(visualizationWindowItems.size()), visualizationWindowItems.data(), this);

    m_visualizationPanelItemFactory = gtk_item_factory_new(GTK_TYPE_MENU, "<visualization_panel_main>", nullptr);
    gtk_item_factory_create_items(m_visualizationPanelItemFactory, guint(visualizationPanelItems.size()), visualizationPanelItems.data(), this);

    m_visualizationBoxItemFactory = gtk_item_factory_new(GTK_TYPE_MENU, "<visualization_box_main>", nullptr);
    gtk_item_factory_create_items(m_visualizationBoxItemFactory, guint(visualizationBoxItems.size()), visualizationBoxItems.data(), this);

    m_undefinedItemFactory = gtk_item_factory_new(GTK_TYPE_MENU, "<undefined_widget_main>", nullptr);
    gtk_item_factory_create_items(m_undefinedItemFactory, guint(undefinedWidgetItems.size()), undefinedWidgetItems.data(), this);

    m_splitItemFactory = gtk_item_factory_new(GTK_TYPE_MENU, "<split_widget_main>", nullptr);
    gtk_item_factory_create_items(m_splitItemFactory, guint(splitWidgetItems.size()), splitWidgetItems.data(), this);
}

void CDesignerVisualization::Load()
{
    m_tree.setTreeViewCB(this);
    m_tree.reloadTree();

    //if at least one window was created, retrieve its dimensions
    CIdentifier id;
    if (m_tree.getNextVisualizationWidgetIdentifier(id, EVWidget::Window)) {
        IVWidget* window = m_tree.getVisualizationWidget(id);
        m_previewWindowW = window->getWidth();
        m_previewWindowH = window->getHeight();
    }
    const size_t width = gtk_paned_get_position(GTK_PANED(m_pane));
    gtk_widget_set_size_request(GTK_WIDGET(m_dialog), gint(width + m_previewWindowW), gint(m_previewWindowH));
    gtk_tree_view_expand_all(m_treeView);
    setActiveVisualization(m_activeVisualizationWindowName, m_activeVisualizationPanelName);
}

void CDesignerVisualization::Show() const
{
    // since gtk is asynchronous for the expose event, the m_previewWindowVisible flag is turned on in the corresponding callback
    //m_previewWindowVisible = true;
    gtk_widget_show_all(static_cast<GtkWidget*>(m_dialog));
}

void CDesignerVisualization::Hide()
{
    m_previewWindowVisible = false;
    gtk_widget_hide_all(static_cast<GtkWidget*>(m_dialog));
}

void CDesignerVisualization::SetDeleteEventCB(const visualization_delete_event_cb_t cb, gpointer data)
{
    m_deleteEventCB       = cb;
    m_deleteEventUserData = data;
}

void CDesignerVisualization::OnVisualizationBoxAdded(const Kernel::IBox* box)
{
    CIdentifier widgetID;
    m_tree.addVisualizationWidget(widgetID, box->getName(), EVWidget::Box, CIdentifier::undefined(), 0, box->getIdentifier(), 0, CIdentifier::undefined());

    m_tree.reloadTree();

    //refresh view
    GtkTreeIter iter;
    m_tree.findChildNodeFromRoot(&iter, widgetID);
    refreshActiveVisualization(m_tree.getTreePath(&iter));
}

void CDesignerVisualization::OnVisualizationBoxRemoved(const CIdentifier& boxID)
{
    IVWidget* widget = m_tree.getVisualizationWidgetFromBoxIdentifier(boxID);
    if (widget != nullptr) {
        //unaffected widget : delete it
        if (widget->getParentIdentifier() == CIdentifier::undefined()) { m_tree.destroyHierarchy(widget->getIdentifier()); }
        else { destroyVisualizationWidget(widget->getIdentifier()); }   //simplify tree

        m_tree.reloadTree();

        //refresh view
        refreshActiveVisualization(nullptr);
    }
}

void CDesignerVisualization::OnVisualizationBoxRenamed(const CIdentifier& boxID)
{
    //retrieve visualization widget
    IVWidget* widget = m_tree.getVisualizationWidgetFromBoxIdentifier(boxID);
    if (widget != nullptr) {
        //retrieve box name
        const Kernel::IBox* box = m_scenario.m_Scenario.getBoxDetails(boxID);
        if (box != nullptr) {
            widget->setName(box->getName());        //set new visualization widget name
            m_tree.reloadTree();                    //reload tree
            refreshActiveVisualization(nullptr);    //refresh view
        }
    }
}

void CDesignerVisualization::CreateTreeWidget(IVWidget* widget)
{
    if (widget->getType() == EVWidget::HorizontalSplit || widget->getType() == EVWidget::VerticalSplit) {
        /* TODO_JL: Find a way to store divider position and max divider position
        TAttributeHandler handler(*widget);
        handler.addAttribute(AttributeId_EVisualizationWidget::DividerPosition, 1);
        handler.addAttribute(AttributeId_EVisualizationWidget::MaxDividerPosition, 2);
        */
    }
}

//need width request of 0 to avoid graphical bugs (label/icon overlapping other widgets) when shrinking buttons
static const gint LABEL_WIDTH_REQUEST = 0;
static const gint ICON_WIDTH_REQUEST  = 0;
//need expand and fill flags to TRUE to see 0-size-requesting widgets
static const gboolean LABEL_EXPAND = TRUE;
static const gboolean LABEL_FILL   = TRUE;
static const gboolean ICON_EXPAND  = TRUE;
static const gboolean ICON_FILL    = TRUE;

GtkWidget* CDesignerVisualization::LoadTreeWidget(IVWidget* widget)
{
    GtkWidget* treeWidget = nullptr;

    //create widget
    //-------------
    if (widget->getType() == EVWidget::Panel) {
        //retrieve panel index
        IVWidget* window = m_tree.getVisualizationWidget(widget->getParentIdentifier());
        if (window != nullptr) {
            size_t idx;
            window->getChildIndex(widget->getIdentifier(), idx);

            //create notebook if this is the first panel
            if (idx == 0) { treeWidget = gtk_notebook_new(); }
            else //otherwise retrieve it from first panel
            {
                CIdentifier firstPanelID;
                window->getChildIdentifier(0, firstPanelID);
                GtkTreeIter firstPanelIter;
                m_tree.findChildNodeFromRoot(&firstPanelIter, firstPanelID);
                void* notebookWidget = nullptr;
                m_tree.getPointerValueFromTreeIter(&firstPanelIter, notebookWidget, EVTColumn::PointerWidget);
                treeWidget = static_cast<GtkWidget*>(notebookWidget);
            }
        }
    }
    else if (widget->getType() == EVWidget::VerticalSplit || widget->getType() == EVWidget::HorizontalSplit
             || widget->getType() == EVWidget::Undefined || widget->getType() == EVWidget::Box) {
        //tree widget = table containing event boxes + visualization widget in the center
        treeWidget               = GTK_WIDGET(newWidgetsTable());
        GtkWidget* currentWidget = GetVisualizationWidget(treeWidget);
        if (currentWidget != nullptr) { gtk_container_remove(GTK_CONTAINER(treeWidget), currentWidget); }

        if (widget->getType() == EVWidget::VerticalSplit || widget->getType() == EVWidget::HorizontalSplit) {
            if (gtk_widget_get_parent(treeWidget) != nullptr) { gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(treeWidget)), treeWidget); }

            //create a paned and insert it in table
            GtkWidget* paned = (widget->getType() == EVWidget::HorizontalSplit) ? gtk_hpaned_new() : gtk_vpaned_new();
            gtk_table_attach(GTK_TABLE(treeWidget), paned, 1, 2, 1, 2, GtkAttachOptions(GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                             GtkAttachOptions(GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
        }
        else //undefined or visualization box : visualization widget is a GtkButton (left : icon, right : label)
        {
            if (gtk_widget_get_parent(treeWidget) != nullptr) { gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(treeWidget)), treeWidget); }

            //create a button and insert it in table
            GtkWidget* button = gtk_button_new();
            gtk_widget_set_size_request(button, 0, 0);
            gtk_table_attach(GTK_TABLE(treeWidget), button, 1, 2, 1, 2, GtkAttachOptions(GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                             GtkAttachOptions(GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);

            //box inserted in button
            GtkBox* box = GTK_BOX(gtk_vbox_new(FALSE, 0));
            gtk_widget_set_size_request(GTK_WIDGET(box), 0, 0);

            //icon - actual icon will be loaded in EndLoadTreeWidget
            GtkWidget* icon = gtk_image_new_from_stock(GetTreeWidgetIcon(EVTNode::Undefined), GTK_ICON_SIZE_BUTTON);
            if (ICON_WIDTH_REQUEST == 0) { gtk_widget_set_size_request(icon, 0, 0); }
            gtk_box_pack_start(box, icon, ICON_EXPAND, ICON_FILL, 0);

            //label
            GtkWidget* label = gtk_label_new(static_cast<const char*>(widget->getName()));
            if (LABEL_WIDTH_REQUEST == 0) { gtk_widget_set_size_request(label, 0, 0); }
            gtk_box_pack_start(box, label, LABEL_EXPAND, LABEL_FILL, 0);

            //add box to button
            gtk_container_add(GTK_CONTAINER(button), GTK_WIDGET(box));

            //set up button as drag destination
            gtk_drag_dest_set(button, GTK_DEST_DEFAULT_ALL, TARGETS.data(), 2, GDK_ACTION_COPY);
            g_signal_connect(G_OBJECT(button), "drag-data-received", G_CALLBACK(dragDataReceivedInWidgetCB), this);

            //set up button as drag source as well
            gtk_drag_source_set(button, GDK_BUTTON1_MASK, TARGETS.data(), gint(TARGETS.size()), GDK_ACTION_COPY);
            g_signal_connect(G_OBJECT(button), "drag-data-get", G_CALLBACK(dragDataGetFromWidgetCB), this);

            //ask for notification of some events
            if (widget->getType() == EVWidget::Box) {
                GTK_WIDGET_SET_FLAGS(button, GDK_KEY_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
                g_signal_connect(G_OBJECT(button), "key-press-event", G_CALLBACK(widgetKeyPressEventCB), this);
                g_signal_connect(G_OBJECT(button), "enter-notify-event", G_CALLBACK(widgetEnterNotifyEventCB), this);
                g_signal_connect(G_OBJECT(button), "leave-notify-event", G_CALLBACK(widgetLeaveNotifyEventCB), this);
            }
        }

        //parent widget to its parent, if any
        //-----------------------------------
        IVWidget* parentWidget = m_tree.getVisualizationWidget(widget->getParentIdentifier());
        if (parentWidget != nullptr) //visualization boxes may be unparented
        {
            GtkTreeIter parentIter;
            m_tree.findChildNodeFromRoot(&parentIter, parentWidget->getIdentifier());

            if (parentWidget->getType() == EVWidget::Panel) {
                //parent widget to notebook as a new page
                void* notebook = nullptr;
                m_tree.getPointerValueFromTreeIter(&parentIter, notebook, EVTColumn::PointerWidget);
                char* panelName = nullptr;
                m_tree.getStringValueFromTreeIter(&parentIter, panelName, EVTColumn::StringName);
                gtk_notebook_append_page(GTK_NOTEBOOK(notebook), treeWidget, gtk_label_new(panelName));
            }
            else if (parentWidget->getType() == EVWidget::VerticalSplit || parentWidget->getType() == EVWidget::HorizontalSplit) {
                //insert widget in parent paned
                void* parentTreeWidget = nullptr;
                m_tree.getPointerValueFromTreeIter(&parentIter, parentTreeWidget, EVTColumn::PointerWidget);


                if (parentTreeWidget != nullptr && GTK_IS_WIDGET(parentTreeWidget)) {
                    GtkWidget* pParentWidget = GetVisualizationWidget(GTK_WIDGET(parentTreeWidget));
                    if (pParentWidget != nullptr && GTK_IS_PANED(pParentWidget)) {
                        size_t idx;
                        if (parentWidget->getChildIndex(widget->getIdentifier(), idx)) {
                            if (idx == 0) { gtk_paned_pack1(GTK_PANED(pParentWidget), treeWidget, TRUE, TRUE); }
                            else { gtk_paned_pack2(GTK_PANED(pParentWidget), treeWidget, TRUE, TRUE); }
                        }
                    }
                }
            }
        }
    }

    //resize widgets once they are allocated : this is the case when they are shown on an expose event
    //FIXME : perform resizing only once (when it is done as many times as there are widgets in the tree here)
    if (treeWidget != nullptr) { gtk_signal_connect(GTK_OBJECT(GetVisualizationWidget(treeWidget)), "expose-event", G_CALLBACK(WidgetExposeEventCB), this); }

    return treeWidget;
}

void CDesignerVisualization::EndLoadTreeWidget(IVWidget* widget)
{
    //retrieve tree widget
    GtkTreeIter it;
    m_tree.findChildNodeFromRoot(&it, widget->getIdentifier());
    void* treeWidget = nullptr;
    m_tree.getPointerValueFromTreeIter(&it, treeWidget, EVTColumn::PointerWidget);

    //get actual visualization widget
    GtkWidget* vizWidget = GetVisualizationWidget(static_cast<GtkWidget*>(treeWidget));

    if (widget->getType() == EVWidget::Panel) {
        //reposition paned widget handles
        ResizeCB(nullptr);
    }
    else if (widget->getType() == EVWidget::Undefined || widget->getType() == EVWidget::Box) {
        if (GTK_IS_BUTTON(vizWidget) != FALSE) {
            //replace dummy icon with correct one
            //-----------------------------------
            //retrieve icon name from tree
            char* iconString = nullptr;
            m_tree.getStringValueFromTreeIter(&it, iconString, EVTColumn::StringStockIcon);
            //retrieve hbox
            const GList* buttonChildren = gtk_container_get_children(GTK_CONTAINER(vizWidget));
            GtkContainer* box           = GTK_CONTAINER(buttonChildren->data);
            //remove first widget
            const GList* boxChildren = gtk_container_get_children(box);
            gtk_container_remove(box, GTK_WIDGET(boxChildren->data));
            //create new icon
            GtkWidget* icon = gtk_image_new_from_stock(iconString, GTK_ICON_SIZE_BUTTON);
            if (ICON_WIDTH_REQUEST == 0) { gtk_widget_set_size_request(icon, 0, 0); }
            gtk_box_pack_start(GTK_BOX(box), icon, ICON_EXPAND, ICON_FILL, 0);
            //insert it in first position
            gtk_box_reorder_child(GTK_BOX(box), icon, 0);
        }
    }
}

GtkWidget* CDesignerVisualization::GetTreeWidget(GtkWidget* widget) { return gtk_widget_get_parent(widget); }

GtkWidget* CDesignerVisualization::GetVisualizationWidget(GtkWidget* widget)
{
    if (GTK_IS_TABLE(widget)) { return getTableChild(GTK_TABLE(widget), 1, 2, 1, 2)->widget; }
    return widget;
}

const char* CDesignerVisualization::GetTreeWidgetIcon(const EVTNode type)
{
    switch (type) {
        case EVTNode::Unaffected: return GTK_STOCK_DIALOG_QUESTION;
        case EVTNode::Undefined: return GTK_STOCK_CANCEL;
        case EVTNode::VisualizationWindow: return GTK_STOCK_DND_MULTIPLE;
        case EVTNode::VisualizationPanel: return GTK_STOCK_DND;
        case EVTNode::VisualizationBox: return GTK_STOCK_EXECUTE; //default (actual icon name may be retrieved from box descriptor)
        case EVTNode::HorizontalSplit:
        case EVTNode::VerticalSplit: return GTK_STOCK_ADD;
        case EVTNode::VPU:
        default: return "";
    }
}

gboolean CDesignerVisualization::deleteEventCB(GtkWidget* /*widget*/, GdkEvent* /*event*/, gpointer data)
{
    return static_cast<CDesignerVisualization*>(data)->deleteEvent() ? TRUE : FALSE;
}

bool CDesignerVisualization::deleteEvent() const
{
    if (m_deleteEventCB != nullptr) {
        m_deleteEventCB(m_deleteEventUserData);
        return true;
    }

    return false;
}

#ifdef HANDLE_MIN_MAX_EVENTS
gboolean CDesignerVisualization::window_state_event_cb(GtkWidget * widget, GdkEventWindowState * event, gpointer data)
{
    //refresh widgets if window was maximized or minimized
    if (event->changed_mask& GDK_WINDOW_STATE_MAXIMIZED ||
        event->changed_mask& GDK_WINDOW_STATE_ICONIFIED)
    {
        //widgets haven't been reallocated yet, perform resizing only when this happens
        //gtk_signal_connect(GTK_OBJECT(gtk_paned_get_child2(GTK_PANED(m_pane))), "size-allocate", G_CALLBACK(widget_size_allocate_cb), this);
        gtk_signal_connect(GTK_OBJECT(gtk_paned_get_child2(GTK_PANED(m_pane))), "expose-event", G_CALLBACK(widget_expose_cb), this);
    }

    return FALSE;
}
#endif

//event generated whenever window size changes, including when it is first created
gboolean CDesignerVisualization::ConfigureEventCB(GtkWidget* /*widget*/, GdkEventConfigure* /*event*/, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->m_previewWindowVisible = true;
    static_cast<CDesignerVisualization*>(data)->ResizeCB(nullptr);
    return FALSE;
}

gboolean CDesignerVisualization::WidgetExposeEventCB(GtkWidget* widget, GdkEventExpose* /*event*/, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->m_previewWindowVisible = true;
    g_signal_handlers_disconnect_by_func(G_OBJECT(widget), G_CALLBACK2(CDesignerVisualization::WidgetExposeEventCB), data);
    static_cast<CDesignerVisualization*>(data)->ResizeCB(nullptr);
    return FALSE;
}

void CDesignerVisualization::ResizeCB(IVWidget* widget)
{
    if (widget == nullptr) {
        //assign current window size to each window
        GtkWidget* notebook = gtk_paned_get_child2(GTK_PANED(m_pane));
        if (notebook != nullptr) {
            CIdentifier id = CIdentifier::undefined();
            //retrieve current preview window size, if window is visible
            if (m_previewWindowVisible) {
                notebook = gtk_paned_get_child2(GTK_PANED(m_pane));
                if (notebook != nullptr) {
                    //update preview window dims
                    m_previewWindowW = notebook->allocation.width;
                    m_previewWindowH = notebook->allocation.height;
                }
            }

            while (m_tree.getNextVisualizationWidgetIdentifier(id, EVWidget::Window)) {
                IVWidget* window = m_tree.getVisualizationWidget(id);

                //store new dimensions
                window->setWidth(m_previewWindowW);
                window->setHeight(m_previewWindowH);
            }
        }
        // else { return;}   //? 

        //retrieve active visualization panel
        GtkTreeIter windowIt;
        if (!m_tree.findChildNodeFromRoot(&windowIt, m_activeVisualizationWindowName, EVTNode::VisualizationWindow)) { return; }
        GtkTreeIter panelIt = windowIt;
        if (!m_tree.findChildNodeFromParent(&panelIt, m_activeVisualizationPanelName, EVTNode::VisualizationPanel)) { return; }
        CIdentifier panelID;
        if (!m_tree.getIdentifierFromTreeIter(&panelIt, panelID, EVTColumn::StringIdentifier)) { return; }
        IVWidget* panel = m_tree.getVisualizationWidget(panelID);

        //resize visualization panel hierarchy
        if (panel != nullptr) {
            CIdentifier childID;
            panel->getChildIdentifier(0, childID);
            IVWidget* childWidget = m_tree.getVisualizationWidget(childID);
            if (childWidget != nullptr) { ResizeCB(childWidget); }
        }
    }
    else if (widget->getType() == EVWidget::VerticalSplit || widget->getType() == EVWidget::HorizontalSplit) {
        GtkTreeIter it;
        if (m_tree.findChildNodeFromRoot(&it, widget->getIdentifier()) == TRUE) {
            //retrieve paned widget
            void* treeWidget = nullptr;
            m_tree.getPointerValueFromTreeIter(&it, treeWidget, EVTColumn::PointerWidget);
            GtkWidget* paned = GetVisualizationWidget(GTK_WIDGET(treeWidget));
            enablePanedSignals(paned, false);

            //retrieve paned attributes
            const int handlePos    = widget->getDividerPosition();
            const int maxHandlePos = widget->getMaxDividerPosition();

            if (handlePos == std::numeric_limits<int>::min() || maxHandlePos == std::numeric_limits<int>::min()) {
                // these variables hadn't been initialized meaningfully before. @fixme what is the correct place to init them?
                notifyPositionPaned(paned); // for now, this inits them as a side effect
            }
            if (maxHandlePos > 0) {
                //retrieve current maximum handle position
                const int pos = GTK_IS_VPANED(paned) ? GTK_PANED(paned)->container.widget.allocation.height
                                    : GTK_PANED(paned)->container.widget.allocation.width;

                //set new paned handle position
                gtk_paned_set_position(GTK_PANED(paned), handlePos * pos / maxHandlePos);
            }

            enablePanedSignals(paned, true);

            //go down child 1
            CIdentifier childID;
            widget->getChildIdentifier(0, childID);
            IVWidget* childWidget = m_tree.getVisualizationWidget(childID);
            if (childWidget != nullptr) { ResizeCB(childWidget); }

            //go down child 2
            widget->getChildIdentifier(1, childID);
            childWidget = m_tree.getVisualizationWidget(childID);
            if (childWidget != nullptr) { ResizeCB(childWidget); }
        }
    }
}

void CDesignerVisualization::NotebookPageSwitchCB(GtkNotebook* notebook, GtkNotebookPage* /*page*/, const guint pagenum, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->notebookPageSelectedCB(notebook, pagenum);
}

gboolean CDesignerVisualization::NotifyPositionPanedCB(GtkWidget* widget, GParamSpec* /*spec*/, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->notifyPositionPaned(widget);
    return TRUE;
}

//--------------------------
//Event box table management
//--------------------------

void CDesignerVisualization::setupNewEventBoxTable(GtkBuilder* xml)
{
    //set up event boxes as drag targets
    gtk_drag_dest_set(GTK_WIDGET(gtk_builder_get_object(xml, "window_manager_eventbox-eventbox2")),
                      GTK_DEST_DEFAULT_ALL, TARGETS.data(), gint(TARGETS.size()), GDK_ACTION_COPY);
    gtk_drag_dest_set(GTK_WIDGET(gtk_builder_get_object(xml, "window_manager_eventbox-eventbox4")),
                      GTK_DEST_DEFAULT_ALL, TARGETS.data(), gint(TARGETS.size()), GDK_ACTION_COPY);
    gtk_drag_dest_set(GTK_WIDGET(gtk_builder_get_object(xml, "window_manager_eventbox-eventbox6")),
                      GTK_DEST_DEFAULT_ALL, TARGETS.data(), gint(TARGETS.size()), GDK_ACTION_COPY);
    gtk_drag_dest_set(GTK_WIDGET(gtk_builder_get_object(xml, "window_manager_eventbox-eventbox8")),
                      GTK_DEST_DEFAULT_ALL, TARGETS.data(), gint(TARGETS.size()), GDK_ACTION_COPY);

    //set up event boxes callbacks for drag data received events
    char buf[256];
    sprintf(buf, "%p %s", this, "top");
    m_topEventBoxData = buf;
    g_signal_connect(G_OBJECT(gtk_builder_get_object(xml, "window_manager_eventbox-eventbox2")), "drag-data-received",
                     G_CALLBACK(dataReceivedInEventBoxCB), gpointer(m_topEventBoxData.c_str()));
    sprintf(buf, "%p %s", this, "left");
    m_leftEventBoxData = buf;
    g_signal_connect(G_OBJECT(gtk_builder_get_object(xml, "window_manager_eventbox-eventbox4")), "drag-data-received",
                     G_CALLBACK(dataReceivedInEventBoxCB), gpointer(m_leftEventBoxData.c_str()));
    sprintf(buf, "%p %s", this, "right");
    m_rightEventBoxData = buf;
    g_signal_connect(G_OBJECT(gtk_builder_get_object(xml, "window_manager_eventbox-eventbox6")), "drag-data-received",
                     G_CALLBACK(dataReceivedInEventBoxCB), gpointer(m_rightEventBoxData.c_str()));
    sprintf(buf, "%p %s", this, "bottom");
    m_bottomEventBoxData = buf;
    g_signal_connect(G_OBJECT(gtk_builder_get_object(xml, "window_manager_eventbox-eventbox8")), "drag-data-received",
                     G_CALLBACK(dataReceivedInEventBoxCB), gpointer(m_bottomEventBoxData.c_str()));
}

void CDesignerVisualization::refreshActiveVisualization(GtkTreePath* selectedItemPath)
{
    //show tree
    gtk_tree_view_expand_all(m_treeView);

    //select item
    if (selectedItemPath != nullptr) { gtk_tree_view_set_cursor(m_treeView, selectedItemPath, nullptr, false); }
    //select previous visualization tab again (or another tab if it doesn't exist anymore)
    else { setActiveVisualization(m_activeVisualizationWindowName, m_activeVisualizationPanelName); }
}

void CDesignerVisualization::setActiveVisualization(const char* activeWindow, const char* activePanel)
{
    //clear active window/panel names
    m_activeVisualizationWindowName = "";
    m_activeVisualizationPanelName  = "";

    //retrieve active window
    GtkTreeIter windowIter;

    if (m_tree.findChildNodeFromRoot(&windowIter, activeWindow, EVTNode::VisualizationWindow)) { m_activeVisualizationWindowName = CString(activeWindow); }
    else {
        //pick first window if previously active window doesn't exist anymore
        CIdentifier id = CIdentifier::undefined();

        if (m_tree.getNextVisualizationWidgetIdentifier(id, EVWidget::Window)) {
            m_activeVisualizationWindowName = m_tree.getVisualizationWidget(id)->getName();
            m_tree.findChildNodeFromRoot(&windowIter, m_activeVisualizationWindowName.toASCIIString(), EVTNode::VisualizationWindow);
        }
        else //no windows left
        {
            if (gtk_paned_get_child2(GTK_PANED(m_pane)) != nullptr) { gtk_container_remove(GTK_CONTAINER(m_pane), gtk_paned_get_child2(GTK_PANED(m_pane))); }
            return;
        }
    }

    //retrieve active panel
    GtkTreeIter panelIter = windowIter;
    if (m_tree.findChildNodeFromParent(&panelIter, activePanel, EVTNode::VisualizationPanel)) { m_activeVisualizationPanelName = CString(activePanel); }
    else //couldn't find panel : select first one
    {
        CIdentifier windowID;
        m_tree.getIdentifierFromTreeIter(&windowIter, windowID, EVTColumn::StringIdentifier);
        const IVWidget* window = m_tree.getVisualizationWidget(windowID);
        CIdentifier panelID;
        if (window->getChildIdentifier(0, panelID)) {
            panelIter = windowIter;
            m_tree.findChildNodeFromParent(&panelIter, panelID);
            char* str = nullptr;
            m_tree.getStringValueFromTreeIter(&panelIter, str, EVTColumn::StringName);
            m_activeVisualizationPanelName = str;
        }
        else //no panel in window
        {
            GtkWidget* currentNotebook = gtk_paned_get_child2(GTK_PANED(m_pane));
            if (currentNotebook != nullptr) {
                gtk_object_ref(GTK_OBJECT(currentNotebook));
                gtk_container_remove(GTK_CONTAINER(m_pane), currentNotebook);
            }
            return;
        }
    }

    //retrieve notebook and set it visible
    void* notebook = nullptr;
    m_tree.getPointerValueFromTreeIter(&panelIter, notebook, EVTColumn::PointerWidget);
    GtkWidget* widget = gtk_paned_get_child2(GTK_PANED(m_pane));
    if (widget != GTK_WIDGET(notebook)) {
        if (widget != nullptr) {
            //FIXME : don't ref previous notebook if parent window doesn't exist anymore
            gtk_object_ref(GTK_OBJECT(widget));
            gtk_container_remove(GTK_CONTAINER(m_pane), widget);
        }
        gtk_paned_add2(GTK_PANED(m_pane), GTK_WIDGET(notebook));
        //gtk_object_unref(currentNotebook);
    }

    //disable switch page notifications
    enableNotebookSignals(GTK_WIDGET(notebook), false);

    //set active panel visible
    int i;
    for (i = 0; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)); ++i) {
        if (strcmp(gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i)),
                   m_activeVisualizationPanelName) == 0) {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), i);
            break;
        }
    }

    //if active page couldn't be found
    if (i == gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook))) {
        //error!
        //pick first page if it exists
        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 0) {
            m_activeVisualizationPanelName = gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 0));
            gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
        }
        else { m_activeVisualizationPanelName = ""; } //error : no pages in notebook, clear panel name
    }

    //enable switch page notifications
    enableNotebookSignals(GTK_WIDGET(notebook), true);

    //refresh display
    gtk_widget_show_all(m_pane);
}

//creates a new widgets table and sets it as current
GtkTable* CDesignerVisualization::newWidgetsTable()
{
    //@FIXME is the memory ever freed? Valgrind is suspicious about this. It seems that a builder is allocated, but only a member of builder is returned as GtkTable*.
    GtkBuilder* pGtkBuilderTable = gtk_builder_new(); // glade_xml_new(m_guiFile.c_str(), "window_manager_eventbox-table", nullptr);
    gtk_builder_add_from_file(pGtkBuilderTable, m_guiFile.c_str(), nullptr);
    gtk_builder_connect_signals(pGtkBuilderTable, nullptr);

    //set up event boxes
    setupNewEventBoxTable(pGtkBuilderTable);

    GtkTable* table = GTK_TABLE(gtk_builder_get_object(pGtkBuilderTable, "window_manager_eventbox-table"));

    //clear central button label
    const GtkTableChild* tc = getTableChild(table, 1, 2, 1, 2);
    GtkButton* button       = GTK_BUTTON(tc->widget);
    gtk_button_set_label(button, "");

    //set it up as drag destination
    gtk_drag_dest_set(GTK_WIDGET(button), GTK_DEST_DEFAULT_ALL, TARGETS.data(), gint(TARGETS.size()), GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(button), "drag-data-received", G_CALLBACK(dragDataReceivedInWidgetCB), this);

    //set it up as drag source as well
    gtk_drag_source_set(GTK_WIDGET(button), GDK_BUTTON1_MASK, TARGETS.data(), 2, GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(button), "drag-data-get", G_CALLBACK(dragDataGetFromWidgetCB), this);

    return table;
}

void CDesignerVisualization::askNewVisualizationWindow()
{
    //show dialog
    CInputDialog id(m_guiFile.c_str(), &CDesignerVisualization::NewVisualizationWindowCB, this, "New window", "Please enter name of new window : ");
    id.Run();
}

bool CDesignerVisualization::NewVisualizationWindow(const char* label)
{
    //ensure name is unique
    IVWidget* window;
    CIdentifier windowID = CIdentifier::undefined();

    while (m_tree.getNextVisualizationWidgetIdentifier(windowID, EVWidget::Window)) {
        window = m_tree.getVisualizationWidget(windowID);

        if (strcmp(window->getName().toASCIIString(), label) == 0) {
            displayErrorDialog("Window creation failed !", "An existing window already uses this name. Please choose another name.");
            return false;
        }
    }

    //proceed with window creation
    //m_visualizationTree.addVisualizationWindow(windowID, CString(label));
    m_tree.addVisualizationWidget(windowID, CString(label), EVWidget::Window, CIdentifier::undefined(),
                                  0, CIdentifier::undefined(), 0, CIdentifier::undefined());

    window = m_tree.getVisualizationWidget(windowID);

    //add attributes
    window->setWidth(1);
    window->setHeight(1);

    //create default visualization panel as well
    CIdentifier childID;
    const CString childName = "Default tab";

    m_tree.addVisualizationWidget(childID, childName, EVWidget::Panel, windowID, 0, CIdentifier::undefined(), 1, CIdentifier::undefined());

    m_tree.reloadTree();

    //refresh view
    GtkTreeIter childIter;
    m_tree.findChildNodeFromRoot(&childIter, childID);
    refreshActiveVisualization(m_tree.getTreePath(&childIter));

    return true;
}

void CDesignerVisualization::askRenameVisualizationWindow()
{
    //show dialog
    CInputDialog id(m_guiFile.c_str(), &CDesignerVisualization::RenameVisualizationWindowCB, this, "Rename window", "Please enter new name of window : ");

    id.Run();
}

bool CDesignerVisualization::renameVisualizationWindow(const char* label)
{
    //retrieve visualization window
    GtkTreeIter iter;
    if (!m_tree.findChildNodeFromRoot(&iter, m_activeVisualizationWindowName, EVTNode::VisualizationWindow)) {
        displayErrorDialog("Window renaming failed !", "Couldn't retrieve window.");
        return false;
    }

    CIdentifier windowID;
    m_tree.getIdentifierFromTreeIter(&iter, windowID, EVTColumn::StringIdentifier);
    IVWidget* window = m_tree.getVisualizationWidget(windowID);
    if (window == nullptr) {
        displayErrorDialog("Window renaming failed !", "Couldn't retrieve window.");
        return false;
    }

    //if trying to set identical name, return
    const CString newName = label;
    if (window->getName() == newName) { return true; }

    //ensure name is unique
    CIdentifier id = CIdentifier::undefined();
    while (m_tree.getNextVisualizationWidgetIdentifier(id, EVWidget::Window)) {
        //name already in use : warn user
        if (m_tree.getVisualizationWidget(id)->getName() == newName) {
            displayErrorDialog("Window renaming failed !", "An existing window already uses this name. Please choose another name.");
            return false;
        }
    }

    //change its name
    window->setName(newName);

    m_tree.reloadTree();

    //refresh view
    m_tree.findChildNodeFromRoot(&iter, windowID);
    refreshActiveVisualization(m_tree.getTreePath(&iter));

    return true;
}

bool CDesignerVisualization::removeVisualizationWindow()
{
    //retrieve visualization window
    CIdentifier windowID = CIdentifier::undefined();
    while (m_tree.getNextVisualizationWidgetIdentifier(windowID, EVWidget::Window)) {
        if (m_tree.getVisualizationWidget(windowID)->getName() == m_activeVisualizationWindowName) { break; }
    }

    //return if window was not found
    if (windowID == CIdentifier::undefined()) {
        displayErrorDialog("Window removal failed !", "Couldn't retrieve window.");
        return false;
    }

    //destroy hierarchy but only unaffect visualization boxes
    m_tree.destroyHierarchy(windowID, false);

    m_tree.reloadTree();

    //refresh view
    refreshActiveVisualization(nullptr);

    return true;
}

void CDesignerVisualization::askNewVisualizationPanel()
{
    //show dialog
    CInputDialog id(m_guiFile.c_str(), &CDesignerVisualization::NewVisualizationPanelCB, this, "New tab", "Please enter name of new tab : ");

    id.Run();
}

bool CDesignerVisualization::newVisualizationPanel(const char* label)
{
    //retrieve visualization window
    IVWidget* window     = nullptr;
    CIdentifier windowID = CIdentifier::undefined();

    while (m_tree.getNextVisualizationWidgetIdentifier(windowID, EVWidget::Window)) {
        window = m_tree.getVisualizationWidget(windowID);
        if (window->getName() == m_activeVisualizationWindowName) { break; }
    }

    //return if parent window was not found
    if (windowID == CIdentifier::undefined() || window == nullptr) {
        displayErrorDialog("Tab creation failed !", "Couldn't retrieve parent window.");
        return false;
    }

    CIdentifier childID;
    const CString newName = label;

    //ensure visualization panel name is unique in this window
    for (size_t i = 0; i < window->getNbChildren(); ++i) {
        window->getChildIdentifier(i, childID);
        if (m_tree.getVisualizationWidget(childID)->getName() == newName) {
            displayErrorDialog("Tab creation failed !", "An existing tab already uses this name. Please choose another name.");
            return false;
        }
    }

    //proceed with panel creation
    m_tree.addVisualizationWidget(childID, newName, EVWidget::Panel, windowID, window->getNbChildren(), CIdentifier::undefined(), 1, CIdentifier::undefined());

    m_tree.reloadTree();

    //refresh view
    GtkTreeIter iter;
    m_tree.findChildNodeFromRoot(&iter, childID);
    refreshActiveVisualization(m_tree.getTreePath(&iter));

    return true;
}

void CDesignerVisualization::askRenameVisualizationPanel()
{
    //show dialog
    CInputDialog id(m_guiFile.c_str(), &CDesignerVisualization::RenameVisualizationPanelCB, this, "Rename tab", "Please enter new name of tab : ");

    id.Run();
}

bool CDesignerVisualization::renameVisualizationPanel(const char* label)
{
    //retrieve visualization window
    GtkTreeIter iter;
    if (!m_tree.findChildNodeFromRoot(&iter, m_activeVisualizationWindowName.toASCIIString(), EVTNode::VisualizationWindow)) {
        displayErrorDialog("Tab renaming failed !", "Couldn't retrieve parent window.");
        return false;
    }
    CIdentifier windowID;
    m_tree.getIdentifierFromTreeIter(&iter, windowID, EVTColumn::StringIdentifier);
    IVWidget* window = m_tree.getVisualizationWidget(windowID);
    if (window == nullptr) {
        displayErrorDialog("Tab renaming failed !", "Couldn't retrieve parent window.");
        return false;
    }

    //retrieve visualization panel
    if (!m_tree.findChildNodeFromParent(&iter, m_activeVisualizationPanelName.toASCIIString(), EVTNode::VisualizationPanel)) {
        displayErrorDialog("Tab renaming failed !", "Couldn't retrieve tab.");
        return false;
    }

    CIdentifier panelID;
    m_tree.getIdentifierFromTreeIter(&iter, panelID, EVTColumn::StringIdentifier);
    IVWidget* widget = m_tree.getVisualizationWidget(panelID);
    if (widget == nullptr) {
        displayErrorDialog("tab renaming failed !", "Couldn't retrieve tab.");
        return false;
    }

    //if trying to set identical name, return
    const CString newName = label;
    if (widget->getName() == newName) { return true; }

    //ensure visualization panel name is unique in this window
    CIdentifier childID;
    for (size_t i = 0; i < window->getNbChildren(); ++i) {
        window->getChildIdentifier(i, childID);
        if (m_tree.getVisualizationWidget(childID)->getName() == newName) {
            displayErrorDialog("Tab renaming failed !", "An existing tab already uses this name. Please choose another name.");
            return false;
        }
    }

    widget->setName(newName);

    m_tree.reloadTree();

    //refresh view
    m_tree.findChildNodeFromRoot(&iter, panelID);
    refreshActiveVisualization(m_tree.getTreePath(&iter));

    return true;
}

bool CDesignerVisualization::removeVisualizationPanel()
{
    //retrieve visualization window
    GtkTreeIter iter;
    m_tree.findChildNodeFromRoot(&iter, m_activeVisualizationWindowName.toASCIIString(), EVTNode::VisualizationWindow);

    //retrieve visualization panel
    m_tree.findChildNodeFromParent(&iter, m_activeVisualizationPanelName.toASCIIString(), EVTNode::VisualizationPanel);
    CIdentifier panelID;
    m_tree.getIdentifierFromTreeIter(&iter, panelID, EVTColumn::StringIdentifier);

    //destroy hierarchy but only unaffect visualization boxes (as opposed to destroying them)
    if (!m_tree.destroyHierarchy(m_tree.getVisualizationWidget(panelID)->getIdentifier(), false)) {
        displayErrorDialog("Tab removal failed !", "An error occurred while destroying widget hierarchy.");
        return false;
    }

    m_tree.reloadTree();

    //refresh view
    refreshActiveVisualization(nullptr);

    return true;
}

bool CDesignerVisualization::removeVisualizationWidget()
{
    //retrieve widget
    GtkTreeIter iter;
    if (!m_tree.getTreeSelection(m_treeView, &iter)) { return false; }
    CIdentifier id;
    m_tree.getIdentifierFromTreeIter(&iter, id, EVTColumn::StringIdentifier);
    return removeVisualizationWidget(id);
}

//TODO : move this to CVisualizationTree?
bool CDesignerVisualization::removeVisualizationWidget(const CIdentifier& identifier)
{
    IVWidget* widget = m_tree.getVisualizationWidget(identifier);
    if (widget == nullptr) { return false; }

    const IVWidget* parentWidget = m_tree.getVisualizationWidget(widget->getParentIdentifier());

    //unparent or destroy widget
    size_t idx;
    m_tree.unparentVisualizationWidget(identifier, idx);
    if (widget->getType() != EVWidget::Box) { m_tree.destroyHierarchy(identifier, false); }

    //reparent other child widget, if any
    if (parentWidget->getType() != EVWidget::Panel) {
        //retrieve parent's other widget
        CIdentifier otherWidgetID;
        parentWidget->getChildIdentifier(1 - idx, otherWidgetID);

        //unparent parent
        size_t parentIdx;
        const CIdentifier parentID = parentWidget->getParentIdentifier();
        m_tree.unparentVisualizationWidget(parentWidget->getIdentifier(), parentIdx);

        //reparent other widget to its grandparent
        m_tree.unparentVisualizationWidget(otherWidgetID, idx);
        m_tree.parentVisualizationWidget(otherWidgetID, parentID, parentIdx);

        //destroy parent
        m_tree.destroyHierarchy(parentWidget->getIdentifier(), false);
    }

    m_tree.reloadTree();

    //refresh view
    refreshActiveVisualization(nullptr);

    return true;
}

bool CDesignerVisualization::destroyVisualizationWidget(const CIdentifier& identifier)
{
    const bool b = removeVisualizationWidget(identifier);
    m_tree.destroyHierarchy(identifier, true);
    return b;
}

//CALLBACKS
//---------

void CDesignerVisualization::notebookPageSelectedCB(GtkNotebook* notebook, const guint pagenum)
{
    GtkTreeIter iter;
    m_tree.findChildNodeFromRoot(&iter, static_cast<void*>(notebook));
    CIdentifier id;
    m_tree.getIdentifierFromTreeIter(&iter, id, EVTColumn::StringIdentifier);
    IVWidget* widget = m_tree.getVisualizationWidget(id);
    if (widget != nullptr) {
        IVWidget* window = m_tree.getVisualizationWidget(widget->getParentIdentifier());
        if (window != nullptr) {
            window->getChildIdentifier(pagenum, id);
            if (m_tree.findChildNodeFromRoot(&iter, id)) { refreshActiveVisualization(m_tree.getTreePath(&iter)); }
        }
    }
}

void CDesignerVisualization::enableNotebookSignals(GtkWidget* notebook, const bool b)
{
    if (b) { g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(NotebookPageSwitchCB), this); }
    else { g_signal_handlers_disconnect_by_func(G_OBJECT(notebook), G_CALLBACK2(NotebookPageSwitchCB), this); }
}

void CDesignerVisualization::notifyPositionPaned(GtkWidget* widget)
{
    GtkPaned* paned = GTK_PANED(widget);

    //return if handle pos was changed because parent window was resized
    const int pos             = gtk_paned_get_position(paned);
    const int maxPos          = GTK_IS_VPANED(paned) ? paned->container.widget.allocation.height : paned->container.widget.allocation.width;
    const int handleThickness = GTK_IS_VPANED(paned) ? paned->handle_pos.height : paned->handle_pos.width;

    if (pos + handleThickness == maxPos) { return; }

    //look for widget in tree
    GtkWidget* treeWidget = GetTreeWidget(widget);
    GtkTreeIter iter;
    if (m_tree.findChildNodeFromRoot(&iter, treeWidget)) {
        CIdentifier id;
        m_tree.getIdentifierFromTreeIter(&iter, id, EVTColumn::StringIdentifier);

        //store new position and max position
        auto* visualizationWidget = m_tree.getVisualizationWidget(id);
        visualizationWidget->setDividerPosition(pos);
        visualizationWidget->setMaxDividerPosition(maxPos);
    }
}

void CDesignerVisualization::enablePanedSignals(GtkWidget* paned, const bool b)
{
    if (b) { g_signal_connect(G_OBJECT(paned), "notify::position", G_CALLBACK(NotifyPositionPanedCB), this); }
    else { g_signal_handlers_disconnect_by_func(G_OBJECT(paned), G_CALLBACK2(NotifyPositionPanedCB), this); }
}

void CDesignerVisualization::AskNewVisualizationWindowCB(gpointer data, guint /*callback_action*/, GtkWidget* /*widget*/)
{
    static_cast<CDesignerVisualization*>(data)->askNewVisualizationWindow();
}

void CDesignerVisualization::NewVisualizationWindowCB(GtkWidget* /*widget*/, gpointer data)
{
    const CInputDialog* inputDialog = static_cast<CInputDialog*>(data);

    if (inputDialog->GetUserData() != nullptr) {
        static_cast<CDesignerVisualization*>(inputDialog->GetUserData())->NewVisualizationWindow(inputDialog->GetEntry());
    }
}

void CDesignerVisualization::AskRenameVisualizationWindowCB(gpointer data, guint /*callback_action*/, GtkWidget* /*widget*/)
{
    static_cast<CDesignerVisualization*>(data)->askRenameVisualizationWindow();
}

void CDesignerVisualization::RenameVisualizationWindowCB(GtkWidget* /*widget*/, gpointer data)
{
    const CInputDialog* inputDialog = static_cast<CInputDialog*>(data);

    if (inputDialog->GetUserData() != nullptr) {
        static_cast<CDesignerVisualization*>(inputDialog->GetUserData())->renameVisualizationWindow(inputDialog->GetEntry());
    }
}

void CDesignerVisualization::RemoveVisualizationWindowCB(gpointer data, guint /*action*/, GtkWidget* /*widget*/)
{
    static_cast<CDesignerVisualization*>(data)->removeVisualizationWindow();
}

void CDesignerVisualization::AskNewVisualizationPanelCB(gpointer data, guint /*action*/, GtkWidget* /*widget*/)
{
    static_cast<CDesignerVisualization*>(data)->askNewVisualizationPanel();
}

void CDesignerVisualization::NewVisualizationPanelCB(GtkWidget* /*widget*/, gpointer data)
{
    const auto* inputDialog = static_cast<CInputDialog*>(data);

    if (inputDialog->GetUserData() != nullptr) {
        static_cast<CDesignerVisualization*>(inputDialog->GetUserData())->newVisualizationPanel(inputDialog->GetEntry());
    }
}

void CDesignerVisualization::AskRenameVisualizationPanelCB(gpointer data, guint /*action*/, GtkWidget* /*widget*/)
{
    static_cast<CDesignerVisualization*>(data)->askRenameVisualizationPanel();
}

void CDesignerVisualization::RenameVisualizationPanelCB(GtkWidget* /*widget*/, gpointer data)
{
    const auto* inputDialog = static_cast<CInputDialog*>(data);

    if (inputDialog->GetUserData() != nullptr) {
        static_cast<CDesignerVisualization*>(inputDialog->GetUserData())->renameVisualizationPanel(inputDialog->GetEntry());
    }
}

void CDesignerVisualization::RemoveVisualizationPanelCB(gpointer data, guint /*action*/, GtkWidget* /*widget*/)
{
    static_cast<CDesignerVisualization*>(data)->removeVisualizationPanel();
}

void CDesignerVisualization::RemoveVisualizationWidgetCB(gpointer data, guint /*action*/, GtkWidget* /*widget*/)
{
    static_cast<CDesignerVisualization*>(data)->removeVisualizationWidget();
}

void CDesignerVisualization::widgetKeyPressEventCB(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->widgetKeyPressEvent(widget, event);
}

void CDesignerVisualization::widgetKeyPressEvent(GtkWidget* /*widget*/, GdkEventKey* event)
{
    //remove widget
    if (event->keyval == GDK_Delete || event->keyval == GDK_KP_Delete) {
        if (m_highlightedWidget != nullptr) {
            GtkTreeIter iter;
            if (m_tree.findChildNodeFromRoot(&iter, GetTreeWidget(m_highlightedWidget))) {
                CIdentifier id;
                m_tree.getIdentifierFromTreeIter(&iter, id, EVTColumn::StringIdentifier);
                removeVisualizationWidget(id);
            }
        }
    }
}

gboolean CDesignerVisualization::widgetEnterNotifyEventCB(GtkWidget* widget, GdkEventCrossing* event, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->widgetEnterNotifyEvent(widget, event);
    return FALSE;
}

void CDesignerVisualization::widgetEnterNotifyEvent(GtkWidget* widget, GdkEventCrossing* /*event*/) { m_highlightedWidget = widget; }

gboolean CDesignerVisualization::widgetLeaveNotifyEventCB(GtkWidget* widget, GdkEventCrossing* event, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->widgetLeaveNotifyEvent(widget, event);
    return FALSE;
}

void CDesignerVisualization::widgetLeaveNotifyEvent(GtkWidget* /*widget*/, GdkEventCrossing* /*event*/) { m_highlightedWidget = nullptr; }

gboolean CDesignerVisualization::buttonReleaseCB(GtkWidget* widget, GdkEventButton* event, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->buttonRelease(widget, event);
    return FALSE;
}

void CDesignerVisualization::buttonRelease(GtkWidget* widget, GdkEventButton* event) const
{
    if (GTK_IS_TREE_VIEW(widget)) {
        if (event->button == 3) //right button
        {
            if (event->type != GDK_BUTTON_PRESS) {
                GtkTreeIter it;

                if (!m_tree.getTreeSelection(m_treeView, &it)) { return; }

                const EVTNode type = EVTNode(m_tree.getULongValueFromTreeIter(&it, EVTColumn::ULongNodeType));

                if (type == EVTNode::Unaffected) {
                    gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget(m_unaffectedItemFactory, "<unaffected_main>")),
                                   nullptr, nullptr, nullptr, nullptr, event->button, event->time);
                }
                else if (type == EVTNode::VisualizationWindow) {
                    gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget(m_visualizationWindowItemFactory, "<visualization_window_main>")),
                                   nullptr, nullptr, nullptr, nullptr, event->button, event->time);
                }
                else if (type == EVTNode::VisualizationPanel) {
                    gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget(m_visualizationPanelItemFactory, "<visualization_panel_main>")),
                                   nullptr, nullptr, nullptr, nullptr, event->button, event->time);
                }
                else if (type == EVTNode::HorizontalSplit || type == EVTNode::VerticalSplit) {
                    gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget(m_splitItemFactory, "<split_widget_main>")),
                                   nullptr, nullptr, nullptr, nullptr, event->button, event->time);
                }
                else if (type == EVTNode::VisualizationBox) {
                    //ensure visualization box is parented to a tab
                    if (m_tree.findParentNode(&it, EVTNode::VisualizationPanel)) {
                        gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget(m_visualizationBoxItemFactory, "<visualization_box_main>")),
                                       nullptr, nullptr, nullptr, nullptr, event->button, event->time);
                    }
                }
                else if (type == EVTNode::Undefined) {
                    //ensure empty plugin is not parented to a panel (because an empty widget is always present in an empty panel)
                    CIdentifier id;
                    m_tree.getIdentifierFromTreeIter(&it, id, EVTColumn::StringIdentifier);
                    IVWidget* visuWidget = m_tree.getVisualizationWidget(id);
                    if (visuWidget != nullptr) {
                        IVWidget* parentVisuWidget = m_tree.getVisualizationWidget(visuWidget->getParentIdentifier());
                        if (parentVisuWidget != nullptr) {
                            if (parentVisuWidget->getType() != EVWidget::Panel) {
                                gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget(m_undefinedItemFactory, "<undefined_widget_main>")),
                                               nullptr, nullptr, nullptr, nullptr, event->button, event->time);
                            }
                        }
                    }
                }
            }
        }
    }
}

void CDesignerVisualization::cursorChangedCB(GtkTreeView* treeView, gpointer data) { static_cast<CDesignerVisualization*>(data)->cursorChanged(treeView); }

void CDesignerVisualization::cursorChanged(GtkTreeView* treeView)
{
    //retrieve selection
    GtkTreeIter selectionIt;
    if (!m_tree.getTreeSelection(treeView, &selectionIt)) { return; }

    //save active item
    if (m_tree.getULongValueFromTreeIter(&selectionIt, EVTColumn::ULongNodeType) == size_t(EVTNode::VisualizationBox)) {
        m_tree.getIdentifierFromTreeIter(&selectionIt, m_activeVisualizationBoxID, EVTColumn::StringIdentifier);
    }

    GtkTreeIter panelIt = selectionIt;

    //if selection lies in a visualization panel subtree, display this subtree
    if (m_tree.findParentNode(&panelIt, EVTNode::VisualizationPanel)) {
        //get visualization panel name
        char* panelName = nullptr;
        m_tree.getStringValueFromTreeIter(&panelIt, panelName, EVTColumn::StringName);

        //retrieve visualization window that contains selection
        GtkTreeIter windowIt = panelIt;
        if (m_tree.findParentNode(&windowIt, EVTNode::VisualizationWindow)) {
            //get its name
            char* windowName = nullptr;
            m_tree.getStringValueFromTreeIter(&windowIt, windowName, EVTColumn::StringName);

            //set active visualization
            setActiveVisualization(windowName, panelName);
        }
    }
    else {
        GtkTreeIter windowIt = selectionIt;

        //if selection is a visualization window, display it
        if (m_tree.findParentNode(&windowIt, EVTNode::VisualizationWindow)) {
            //retrieve visualization window
            CIdentifier windowID;
            m_tree.getIdentifierFromTreeIter(&windowIt, windowID, EVTColumn::StringIdentifier);
            const IVWidget* window = m_tree.getVisualizationWidget(windowID);

            //if window has at least one panel
            if (window->getNbChildren() > 0) {
                //retrieve first panel
                CIdentifier panelID;
                window->getChildIdentifier(0, panelID);
                m_tree.findChildNodeFromParent(&panelIt, panelID);

                //retrieve notebook
                void* notebook = nullptr;
                m_tree.getPointerValueFromTreeIter(&panelIt, notebook, EVTColumn::PointerWidget);

                //get label of its active tab
                GtkWidget* pageLabel = gtk_notebook_get_tab_label(
                    GTK_NOTEBOOK(notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))));

                //set active visualization
                if (pageLabel != nullptr) { setActiveVisualization(window->getName().toASCIIString(), gtk_label_get_text(GTK_LABEL(pageLabel))); }
                else { setActiveVisualization(window->getName().toASCIIString(), nullptr); }
            }
            else //window has no panels
            {
                setActiveVisualization(window->getName().toASCIIString(), nullptr);
            }
        }
        else {
            //refresh active visualization (GtkWidgets may have changed if tree was reloaded)
            setActiveVisualization(m_activeVisualizationWindowName, m_activeVisualizationPanelName);
        }
    }
}

void CDesignerVisualization::dragDataGetFromTreeCB(GtkWidget* srcWidget, GdkDragContext* /*dc*/, GtkSelectionData* selection,
                                                   guint /*info*/, guint /*time*/, gpointer /*data*/)
{
    char str[1024];
    sprintf(str, "%p", srcWidget);
    gtk_selection_data_set_text(selection, str, gint(strlen(str)));
}

void CDesignerVisualization::dragDataGetFromWidgetCB(GtkWidget* srcWidget, GdkDragContext* /*dc*/, GtkSelectionData* selection,
                                                     guint /*info*/, guint /*time*/, gpointer /*data*/)
{
    char str[1024];
    sprintf(str, "%p", srcWidget);
    gtk_selection_data_set_text(selection, str, gint(strlen(str)));
}

void CDesignerVisualization::dragDataReceivedInWidgetCB(GtkWidget* dstWidget, GdkDragContext* /*dc*/, gint /*x*/, gint /*y*/, GtkSelectionData* selection,
                                                        guint /*info*/, guint /*time*/, gpointer data)
{
    static_cast<CDesignerVisualization*>(data)->dragDataReceivedInWidget(dstWidget, selection);
}

void CDesignerVisualization::dragDataReceivedInWidget(GtkWidget* dstWidget, GtkSelectionData* selection)
{
    void* srcWidget = nullptr;
    sscanf(reinterpret_cast<const char*>(gtk_selection_data_get_text(selection)), "%p", &srcWidget);
    GtkTreeIter srcIter;

    //retrieve source widget iterator
    if (GTK_IS_TREE_VIEW(srcWidget)) {
        //ensure dragged widget is a visualization box
        if (!m_tree.findChildNodeFromRoot(&srcIter, m_activeVisualizationBoxID)) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "dragDataReceivedInWidget couldn't retrieve iterator of active visualization box!\n";
            return;
        }
    }
    else if (GTK_IS_BUTTON(srcWidget)) {
        if (srcWidget == dstWidget) { return; }
        if (!m_tree.findChildNodeFromRoot(&srcIter, GetTreeWidget(GTK_WIDGET(srcWidget)))) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "dragDataReceivedInWidget couldn't retrieve iterator of dragged button!\n";
            return;
        }
    }
    else { return; }

    //retrieve src widget identifier and src visualization widget
    CIdentifier srcID;
    m_tree.getIdentifierFromTreeIter(&srcIter, srcID, EVTColumn::StringIdentifier);
    IVWidget* srcVisualizationWidget = m_tree.getVisualizationWidget(srcID);
    if (srcVisualizationWidget == nullptr) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "dragDataReceivedInWidget couldn't retrieve source visualization widget!\n";
        return;
    }

    //retrieve dest widget type
    GtkTreeIter dstIter;
    if (!m_tree.findChildNodeFromRoot(&dstIter, GetTreeWidget(dstWidget))) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "dragDataReceivedInWidget couldn't retrieve iterator of destination widget!\n";
        return;
    }

    //if src widget is unaffected or if dest widget is a visualization box, perform the drop operation directly
    if (srcVisualizationWidget->getParentIdentifier() == CIdentifier::undefined()
        || m_tree.getULongValueFromTreeIter(&dstIter, EVTColumn::ULongNodeType) == size_t(EVTNode::VisualizationBox)) {
        m_tree.dragDataReceivedInWidgetCB(srcID, dstWidget);
    }
    else //dest widget is a dummy : unaffect src widget and simplify the tree before performing the drop operation
    {
        //save dest widget identifier
        CIdentifier dstID;
        m_tree.getIdentifierFromTreeIter(&dstIter, dstID, EVTColumn::StringIdentifier);

        //unaffect src widget, so that tree is simplified
        if (!removeVisualizationWidget(srcID)) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "dragDataReceivedInWidget couldn't remove source widget from its parent!\n";
            return;
        }

        //then drop it
        if (!m_tree.findChildNodeFromRoot(&dstIter, dstID)) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug
                    << "dragDataReceivedInWidget couldn't retrieve iterator of dummy destination widget to delete!\n";
            return;
        }
        void* newDstTreeWidget = nullptr;
        m_tree.getPointerValueFromTreeIter(&dstIter, newDstTreeWidget, EVTColumn::PointerWidget);
        m_tree.dragDataReceivedInWidgetCB(srcID, GetVisualizationWidget(GTK_WIDGET(newDstTreeWidget)));
    }

    //refresh view
    GtkTreeIter draggedIter;
    m_tree.findChildNodeFromRoot(&draggedIter, srcID);
    refreshActiveVisualization(m_tree.getTreePath(&draggedIter));
}

void CDesignerVisualization::dataReceivedInEventBoxCB(GtkWidget* dstWidget, GdkDragContext* /*dc*/, gint /*x*/, gint /*y*/, GtkSelectionData* selection,
                                                      guint /*info*/, guint /*time*/, gpointer data)
{
    char buf[1024];
    void* visualization = nullptr;
    sscanf(static_cast<const char*>(data), "%p %s", &visualization, buf);

    VisualizationToolkit::EDragLocation location;
    if (strcmp(buf, "left") == 0) { location = VisualizationToolkit::EDragLocation::Left; }
    else if (strcmp(buf, "right") == 0) { location = VisualizationToolkit::EDragLocation::Right; }
    else if (strcmp(buf, "top") == 0) { location = VisualizationToolkit::EDragLocation::Top; }
    else { location = VisualizationToolkit::EDragLocation::Bottom; }

    static_cast<CDesignerVisualization*>(visualization)->dragDataReceivedInEventBox(dstWidget, selection, location);
}

void CDesignerVisualization::dragDataReceivedInEventBox(GtkWidget* dstWidget, GtkSelectionData* selection, const VisualizationToolkit::EDragLocation location)
{
    void* srcWidget = nullptr;
    sscanf(reinterpret_cast<const char*>(gtk_selection_data_get_text(selection)), "%p", &srcWidget);
    GtkTreeIter srcIter;

    //get iterator to src widget
    if (GTK_IS_TREE_VIEW(srcWidget)) {
        if (!m_tree.findChildNodeFromRoot(&srcIter, m_activeVisualizationBoxID)) { return; }
        //get actual src widget (item being dropped) and ensure it isn't being dropped in its own table
        m_tree.getPointerValueFromTreeIter(&srcIter, srcWidget, EVTColumn::PointerWidget);
        if (srcWidget == gtk_widget_get_parent(dstWidget)) { return; }
    }
    else if (GTK_IS_BUTTON(srcWidget)) {
        //ensure src widget isn't being dropped in its own table
        if (gtk_widget_get_parent(GTK_WIDGET(srcWidget)) == gtk_widget_get_parent(dstWidget)) { return; }
        m_tree.findChildNodeFromRoot(&srcIter, GetTreeWidget(GTK_WIDGET(srcWidget)));
    }
    else { return; }

    //ensure src widget is a visualization box
    if (m_tree.getULongValueFromTreeIter(&srcIter, EVTColumn::ULongNodeType) != size_t(EVTNode::VisualizationBox)) { return; }

    //retrieve src widget identifier
    CIdentifier srcID;
    m_tree.getIdentifierFromTreeIter(&srcIter, srcID, EVTColumn::StringIdentifier);

    //if widget is unaffected, just drag n drop it
    GtkTreeIter unaffectedIter = srcIter;
    if (m_tree.findParentNode(&unaffectedIter, EVTNode::Unaffected)) { m_tree.dragDataReceivedOutsideWidgetCB(srcID, dstWidget, location); }
    else {
        //save dest widget identifier
        GtkTreeIter dstIter;
        m_tree.findChildNodeFromRoot(&dstIter, GetTreeWidget(dstWidget));
        CIdentifier dstID;
        m_tree.getIdentifierFromTreeIter(&dstIter, dstID, EVTColumn::StringIdentifier);

        //if dest widget is src widget's parent (paned widget), drop src widget in corresponding event box of parent's other child
        //(otherwise, DND will fail due to parent's removal during tree simplification process)
        const IVWidget* srcVisualizationWidget = m_tree.getVisualizationWidget(srcID);
        if (srcVisualizationWidget->getParentIdentifier() == dstID) {
            const IVWidget* widget = m_tree.getVisualizationWidget(srcVisualizationWidget->getParentIdentifier());
            widget->getChildIdentifier(0, dstID);
            if (srcID == dstID) { widget->getChildIdentifier(1, dstID); }
        }

        //unaffect src widget, so that tree is simplified
        removeVisualizationWidget(srcID);

        //then drop it
        m_tree.findChildNodeFromRoot(&dstIter, dstID);
        void* newDstTreeWidget = nullptr;
        m_tree.getPointerValueFromTreeIter(&dstIter, newDstTreeWidget, EVTColumn::PointerWidget);
        m_tree.dragDataReceivedOutsideWidgetCB(srcID, GetVisualizationWidget(GTK_WIDGET(newDstTreeWidget)), location);
    }

    //refresh view
    GtkTreeIter draggedIter;
    m_tree.findChildNodeFromRoot(&draggedIter, srcID);
    refreshActiveVisualization(m_tree.getTreePath(&draggedIter));
}

}  // namespace Designer
}  // namespace OpenViBE
