///-------------------------------------------------------------------------------------------------
/// 
/// \file CInterfacedScenario.cpp
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

#include <boost/filesystem.hpp>
#include "CInterfacedScenario.hpp"
#include "CApplication.hpp"
#include "CBoxProxy.hpp"
#include "CCommentProxy.hpp"
#include "CLinkProxy.hpp"
#include "CConnectorEditor.hpp"
#include "CInterfacedObject.hpp"
#include "TAttributeHandler.hpp"
#include "CDesignerVisualization.hpp"
#include "CPlayerVisualization.hpp"
#include "CRenameDialog.hpp"
#include "CAboutPluginDialog.hpp"
#include "CAboutScenarioDialog.hpp"
#include "CSettingEditorDialog.hpp"
#include "CCommentEditorDialog.hpp"

#include <cmath>
#include <array>
#include <vector>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <algorithm>

#include <gdk/gdkkeysyms.h>
#include <fs/Files.h>

#include <cstdlib>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#include <strings.h>
#define _strcmpi strcasecmp
#endif

namespace OpenViBE {
namespace Designer {

extern std::map<size_t, GdkColor> gColors;

static std::array<GtkTargetEntry, 2> targets = { { { const_cast<gchar*>("STRING"), 0, 0 }, { const_cast<gchar*>("text/plain"), 0, 0 } } };

static GdkColor colorFromIdentifier(const CIdentifier& id, const bool isDeprecated = false)
{
    GdkColor color;
    uint64_t res = id.id();
    color.pixel  = guint16(0);
    color.red    = guint16((res & 0xffff) | 0x8000);
    color.green  = guint16(((res >> 16) & 0xffff) | 0x8000);
    color.blue   = guint16(((res >> 32) & 0xffff) | 0x8000);

    if (isDeprecated) {
        color.blue  = 2 * color.blue / 3;
        color.red   = 2 * color.red / 3;
        color.green = 2 * color.green / 3;
    }

    return color;
}

static std::string getBoxAlgorithmURL(const std::string& in, const bool removeSlash = false)
{
    std::string out;
    bool lastWasSeparator = true;

    for (const char c : in) {
        if (std::isalnum(c) || (!removeSlash && c == '/')) {
            if (c == '/') { out += "_"; }
            else {
                if (lastWasSeparator) { out += std::toupper(c); }
                else { out += c; }
            }
            lastWasSeparator = false;
        }
        else {
            // if(!lastWasSeparator) { out += "_"; }
            lastWasSeparator = true;
        }
    }
    return out;
}

static void CountWidgetCB(GtkWidget* /*widget*/, gpointer data)
{
    int* i = reinterpret_cast<int*>(data);
    if (i) { (*i)++; }
}

static int GtkContainerGetChildrenCount(GtkContainer* container)
{
    int res = 0;
    gtk_container_foreach(container, CountWidgetCB, &res);
    return res;
}

static gboolean ScrolledwindowScrollEventCB(GtkWidget* /*widget*/, GdkEventScroll* event)
{
    guint state = event->state & gtk_accelerator_get_default_mod_mask();

    /* Shift+Wheel scrolls the in the perpendicular direction */
    if (state & GDK_SHIFT_MASK) {
        if (event->direction == GDK_SCROLL_UP) { event->direction = GDK_SCROLL_LEFT; }
        else if (event->direction == GDK_SCROLL_LEFT) { event->direction = GDK_SCROLL_UP; }
        else if (event->direction == GDK_SCROLL_DOWN) { event->direction = GDK_SCROLL_RIGHT; }
        else if (event->direction == GDK_SCROLL_RIGHT) { event->direction = GDK_SCROLL_DOWN; }

        event->state &= ~GDK_SHIFT_MASK;
        state &= ~GDK_SHIFT_MASK;
    }

    return FALSE;
}

static void DrawingAreaExposeCB(GtkWidget* /*widget*/, GdkEventExpose* event, gpointer data)
{
    static_cast<CInterfacedScenario*>(data)->ScenarioDrawingAreaExposeCB(event);
}

static void DrawingAreaDragDataReceivedCB(GtkWidget* /*widget*/, GdkDragContext* dc, const gint x, const gint y, GtkSelectionData* selectionData,
                                          const guint info, const guint t, gpointer data)
{
    static_cast<CInterfacedScenario*>(data)->ScenarioDrawingAreaDragDataReceivedCB(dc, x, y, selectionData, info, t);
}

static gboolean DrawingAreaMotionNotifyCB(GtkWidget* widget, GdkEventMotion* event, gpointer data)
{
    static_cast<CInterfacedScenario*>(data)->ScenarioDrawingAreaMotionNotifyCB(widget, event);
    return FALSE;
}

static void DrawingAreaButtonPressedCB(GtkWidget* widget, GdkEventButton* event, gpointer data)
{
    static_cast<CInterfacedScenario*>(data)->ScenarioDrawingAreaButtonPressedCB(widget, event);
}

static void DrawingAreaButtonReleasedCB(GtkWidget* widget, GdkEventButton* event, gpointer data)
{
    static_cast<CInterfacedScenario*>(data)->ScenarioDrawingAreaButtonReleasedCB(widget, event);
}

static void DrawingAreaKeyPressEventCB(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
    static_cast<CInterfacedScenario*>(data)->ScenarioDrawingAreaKeyPressEventCB(widget, event);
}

static void DrawingAreaKeyReleaseEventCB(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
    static_cast<CInterfacedScenario*>(data)->ScenarioDrawingAreaKeyReleaseEventCB(widget, event);
}

static void MenuCB(GtkMenuItem* /*item*/, CInterfacedScenario::box_ctx_menu_cb_t* cb)
{
    //CInterfacedScenario::box_ctx_menu_cb_t* pContextMenuCB=static_cast < CInterfacedScenario::box_ctx_menu_cb_t* >(data);
    switch (cb->command) {
        case EContextMenu::SelectionCopy: cb->scenario->CopySelection();
            break;
        case EContextMenu::SelectionCut: cb->scenario->CutSelection();
            break;
        case EContextMenu::SelectionPaste: cb->scenario->PasteSelection();
            break;
        case EContextMenu::SelectionDelete: cb->scenario->DeleteSelection();
            break;

        case EContextMenu::BoxRename: cb->scenario->ContextMenuBoxRenameCB(*cb->box);
            break;
        case EContextMenu::BoxUpdate:
        {
            cb->scenario->SnapshotCB();
            cb->scenario->ContextMenuBoxUpdateCB(*cb->box);
            cb->scenario->Redraw();
            break;
        }
        case EContextMenu::BoxRemoveDeprecatedInterfacors:
        {
            cb->scenario->ContextMenuBoxRemoveDeprecatedInterfacorsCB(*cb->box);
            cb->scenario->Redraw();
            break;
        }
        //case EContextMenu::BoxRename: cb->pInterfacedScenario->contextMenuBoxRenameAllCB(); break;
        case EContextMenu::BoxDelete:
        {
            // If selection is empty delete the box under cursor
            if (cb->scenario->m_SelectedObjects.empty()) {
                cb->scenario->DeleteBox(cb->box->getIdentifier());
                cb->scenario->Redraw();
                cb->scenario->SnapshotCB();
            }
            else { cb->scenario->DeleteSelection(); }
            break;
        }
        case EContextMenu::BoxAddInput: cb->scenario->ContextMenuBoxAddInputCB(*cb->box);
            break;
        case EContextMenu::BoxEditInput: cb->scenario->ContextMenuBoxEditInputCB(*cb->box, cb->index);
            break;
        case EContextMenu::BoxRemoveInput: cb->scenario->ContextMenuBoxRemoveInputCB(*cb->box, cb->index);
            break;
        case EContextMenu::BoxAddOutput: cb->scenario->ContextMenuBoxAddOutputCB(*cb->box);
            break;
        case EContextMenu::BoxEditOutput: cb->scenario->ContextMenuBoxEditOutputCB(*cb->box, cb->index);
            break;
        case EContextMenu::BoxRemoveOutput: cb->scenario->ContextMenuBoxRemoveOutputCB(*cb->box, cb->index);
            break;

        case EContextMenu::BoxConnectScenarioInput: cb->scenario->ContextMenuBoxConnectScenarioInputCB(*cb->box, cb->index, cb->secondaryIndex);
            break;
        case EContextMenu::BoxConnectScenarioOutput: cb->scenario->ContextMenuBoxConnectScenarioOutputCB(*cb->box, cb->index, cb->secondaryIndex);
            break;

        case EContextMenu::BoxDisconnectScenarioInput: cb->scenario->ContextMenuBoxDisconnectScenarioInputCB(*cb->box, cb->index, cb->secondaryIndex);
            break;
        case EContextMenu::BoxDisconnectScenarioOutput: cb->scenario->ContextMenuBoxDisconnectScenarioOutputCB(*cb->box, cb->index, cb->secondaryIndex);
            break;

        case EContextMenu::BoxAddSetting: cb->scenario->ContextMenuBoxAddSettingCB(*cb->box);
            break;
        case EContextMenu::BoxEditSetting: cb->scenario->ContextMenuBoxEditSettingCB(*cb->box, cb->index);
            break;
        case EContextMenu::BoxRemoveSetting: cb->scenario->ContextMenuBoxRemoveSettingCB(*cb->box, cb->index);
            break;
        case EContextMenu::BoxConfigure: cb->scenario->ContextMenuBoxConfigureCB(*cb->box);
            break;
        case EContextMenu::BoxAbout: cb->scenario->ContextMenuBoxAboutCB(*cb->box);
            break;
        case EContextMenu::BoxEnable:
        {
            if (cb->scenario->m_SelectedObjects.empty()) { cb->scenario->ContextMenuBoxEnableCB(*cb->box); }
            else { cb->scenario->ContextMenuBoxEnableAllCB(); }
            break;
        }
        case EContextMenu::BoxDisable:
        {
            if (cb->scenario->m_SelectedObjects.empty()) {
                cb->scenario->ContextMenuBoxDisableCB(*cb->box);
                break;
            }
            cb->scenario->ContextMenuBoxDisableAllCB();
            break;
        }
        case EContextMenu::BoxDocumentation: cb->scenario->ContextMenuBoxDocumentationCB(*cb->box);
            break;

        case EContextMenu::BoxEditMetabox: cb->scenario->ContextMenuBoxEditMetaboxCB(*cb->box);
            break;

        case EContextMenu::ScenarioAbout: cb->scenario->ContextMenuScenarioAboutCB();
            break;
        case EContextMenu::ScenarioAddComment: cb->scenario->ContextMenuScenarioAddCommentCB();
            break;

        case EContextMenu::BoxAddMessageInput: break;
        case EContextMenu::BoxRemoveMessageInput: break;
        case EContextMenu::BoxAddMessageOutput: break;
        case EContextMenu::BoxRemoveMessageOutput: break;
        case EContextMenu::BoxEditMessageInput: break;
        case EContextMenu::BoxEditMessageOutput: break;
        default: break;
    }
    // Redraw in any case, as some of the actual callbacks can forget to redraw. As this callback is only called after the user has accessed
    // the right-click menu, so its not a large overhead to do it in general. @TODO might remove the individual redraws.
    cb->scenario->Redraw();
}

static void GDKDrawRoundedRectangle(GdkDrawable* drawable, GdkGC* drawGC, const gboolean fill, const gint x, const gint y,
                                    const gint width, const gint height, const gint radius = 8)
{
    if (fill != 0) {
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
        gdk_draw_rectangle(drawable, drawGC, TRUE, x + radius, y, width - 2 * radius, height);
        gdk_draw_rectangle(drawable, drawGC, TRUE, x, y + radius, width, height - 2 * radius);
#elif defined TARGET_OS_Windows
        gdk_draw_rectangle(drawable, drawGC, TRUE, x + radius, y, width - 2 * radius + 1, height + 1);
        gdk_draw_rectangle(drawable, drawGC, TRUE, x, y + radius, width + 1, height - 2 * radius + 1);
#else
#pragma error("you should give a version of this function for your OS")
#endif
    }
    else {
        gdk_draw_line(drawable, drawGC, x + radius, y, x + width - radius, y);
        gdk_draw_line(drawable, drawGC, x + radius, y + height, x + width - radius, y + height);
        gdk_draw_line(drawable, drawGC, x, y + radius, x, y + height - radius);
        gdk_draw_line(drawable, drawGC, x + width, y + radius, x + width, y + height - radius);
    }
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    gdk_draw_arc(drawable, drawGC, fill, x + width - radius * 2, y, radius * 2, radius * 2, 0 * 64, 90 * 64);
    gdk_draw_arc(drawable, drawGC, fill, x, y, radius * 2, radius * 2, 90 * 64, 90 * 64);
    gdk_draw_arc(drawable, drawGC, fill, x, y + height - radius * 2, radius * 2, radius * 2, 180 * 64, 90 * 64);
    gdk_draw_arc(drawable, drawGC, fill, x + width - radius * 2, y + height - radius * 2, radius * 2, radius * 2, 270 * 64, 90 * 64);
#elif defined TARGET_OS_Windows
    gdk_draw_arc(drawable, drawGC, fill, x + width - radius * 2, y, radius * 2 + (fill != 0 ? 2 : 1), radius * 2 + (fill != 0 ? 2 : 1), 0 * 64, 90 * 64);
    gdk_draw_arc(drawable, drawGC, fill, x, y, radius * 2 + (fill != 0 ? 2 : 1), radius * 2 + (fill != 0 ? 2 : 1), 90 * 64, 90 * 64);
    gdk_draw_arc(drawable, drawGC, fill, x, y + height - radius * 2, radius * 2 + (fill != 0 ? 2 : 1), radius * 2 + (fill != 0 ? 2 : 1), 180 * 64, 90 * 64);
    gdk_draw_arc(drawable, drawGC, fill, x + width - radius * 2, y + height - radius * 2, radius * 2 + (fill != 0 ? 2 : 1), radius * 2 + (fill != 0 ? 2 : 1),
                 270 * 64, 90 * 64);
#else
#pragma error("you should give a version of this function for your OS")
#endif
}

static void TitleButtonCloseCB(GtkButton* /*button*/, gpointer data)
{
    static_cast<CInterfacedScenario*>(data)->m_Application.CloseScenarioCB(static_cast<CInterfacedScenario*>(data));
}

static gboolean WidgetFocusInCB(GtkWidget* /*widget*/, GdkEvent* /*event*/, CApplication* app)
{
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(app->m_Builder, "openvibe-menu_edit")), 0);
    return 0;
}

static gboolean WidgetFocusOutCB(GtkWidget* /*widget*/, GdkEvent* /*event*/, CApplication* app)
{
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(app->m_Builder, "openvibe-menu_edit")), 1);
    return 0;
}

//static void scenario_configuration_add_setting_cb(GtkWidget*, CInterfacedScenario* pInterfacedScenario) { pInterfacedScenario->addScenarioSettingCB(); }

static void ModifyScenarioSettingValueCB(GtkWidget* /*widget*/, CInterfacedScenario::setting_cb_data_t* data)
{
    CIdentifier typeID = CIdentifier::undefined();
    data->scenario->m_Scenario.getSettingType(data->index, typeID);
    data->scenario->m_Scenario.setSettingValue(data->index, data->scenario->m_SettingHelper->GetValue(typeID, data->widgetValue));
    data->scenario->m_HasBeenModified = true;
    data->scenario->UpdateScenarioLabel();
}

static void ModifyScenarioSettingDefaultValueCB(GtkWidget* /*widget*/, CInterfacedScenario::setting_cb_data_t* data)
{
    CIdentifier typeID = CIdentifier::undefined();
    data->scenario->m_Scenario.getSettingType(data->index, typeID);
    data->scenario->m_Scenario.setSettingDefaultValue(data->index, data->scenario->m_SettingHelper->GetValue(typeID, data->widgetValue));

    // We also se the 'actual' value to this
    data->scenario->m_Scenario.setSettingValue(data->index, data->scenario->m_SettingHelper->GetValue(typeID, data->widgetValue));
    data->scenario->m_HasBeenModified = true;
    data->scenario->UpdateScenarioLabel();
}

static void ModifyScenarioSettingMoveUpCB(GtkWidget* /*widget*/, CInterfacedScenario::setting_cb_data_t* data)
{
    if (data->index == 0) { return; }
    data->scenario->SwapScenarioSettings(data->index - 1, data->index);
}

static void ModifyScenarioSettingMoveDownCB(GtkWidget* /*widget*/, CInterfacedScenario::setting_cb_data_t* data)
{
    if (data->index >= data->scenario->m_Scenario.getSettingCount() - 1) { return; }
    data->scenario->SwapScenarioSettings(data->index, data->index + 1);
}

static void ModifyScenarioSettingRevertToDefaultCB(GtkWidget* /*widget*/, CInterfacedScenario::setting_cb_data_t* data)
{
    CString value;
    data->scenario->m_Scenario.getSettingDefaultValue(data->index, value);
    data->scenario->m_Scenario.setSettingValue(data->index, value);
    data->scenario->RedrawScenarioSettings();
}

static void CopyScenarioSettingTokenCB(GtkWidget* /*widget*/, CInterfacedScenario::setting_cb_data_t* data)
{
    CString name;
    data->scenario->m_Scenario.getSettingName(data->index, name);
    name = CString("$var{") + name + CString("}");

    GtkClipboard* defaultClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(defaultClipboard, name.toASCIIString(), -1);

    // On X11 there is another clipboard that it is useful to set as well
    GtkClipboard* x11Clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(x11Clipboard, name.toASCIIString(), -1);
}

static void ModifyScenarioSettingTypeCB(GtkWidget* combobox, CInterfacedScenario::setting_cb_data_t* data)
{
    GtkBuilder* builder = gtk_builder_new();
    gtk_builder_add_from_string(builder, data->scenario->m_SerializedSettingGUIXML.c_str(), data->scenario->m_SerializedSettingGUIXML.length(), nullptr);

    gtk_widget_destroy(data->widgetValue);

    const CIdentifier typeID = data->scenario->m_SettingTypes[gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox))];
    data->scenario->m_Scenario.setSettingType(data->index, typeID);

    const CString name = data->scenario->m_SettingHelper->GetSettingWidgetName(typeID);

    GtkWidget* value = GTK_WIDGET(gtk_builder_get_object(builder, name.toASCIIString()));

    gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(value)), value);
    gtk_table_attach_defaults(GTK_TABLE(data->container), value, 1, 5, 1, 2);

    // Set the value and connect GUI callbacks (because, yes, setValue connects callbacks like a ninja)
    CString str;
    data->scenario->m_Scenario.getSettingDefaultValue(data->index, str);
    data->scenario->m_SettingHelper->SetValue(typeID, value, str);

    // add callbacks to disable the Edit menu in openvibe designer, which will in turn enable using stuff like copy-paste inside the widget
    const CString entryName = data->scenario->m_SettingHelper->GetSettingEntryWidgetName(typeID);
    GtkWidget* entryValue   = GTK_WIDGET(gtk_builder_get_object(builder, entryName.toASCIIString()));

    data->widgetValue      = value;
    data->widgetEntryValue = entryValue;

    g_signal_connect(entryValue, "changed", G_CALLBACK(ModifyScenarioSettingDefaultValueCB), data);

    g_object_unref(builder);
}

static void DeleteScenarioSettingCB(GtkWidget* /*button*/, CInterfacedScenario::setting_cb_data_t* data)
{
    data->scenario->m_Scenario.removeSetting(data->index);
    data->scenario->RedrawConfigureScenarioSettingsDialog();
}

static void ModifyScenarioSettingNameCB(GtkWidget* entry, CInterfacedScenario::setting_cb_data_t* data)
{
    data->scenario->m_Scenario.setSettingName(data->index, gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void ResetScenarioSettingIdentifierCB(GtkWidget* /*button*/, CInterfacedScenario::setting_cb_data_t* data)
{
    const CIdentifier id = data->scenario->m_Scenario.getUnusedSettingIdentifier(CIdentifier::undefined());
    if (id != CIdentifier::undefined()) {
        data->scenario->m_Scenario.updateInterfacorIdentifier(Kernel::EBoxInterfacorType::Setting, data->index, id);
        data->scenario->RedrawConfigureScenarioSettingsDialog();
    }
}

static void ModifyScenarioSettingIdentifierCB(GtkWidget* entry, CInterfacedScenario::setting_cb_data_t* data)
{
    CIdentifier id;
    if (id.fromString(std::string(gtk_entry_get_text(GTK_ENTRY(entry))))) {
        data->scenario->m_Scenario.updateInterfacorIdentifier(Kernel::EBoxInterfacorType::Setting, data->index, id);
    }
}

static void EditScenarioLinkCB(GtkWidget* /*widget*/, CInterfacedScenario::link_cb_data_t* data)
{
    if (data->input) { data->scenario->EditScenarioInputCB(data->index); }
    else { data->scenario->EditScenarioOutputCB(data->index); }
    data->scenario->Redraw();
}

static void ModifyScenarioLinkMoveUpCB(GtkWidget* /*widget*/, CInterfacedScenario::link_cb_data_t* data)
{
    if (data->index == 0) { return; }
    if (data->input) { data->scenario->SwapScenarioInputs(data->index - 1, data->index); }
    else { data->scenario->SwapScenarioOutputs(data->index - 1, data->index); }

    data->scenario->SnapshotCB();
}

static void ModifyScenarioLinkMoveDownCB(GtkWidget* /*widget*/, CInterfacedScenario::link_cb_data_t* data)
{
    const auto interfacorType = data->input ? Kernel::Input : Kernel::Output;
    if (data->scenario->m_Scenario.getInterfacorCount(interfacorType) < 2
        || data->index >= data->scenario->m_Scenario.getInterfacorCount(interfacorType) - 1) { return; }

    if (data->input) { data->scenario->SwapScenarioInputs(data->index, data->index + 1); }
    else { data->scenario->SwapScenarioOutputs(data->index, data->index + 1); }
    data->scenario->SnapshotCB();
}

static void DeleteScenarioLinkCB(GtkButton* /*button*/, CInterfacedScenario::link_cb_data_t* data)
{
    if (data->input) {
        data->scenario->m_Scenario.removeScenarioInput(data->index);
        data->scenario->RedrawScenarioInputSettings();
    }
    else {
        data->scenario->m_Scenario.removeScenarioOutput(data->index);
        data->scenario->RedrawScenarioOutputSettings();
    }

    data->scenario->SnapshotCB();
    data->scenario->Redraw();
}

/*
static void modify_scenario_link_name_cb(GtkWidget* entry, CInterfacedScenario::link_cb_data_t* data)
{
    if (data->m_isInput) { data->m_interfacedScenario->m_scenario.setInputName(data->m_uiLinkIdx, gtk_entry_get_text(GTK_ENTRY(entry))); }
    else { data->m_interfacedScenario->m_scenario.setOutputName(data->m_uiLinkIdx, gtk_entry_get_text(GTK_ENTRY(entry))); }
}

static void modify_scenario_link_type_cb(GtkWidget* comboBox, CInterfacedScenario::link_cb_data_t* data)
{
    const CIdentifier typeID = data->m_interfacedScenario->m_mStreamType[gtk_combo_box_get_active_text(GTK_COMBO_BOX(comboBox))];
    if (data->m_isInput) { data->m_interfacedScenario->m_scenario.setInputType(data->m_uiLinkIdx, typeID); }
    else { data->m_interfacedScenario->m_scenario.setOutputType(data->m_uiLinkIdx, typeID); }
    data->m_interfacedScenario->redraw();
}
//*/

// Redraw Static Helper
static std::array<GdkPoint, 4> get4PointsInterfacorRedraw(const int size, const int shiftX, const int shiftY)
{
    return {
        {
            { (size >> 1) + shiftX, size + shiftY },
            { shiftX, shiftY },
            { size - 1 + shiftX, shiftY },
            { 0, 0 }
        }
    };
}

static void drawScenarioTextIOIndex(GtkWidget* widget, GdkGC* gcline, const size_t index, const gint xText, const gint yText,
                                    const gint xL1, const gint yL1, const gint xL2, const gint yL2)
{
    PangoContext* pangoCtx   = gtk_widget_get_pango_context(widget);
    PangoLayout* pangoLayout = pango_layout_new(pangoCtx);
    pango_layout_set_alignment(pangoLayout, PANGO_ALIGN_CENTER);
    pango_layout_set_markup(pangoLayout, std::to_string(index + 1).c_str(), -1);
    gdk_draw_layout(widget->window, widget->style->text_gc[GTK_WIDGET_STATE(widget)], xText, yText, pangoLayout);
    g_object_unref(pangoLayout);
    gdk_draw_line(widget->window, gcline, xL1, yL1, xL2, yL2);
}

static void drawBorderInterfacor(const GtkWidget* widget, GdkGC* gc, const GdkColor& color, const std::array<GdkPoint, 4>& points, const int border,
                                 const bool isDeprecated)
{
    gdk_gc_set_rgb_fg_color(gc, &color);
    gdk_draw_polygon(widget->window, gc, TRUE, points.data(), 3);
    if (isDeprecated) { gdk_gc_set_rgb_fg_color(gc, &gColors[Color_LinkInvalid]); }
    else { gdk_gc_set_rgb_fg_color(gc, &gColors[border]); }
    gdk_draw_polygon(widget->window, gc, FALSE, points.data(), 3);
}

static void drawCircleWithBorder(const GtkWidget* widget, GdkGC* gc, const GdkColor& bgColor, const GdkColor& fgColor, const gint x, const gint y,
                                 const gint radius)
{
    gdk_gc_set_rgb_fg_color(gc, &bgColor);
    gdk_draw_arc(widget->window, gc, TRUE, x, y, radius, radius, 0, 64 * 360);
    gdk_gc_set_rgb_fg_color(gc, &fgColor);
    gdk_draw_arc(widget->window, gc, FALSE, x, y, radius, radius, 0, 64 * 360);
}

static void linkHandler(Kernel::ILink* link, const int x, const int y, const CIdentifier& attX, const CIdentifier& attY)
{
    if (link) {
        TAttributeHandler handler(*link);

        if (!handler.HasAttribute(attX)) { handler.addAttribute<int>(attX, x); }
        else { handler.setAttributeValue<int>(attX, x); }

        if (!handler.HasAttribute(attY)) { handler.addAttribute<int>(attY, y); }
        else { handler.setAttributeValue<int>(attY, y); }
    }
}

CInterfacedScenario::CInterfacedScenario(const Kernel::IKernelContext& ctx, CApplication& application, Kernel::IScenario& scenario, CIdentifier& scenarioID,
                                         GtkNotebook& notebook, const char* guiFilename, const char* guiSettingsFilename)
    : m_PlayerStatus(Kernel::EPlayerStatus::Stop), m_ScenarioID(scenarioID), m_Application(application), m_Scenario(scenario),
      m_kernelCtx(ctx), m_notebook(notebook), m_guiFilename(guiFilename), m_guiSettingsFilename(guiSettingsFilename)
{
    m_guiBuilder = gtk_builder_new();
    gtk_builder_add_from_file(m_guiBuilder, m_guiFilename.c_str(), nullptr);
    gtk_builder_connect_signals(m_guiBuilder, nullptr);

    std::ifstream settingGUIFilestream;
    FS::Files::openIFStream(settingGUIFilestream, m_guiSettingsFilename.c_str());
    m_SerializedSettingGUIXML = std::string((std::istreambuf_iterator<char>(settingGUIFilestream)), std::istreambuf_iterator<char>());

    m_SettingHelper = new CSettingCollectionHelper(m_kernelCtx, m_guiSettingsFilename.c_str());

    // We will need to access setting types by their name later
    CIdentifier typeID;
    while ((typeID = m_kernelCtx.getTypeManager().getNextTypeIdentifier(typeID)) != CIdentifier::undefined()) {
        if (!m_kernelCtx.getTypeManager().isStream(typeID)) { m_SettingTypes[m_kernelCtx.getTypeManager().getTypeName(typeID).toASCIIString()] = typeID; }
        else { m_streamTypes[m_kernelCtx.getTypeManager().getTypeName(typeID).toASCIIString()] = typeID; }
    }

    m_notebookPageTitle   = GTK_WIDGET(gtk_builder_get_object(m_guiBuilder, "openvibe_scenario_notebook_title"));
    m_notebookPageContent = GTK_WIDGET(gtk_builder_get_object(m_guiBuilder, "openvibe_scenario_notebook_scrolledwindow"));

    gtk_notebook_remove_page(GTK_NOTEBOOK(gtk_builder_get_object(m_guiBuilder, "openvibe-scenario_notebook")), 0);
    gtk_notebook_remove_page(GTK_NOTEBOOK(gtk_builder_get_object(m_guiBuilder, "openvibe-scenario_notebook")), 0);
    gtk_notebook_append_page(&m_notebook, m_notebookPageContent, m_notebookPageTitle);
    gtk_notebook_set_tab_reorderable(&m_notebook, m_notebookPageContent, 1);

    GtkWidget* closeWidget = GTK_WIDGET(gtk_builder_get_object(m_guiBuilder, "openvibe-scenario_button_close"));
    g_signal_connect(G_OBJECT(closeWidget), "clicked", G_CALLBACK(TitleButtonCloseCB), this);

    m_scenarioDrawingArea = GTK_DRAWING_AREA(gtk_builder_get_object(m_guiBuilder, "openvibe-scenario_drawing_area"));
    m_scenarioViewport    = GTK_VIEWPORT(gtk_builder_get_object(m_guiBuilder, "openvibe-scenario_viewport"));
    gtk_drag_dest_set(GTK_WIDGET(m_scenarioDrawingArea), GTK_DEST_DEFAULT_ALL, targets.data(), gint(targets.size()), GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(m_scenarioDrawingArea), "expose-event", G_CALLBACK(DrawingAreaExposeCB), this);
    g_signal_connect(G_OBJECT(m_scenarioDrawingArea), "drag-data-received", G_CALLBACK(DrawingAreaDragDataReceivedCB), this);
    g_signal_connect(G_OBJECT(m_scenarioDrawingArea), "motion-notify-event", G_CALLBACK(DrawingAreaMotionNotifyCB), this);
    g_signal_connect(G_OBJECT(m_scenarioDrawingArea), "button-press-event", G_CALLBACK(DrawingAreaButtonPressedCB), this);
    g_signal_connect(G_OBJECT(m_scenarioDrawingArea), "button-release-event", G_CALLBACK(DrawingAreaButtonReleasedCB), this);
    g_signal_connect(G_OBJECT(m_scenarioDrawingArea), "key-press-event", G_CALLBACK(DrawingAreaKeyPressEventCB), this);
    g_signal_connect(G_OBJECT(m_scenarioDrawingArea), "key-release-event", G_CALLBACK(DrawingAreaKeyReleaseEventCB), this);
    g_signal_connect(G_OBJECT(m_notebookPageContent), "scroll-event", G_CALLBACK(ScrolledwindowScrollEventCB), this);

    m_mensiaLogoPixbuf = gdk_pixbuf_new_from_file(Directories::getDataDir() + "/applications/designer/mensia-decoration.png", nullptr);

#if defined TARGET_OS_Windows
    // add drag-n-drop capabilities onto the scenario notebook to open new scenario
    gtk_drag_dest_add_uri_targets(GTK_WIDGET(m_scenarioDrawingArea));
#endif

    //retrieve visualization tree

    m_Application.m_VisualizationMgr->createVisualizationTree(m_TreeID);
    m_Tree = &m_Application.m_VisualizationMgr->getVisualizationTree(m_TreeID);
    m_Tree->init(&m_Scenario);

    //create window manager
    m_DesignerVisualization = new CDesignerVisualization(m_kernelCtx, *m_Tree, *this);
    m_DesignerVisualization->Init(std::string(guiFilename));

    m_configureSettingsDialog                 = GTK_WIDGET(gtk_builder_get_object(m_Application.m_Builder, "dialog_scenario_configuration"));
    m_settingsVBox                            = GTK_WIDGET(gtk_builder_get_object(m_Application.m_Builder, "dialog_scenario_configuration-vbox"));
    m_noHelpDialog                            = GTK_WIDGET(gtk_builder_get_object(m_Application.m_Builder, "dialog_no_help"));
    m_errorPendingDeprecatedInterfacorsDialog = GTK_WIDGET(gtk_builder_get_object(m_Application.m_Builder, "dialog_pending_deprecated_interfacors"));

    this->RedrawScenarioSettings();
    this->RedrawScenarioInputSettings();
    this->RedrawScenarioOutputSettings();

    m_StateStack.reset(new CScenarioStateStack(ctx, *this, scenario));

    CInterfacedScenario::UpdateScenarioLabel();

    // Output a log message if any box of the scenario is in some special state
    CIdentifier boxID      = CIdentifier::undefined();
    bool warningUpdate     = false;
    bool warningDeprecated = false;
    bool warningUnknown    = false;
    while ((boxID = m_Scenario.getNextBoxIdentifier(boxID)) != CIdentifier::undefined()) {
        //const IBox *box = m_scenario.getBoxDetails(l_oBoxID);
        //const CBoxProxy proxy(m_kernelCtx, *box);
        const CBoxProxy proxy(m_kernelCtx, m_Scenario, boxID);

        if (!warningUpdate && !proxy.IsUpToDate()) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning <<
                    "Scenario requires 'update' of some box(es). You need to replace these boxes or the scenario may not work correctly.\n";
            warningUpdate = true;
        }
        if (!warningDeprecated && proxy.IsDeprecated()) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Scenario constains deprecated box(es). Please consider using other boxes instead.\n";
            warningDeprecated = true;
        }
        //      if (!noteUnstable && proxy.isUnstable())
        //      {
        //          m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Scenario contains unstable box(es).\n";
        //          noteUnstable = true;
        //      }
        if (!warningUnknown && !proxy.IsBoxAlgorithmPluginPresent()) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Scenario contains unknown box algorithm(s).\n";
            if (proxy.IsMetabox()) {
                CString mPath = m_kernelCtx.getConfigurationManager().expand("${Kernel_Metabox}");
                m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Some Metaboxes could not be found in [" << mPath << "]\n";
            }
            warningUnknown = true;
        }
    }
}

CInterfacedScenario::~CInterfacedScenario()
{
    //delete window manager
    delete m_DesignerVisualization;

    if (m_stencilBuffer != nullptr) { g_object_unref(m_stencilBuffer); }

    g_object_unref(m_guiBuilder);
    /*
    g_object_unref(m_builder);
    g_object_unref(m_builder);
    */

    gtk_notebook_remove_page(&m_notebook, gtk_notebook_page_num(&m_notebook, m_notebookPageContent));
}

void CInterfacedScenario::Redraw() const
{
    if (GDK_IS_WINDOW(GTK_WIDGET(m_scenarioDrawingArea)->window)) { gdk_window_invalidate_rect(GTK_WIDGET(m_scenarioDrawingArea)->window, nullptr, 1); }
}

// This function repaints the dialog which opens when configuring settings
void CInterfacedScenario::RedrawConfigureScenarioSettingsDialog()
{
    if (m_HasFileName) {
        char filename[1024];
        FS::Files::getFilename(m_Filename.c_str(), filename);
        const std::string title = std::string("Settings for \"") + filename + "\"";
        gtk_window_set_title(GTK_WINDOW(m_configureSettingsDialog), title.c_str());
    }
    else { gtk_window_set_title(GTK_WINDOW(m_configureSettingsDialog), "Settings for an unnamed scenario"); }

    GList* widgets = gtk_container_get_children(GTK_CONTAINER(m_settingsVBox));
    for (GList* it = widgets; it != nullptr; it = g_list_next(it)) { gtk_widget_destroy(GTK_WIDGET(it->data)); }
    g_list_free(widgets);

    m_settingConfigCBDatas.clear();
    m_settingConfigCBDatas.resize(m_Scenario.getSettingCount());

    if (m_Scenario.getSettingCount() == 0) {
        GtkWidget* widget = gtk_label_new("This scenario has no settings");
        gtk_box_pack_start(GTK_BOX(m_settingsVBox), widget, TRUE, TRUE, 5);
    }
    else {
        for (size_t i = 0; i < m_Scenario.getSettingCount(); ++i) {
            GtkBuilder* builder = gtk_builder_new();
            gtk_builder_add_from_string(builder, m_SerializedSettingGUIXML.c_str(), m_SerializedSettingGUIXML.length(), nullptr);

            GtkWidget* container = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_configuration_setting-table"));
            // this has to be done since the widget is already inside a parent in the gtkbuilder
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(container)), container);
            gtk_box_pack_start(GTK_BOX(m_settingsVBox), container, FALSE, FALSE, 5);

            GtkWidget* entryName     = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_configuration_setting-entry_name"));
            GtkWidget* comboBoxType  = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_configuration_setting-combobox_type"));
            GtkWidget* buttonUp      = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_configuration_setting-button_move_up"));
            GtkWidget* buttonDown    = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_configuration_setting-button_move_down"));
            GtkWidget* buttonDelete  = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_configuration_setting-button_delete"));
            GtkWidget* entryID       = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_configuration_setting-entry_identifier"));
            GtkWidget* buttonResetID = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_configuration_setting-button_reset_identifier"));

            // fill the type dropdown
            CIdentifier typeID = CIdentifier::undefined();
            m_Scenario.getSettingType(i, typeID);

            CIdentifier id;
            CString str;
            gint idx = 0;
            while ((id = m_kernelCtx.getTypeManager().getNextTypeIdentifier(id)) != CIdentifier::undefined()) {
                if (!m_kernelCtx.getTypeManager().isStream(id)) {
                    gtk_combo_box_append_text(GTK_COMBO_BOX(comboBoxType), m_kernelCtx.getTypeManager().getTypeName(id).toASCIIString());
                    if (id == typeID) { gtk_combo_box_set_active(GTK_COMBO_BOX(comboBoxType), idx); }
                    idx++;
                }
            }
            // Set name
            m_Scenario.getSettingName(i, str);
            gtk_entry_set_text(GTK_ENTRY(entryName), str.toASCIIString());

            // Set the identifer
            m_Scenario.getInterfacorIdentifier(Kernel::EBoxInterfacorType::Setting, i, id);
            gtk_entry_set_text(GTK_ENTRY(entryID), id.str().c_str());

            // Add widget for the actual setting
            str                     = m_SettingHelper->GetSettingWidgetName(typeID);
            GtkWidget* defaultValue = GTK_WIDGET(gtk_builder_get_object(builder, str.toASCIIString()));

            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(defaultValue)), defaultValue);
            gtk_table_attach_defaults(GTK_TABLE(container), defaultValue, 1, 5, 1, 2);

            // Set the value and connect GUI callbacks (because, yes, setValue connects callbacks like a ninja)
            m_Scenario.getSettingDefaultValue(i, str);
            m_SettingHelper->SetValue(typeID, defaultValue, str);

            // add callbacks to disable the Edit menu in openvibe designer, which will in turn enable using stuff like copy-paste inside the widget
            str                          = m_SettingHelper->GetSettingEntryWidgetName(typeID);
            GtkWidget* entryDefaultValue = GTK_WIDGET(gtk_builder_get_object(builder, str.toASCIIString()));

            // Set the callbacks
            setting_cb_data_t cbData;
            cbData.scenario         = this;
            cbData.index            = i;
            cbData.widgetValue      = defaultValue;
            cbData.widgetEntryValue = entryDefaultValue;
            cbData.container        = container;

            m_settingConfigCBDatas[i] = cbData;

            // Connect signals of the container
            g_signal_connect(G_OBJECT(comboBoxType), "changed", G_CALLBACK(ModifyScenarioSettingTypeCB), &m_settingConfigCBDatas[i]);
            g_signal_connect(G_OBJECT(buttonDelete), "clicked", G_CALLBACK(DeleteScenarioSettingCB), &m_settingConfigCBDatas[i]);
            g_signal_connect(G_OBJECT(buttonUp), "clicked", G_CALLBACK(ModifyScenarioSettingMoveUpCB), &m_settingConfigCBDatas[i]);
            g_signal_connect(G_OBJECT(buttonDown), "clicked", G_CALLBACK(ModifyScenarioSettingMoveDownCB), &m_settingConfigCBDatas[i]);
            g_signal_connect(G_OBJECT(entryName), "changed", G_CALLBACK(ModifyScenarioSettingNameCB), &m_settingConfigCBDatas[i]);
            g_signal_connect(G_OBJECT(entryID), "activate", G_CALLBACK(ModifyScenarioSettingIdentifierCB), &m_settingConfigCBDatas[i]);
            g_signal_connect(G_OBJECT(buttonResetID), "clicked", G_CALLBACK(ResetScenarioSettingIdentifierCB), &m_settingConfigCBDatas[i]);

            // these callbacks assure that we can use copy/paste and undo within editable fields
            // as otherwise the keyboard shortucts are stolen by the designer
            g_signal_connect(G_OBJECT(entryName), "focus-in-event", G_CALLBACK(WidgetFocusInCB), &m_Application);
            g_signal_connect(G_OBJECT(entryName), "focus-out-event", G_CALLBACK(WidgetFocusOutCB), &m_Application);
            g_signal_connect(G_OBJECT(entryDefaultValue), "focus-in-event", G_CALLBACK(WidgetFocusInCB), &m_Application);
            g_signal_connect(G_OBJECT(entryDefaultValue), "focus-out-event", G_CALLBACK(WidgetFocusOutCB), &m_Application);

            // add callbacks for setting the settings
            g_signal_connect(entryDefaultValue, "changed", G_CALLBACK(ModifyScenarioSettingDefaultValueCB), &m_settingConfigCBDatas[i]);

            g_object_unref(builder);
        }
    }
}

// This function, similar to the previous one, repaints the settings handling sidebar
void CInterfacedScenario::RedrawScenarioSettings()
{
    GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_Application.m_Builder, "openvibe-scenario_configuration_vbox"));

    GList* widgets = gtk_container_get_children(GTK_CONTAINER(widget));
    for (GList* settingIterator = widgets; settingIterator != nullptr; settingIterator = g_list_next(settingIterator)) {
        gtk_widget_destroy(GTK_WIDGET(settingIterator->data));
    }
    g_list_free(widgets);

    m_settingCBDatas.clear();
    m_settingCBDatas.resize(m_Scenario.getSettingCount());

    if (m_Scenario.getSettingCount() == 0) {
        GtkWidget* label = gtk_label_new("This scenario has no settings");
        gtk_box_pack_start(GTK_BOX(widget), label, TRUE, TRUE, 5);
    }
    else {
        for (size_t i = 0; i < m_Scenario.getSettingCount(); ++i) {
            GtkBuilder* builder = gtk_builder_new();
            gtk_builder_add_from_string(builder, m_SerializedSettingGUIXML.c_str(), m_SerializedSettingGUIXML.length(), nullptr);

            GtkWidget* container = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_setting-table"));
            // this has to be done since the widget is already inside a parent in the gtkbuilder
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(container)), container);
            gtk_box_pack_start(GTK_BOX(widget), container, FALSE, FALSE, 5);

            GtkWidget* labelName     = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_setting-label"));
            GtkWidget* buttonDefault = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_setting-button_default"));
            GtkWidget* buttonCopy    = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_setting-button_copy"));

            // Set name
            CString str;
            m_Scenario.getSettingName(i, str);
            gtk_label_set_text(GTK_LABEL(labelName), str.toASCIIString());
            gtk_misc_set_alignment(GTK_MISC(labelName), 0.0, 0.5);

            // Add widget for the actual setting
            CIdentifier typeID = CIdentifier::undefined();
            m_Scenario.getSettingType(i, typeID);
            str = m_SettingHelper->GetSettingWidgetName(typeID);

            GtkWidget* value = GTK_WIDGET(gtk_builder_get_object(builder, str.toASCIIString()));

            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(value)), value);
            gtk_table_attach_defaults(GTK_TABLE(container), value, 0, 1, 1, 2);

            // Set the value and connect GUI callbacks (because, yes, setValue connects callbacks like a ninja)
            m_Scenario.getSettingValue(i, str);
            m_SettingHelper->SetValue(typeID, value, str);

            // add callbacks to disable the Edit menu in openvibe designer, which will in turn enable using stuff like copy-paste inside the widget
            str                   = m_SettingHelper->GetSettingEntryWidgetName(typeID);
            GtkWidget* entryValue = GTK_WIDGET(gtk_builder_get_object(builder, str.toASCIIString()));

            // Set the callbacks
            setting_cb_data_t cbData;
            cbData.scenario         = this;
            cbData.index            = i;
            cbData.widgetValue      = value;
            cbData.widgetEntryValue = entryValue;
            cbData.container        = container;

            m_settingCBDatas[i] = cbData;

            // these callbacks assure that we can use copy/paste and undo within editable fields
            // as otherwise the keyboard shortucts are stolen by the designer
            g_signal_connect(G_OBJECT(entryValue), "focus-in-event", G_CALLBACK(WidgetFocusInCB), &m_Application);
            g_signal_connect(G_OBJECT(entryValue), "focus-out-event", G_CALLBACK(WidgetFocusOutCB), &m_Application);

            // add callbacks for setting the settings
            g_signal_connect(entryValue, "changed", G_CALLBACK(ModifyScenarioSettingValueCB), &m_settingCBDatas[i]);
            g_signal_connect(buttonDefault, "clicked", G_CALLBACK(ModifyScenarioSettingRevertToDefaultCB), &m_settingCBDatas[i]);
            g_signal_connect(buttonCopy, "clicked", G_CALLBACK(CopyScenarioSettingTokenCB), &m_settingCBDatas[i]);

            g_object_unref(builder);
        }
    }
    gtk_widget_show_all(widget);
}

void CInterfacedScenario::RedrawScenarioInputSettings()
{
    size_t (Kernel::IScenario::* getNLink)() const                      = &Kernel::IScenario::getInputCount;
    bool (Kernel::IScenario::* getLinkName)(size_t, CString&) const     = &Kernel::IScenario::getInputName;
    bool (Kernel::IScenario::* getLinkType)(size_t, CIdentifier&) const = &Kernel::IScenario::getInputType;

    this->redrawScenarioLinkSettings(m_Application.m_Inputs, true, m_scenarioInputCBDatas, getNLink, getLinkName, getLinkType);
}

void CInterfacedScenario::RedrawScenarioOutputSettings()
{
    size_t (Kernel::IScenario::* getNLink)() const                      = &Kernel::IScenario::getOutputCount;
    bool (Kernel::IScenario::* getLinkName)(size_t, CString&) const     = &Kernel::IScenario::getOutputName;
    bool (Kernel::IScenario::* getLinkType)(size_t, CIdentifier&) const = &Kernel::IScenario::getOutputType;

    this->redrawScenarioLinkSettings(m_Application.m_Outputs, false, m_scenarioOutputCBDatas, getNLink, getLinkName, getLinkType);
}

// Redraws the tab containing inputs or outputs of the scenario
// This method receives pointers to methods that manipulate either intpus or outputs so it can be generic
void CInterfacedScenario::redrawScenarioLinkSettings(GtkWidget* links, const bool isInput, std::vector<link_cb_data_t>& linkCBDatas,
                                                     size_t (Kernel::IScenario::* getNLink)() const,
                                                     bool (Kernel::IScenario::* getLinkName)(size_t, CString&) const,
                                                     bool (Kernel::IScenario::* getLinkType)(size_t, CIdentifier&) const)
{
    GList* widgets = gtk_container_get_children(GTK_CONTAINER(links));
    for (GList* it = widgets; it != nullptr; it = g_list_next(it)) { gtk_widget_destroy(GTK_WIDGET(it->data)); }
    g_list_free(widgets);

    const size_t nLink = (m_Scenario.*getNLink)();

    linkCBDatas.clear();
    linkCBDatas.resize(nLink);

    gtk_table_resize(GTK_TABLE(links), guint(nLink == 0 ? 1 : nLink), 7);

    if (nLink == 0) {
        GtkWidget* settingPlaceholderLabel = gtk_label_new("This scenario has none");
        gtk_table_attach_defaults(GTK_TABLE(links), settingPlaceholderLabel, 0, 1, 0, 1);
    }
    else {
        for (size_t i = 0; i < nLink; ++i) {
            GtkBuilder* builder = gtk_builder_new();
            gtk_builder_add_from_string(builder, m_SerializedSettingGUIXML.c_str(), m_SerializedSettingGUIXML.length(), nullptr);

            GtkWidget* container = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_io_setting-table"));
            // this has to be done since the widget is already inside a parent in the gtkbuilder
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(container)), container);

            GtkWidget* entryLinkName = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_io_setting-label"));
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(entryLinkName)), entryLinkName);

            GtkWidget* comboBoxType = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_io_setting-combobox_type"));
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(comboBoxType)), comboBoxType);

            // fill the type dropdown
            CIdentifier typeID = CIdentifier::undefined();
            (m_Scenario.*getLinkType)(i, typeID);

            CIdentifier id;
            gint idx = 0;
            while ((id = m_kernelCtx.getTypeManager().getNextTypeIdentifier(id)) != CIdentifier::undefined()) {
                if (m_kernelCtx.getTypeManager().isStream(id)) {
                    gtk_combo_box_append_text(GTK_COMBO_BOX(comboBoxType), m_kernelCtx.getTypeManager().getTypeName(id).toASCIIString());
                    if (id == typeID) { gtk_combo_box_set_active(GTK_COMBO_BOX(comboBoxType), idx); }

                    idx++;
                }
            }
            gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(comboBoxType), GTK_SENSITIVITY_OFF);

            GtkWidget* buttonUp = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_io_setting-button_move_up"));
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(buttonUp)), buttonUp);
            GtkWidget* buttonDown = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_io_setting-button_move_down"));
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(buttonDown)), buttonDown);
            GtkWidget* buttonEdit = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_io_setting-button_edit"));
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(buttonEdit)), buttonEdit);
            GtkWidget* buttonDelete = GTK_WIDGET(gtk_builder_get_object(builder, "scenario_io_setting-button_delete"));
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(buttonDelete)), buttonDelete);

            // Set name
            CString str;
            (m_Scenario.*getLinkName)(i, str);
            gtk_label_set_text(GTK_LABEL(entryLinkName), str.toASCIIString());
            gtk_misc_set_alignment(GTK_MISC(entryLinkName), 0.0, 0.5);
            gtk_widget_set_sensitive(GTK_WIDGET(entryLinkName), GTK_SENSITIVITY_OFF);

            const guint gi = guint(i);
            gtk_table_attach(GTK_TABLE(links), entryLinkName, 0, 1, gi, gi + 1, GtkAttachOptions(GTK_EXPAND | GTK_FILL), GTK_SHRINK, 4, 4);
            gtk_table_attach(GTK_TABLE(links), comboBoxType, 1, 2, gi, gi + 1, GTK_SHRINK, GTK_SHRINK, 4, 4);
            gtk_table_attach(GTK_TABLE(links), buttonUp, 3, 4, gi, gi + 1, GTK_SHRINK, GTK_SHRINK, 4, 4);
            gtk_table_attach(GTK_TABLE(links), buttonDown, 4, 5, gi, gi + 1, GTK_SHRINK, GTK_SHRINK, 4, 4);
            gtk_table_attach(GTK_TABLE(links), buttonEdit, 5, 6, gi, gi + 1, GTK_SHRINK, GTK_SHRINK, 4, 4);
            gtk_table_attach(GTK_TABLE(links), buttonDelete, 6, 7, gi, gi + 1, GTK_SHRINK, GTK_SHRINK, 4, 4);

            // Set the callbacks
            link_cb_data_t cbData;
            cbData.scenario = this;
            cbData.index    = i;
            cbData.input    = isInput;

            linkCBDatas[i] = cbData;

            g_signal_connect(G_OBJECT(buttonDelete), "clicked", G_CALLBACK(DeleteScenarioLinkCB), &linkCBDatas[i]);
            g_signal_connect(G_OBJECT(buttonEdit), "clicked", G_CALLBACK(EditScenarioLinkCB), &linkCBDatas[i]);
            g_signal_connect(G_OBJECT(buttonUp), "clicked", G_CALLBACK(ModifyScenarioLinkMoveUpCB), &linkCBDatas[i]);
            g_signal_connect(G_OBJECT(buttonDown), "clicked", G_CALLBACK(ModifyScenarioLinkMoveDownCB), &linkCBDatas[i]);

            g_object_unref(builder);
        }
    }

    gtk_widget_show_all(links);
}

void CInterfacedScenario::UpdateScenarioLabel()
{
    GtkLabel* gtkLabel = GTK_LABEL(gtk_builder_get_object(m_guiBuilder, "openvibe-scenario_label"));
    std::string label;
    std::string filename       = m_Filename;
    std::string labelUntrimmed = "unsaved document";
    std::string::size_type pos;
    while ((pos = filename.find('\\')) != std::string::npos) { filename[pos] = '/'; }

    label += m_HasBeenModified ? "*" : "";
    label += " ";

    // trimming file name if the number of character is above ${Designer_ScenarioFileNameTrimmingLimit}
    // trim only unselected scenarios
    if (m_HasFileName) {
        labelUntrimmed   = filename;
        filename         = filename.substr(filename.rfind('/') + 1);
        size_t trimLimit = size_t(m_kernelCtx.getConfigurationManager().expandAsUInteger("${Designer_ScenarioFileNameTrimmingLimit}", 25));
        if (trimLimit > 3) { trimLimit -= 3; } // limit should include the '...'
        // default = we trim everything but the current scenario filename
        // if  {we are stacking horizontally the scenarios, we trim also } current filename to avoid losing too much of the edition panel.
        if (filename.size() > trimLimit) {
            if (m_Application.GetCurrentInterfacedScenario() == this
                && m_kernelCtx.getConfigurationManager().expandAsBoolean("${Designer_ScenarioTabsVerticalStack}", false)) {
                filename = "..." + filename.substr(filename.size() - trimLimit, trimLimit);
            }
            if (m_Application.GetCurrentInterfacedScenario() != this) {
                filename = filename.substr(0, trimLimit);
                filename += "...";
            }
        }
        label += filename;
    }
    else { label += "(untitled)"; }

    label += " ";
    label += m_HasBeenModified ? "*" : "";

    gtk_label_set_text(gtkLabel, label.c_str());

    label = labelUntrimmed;
    pos   = 0;
    while ((pos = label.find('&', pos)) != std::string::npos) {
        label.replace(pos, 1, "&amp;");
        pos += 5;
    }
    gtk_widget_set_tooltip_markup(GTK_WIDGET(gtkLabel), ("<i>" + label + (m_HasBeenModified ? " - unsaved" : "") + "</i>").c_str());
}

#define UPDATE_STENCIL_IDX(id,stencilgc) { (id)++; ::GdkColor sc={0, guint16(((id)&0xff0000)>>8), guint16((id)&0xff00), guint16(((id)&0xff)<<8) }; gdk_gc_set_rgb_fg_color(stencilgc, &sc); }

void CInterfacedScenario::Redraw(Kernel::IBox& box)
{
    GtkWidget* widget = GTK_WIDGET(m_scenarioDrawingArea);
    GdkGC* stencilGC  = gdk_gc_new(GDK_DRAWABLE(m_stencilBuffer));
    GdkGC* drawGC     = gdk_gc_new(widget->window);

    const int marginX     = int(round(5 * m_currentScale));
    const int marginY     = int(round(5 * m_currentScale));
    const int circleSize  = int(round(11 * m_currentScale));
    const int circleSpace = int(round(4 * m_currentScale));

    //CBoxProxy proxy(m_kernelCtx, box);
    CBoxProxy proxy(m_kernelCtx, m_Scenario, box.getIdentifier());

    if (box.getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox) {
        CIdentifier metaboxId;
        metaboxId.fromString(box.getAttributeValue(OVP_AttributeId_Metabox_ID));
        proxy.SetBoxAlgorithmDescriptorOverride(
            dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(m_kernelCtx.getMetaboxManager().getMetaboxObjectDesc(metaboxId)));
    }

    int sizeX  = int(round(proxy.GetWidth(GTK_WIDGET(m_scenarioDrawingArea)) * m_currentScale) + marginX * 2);
    int sizeY  = int(round(proxy.GetHeight(GTK_WIDGET(m_scenarioDrawingArea)) * m_currentScale) + marginY * 2);
    int startX = int(round(proxy.GetXCenter() * m_currentScale + m_viewOffsetX - (sizeX >> 1)));
    int startY = int(round(proxy.GetYCenter() * m_currentScale + m_viewOffsetY - (sizeY >> 1)));

    UPDATE_STENCIL_IDX(m_interfacedObjectId, stencilGC)
    GDKDrawRoundedRectangle(GDK_DRAWABLE(m_stencilBuffer), stencilGC, TRUE, startX, startY, sizeX, sizeY, gint(round(8.0 * m_currentScale)));
    m_interfacedObjects[m_interfacedObjectId] = CInterfacedObject(box.getIdentifier());

    bool canCreate                    = proxy.IsBoxAlgorithmPluginPresent();
    bool upToDate                     = canCreate ? proxy.IsUpToDate() : true;
    bool pendingDeprecatedInterfacors = proxy.HasPendingDeprecatedInterfacors();
    bool deprecated                   = canCreate && proxy.IsDeprecated();
    bool metabox                      = canCreate && proxy.IsMetabox();
    bool disabled                     = proxy.IsDisabled();

    // Add a thick dashed border around selected boxes
    if (m_SelectedObjects.count(box.getIdentifier())) {
        int offsetTL = 2;   // Offset Top Left
#if defined TARGET_OS_Windows
        int offsetBR = 4;   // Offset Bottom Right
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
        int offsetBR = 5;   // Offset Bottom Right
#else
        int offsetBR = 4;   // Offset Bottom Right
#endif
        if (metabox) {
            offsetTL = 3;
            offsetBR = 6;
        }

        gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_BoxBorderSelected]);
        gdk_gc_set_line_attributes(drawGC, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
        GDKDrawRoundedRectangle(widget->window, drawGC, TRUE, startX - offsetTL, startY - offsetTL, sizeX + offsetBR, sizeY + offsetBR);
    }

    if (!this->IsLocked() || !m_DebugCPUUsage) {
        //if(m_currentObject[box.getIdentifier()]) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_BoxBackgroundSelected]); }
        //else
        if (!canCreate) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_BoxBackgroundMissing]); }
        else if (disabled) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_BoxBackgroundDisabled]); }
        else if (deprecated) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_BoxBackgroundDeprecated]); }
        else if (!upToDate || pendingDeprecatedInterfacors) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_BoxBackgroundOutdated]); }
        //else if(metabox) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_BoxBackgroundMetabox]); }
        else { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_BoxBackground]); }
    }
    else {
        CIdentifier timeID;
        timeID.fromString(box.getAttributeValue(OV_AttributeId_Box_ComputationTimeLastSecond));
        uint64_t time      = (timeID == CIdentifier::undefined() ? 0 : timeID.id());
        uint64_t reference = (1LL << 32) / (m_nBox == 0 ? 1 : m_nBox);

        GdkColor color;
        if (time < reference) {
            color.pixel = 0;
            color.red   = guint16((time << 16) / reference);
            color.green = 32768;
            color.blue  = 0;
        }
        else {
            if (time < reference * 4) {
                color.pixel = 0;
                color.red   = 65535;
                color.green = guint16(32768 - ((time << 15) / (reference * 4)));
                color.blue  = 0;
            }
            else {
                color.pixel = 0;
                color.red   = 65535;
                color.green = 0;
                color.blue  = 0;
            }
        }
        gdk_gc_set_rgb_fg_color(drawGC, &color);
    }

    GDKDrawRoundedRectangle(widget->window, drawGC, TRUE, startX, startY, sizeX, sizeY, gint(round(8.0 * m_currentScale)));


    int borderColor = Color_BoxBorder;
    gdk_gc_set_rgb_fg_color(drawGC, &gColors[borderColor]);
    gdk_gc_set_line_attributes(drawGC, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    GDKDrawRoundedRectangle(widget->window, drawGC, FALSE, startX, startY, sizeX, sizeY, gint(round(8.0 * m_currentScale)));

    if (metabox) {
        gdk_gc_set_rgb_fg_color(drawGC, &gColors[borderColor]);
        gdk_gc_set_line_attributes(drawGC, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
        GDKDrawRoundedRectangle(widget->window, drawGC, FALSE, startX - 3, startY - 3, sizeX + 6, sizeY + 6, gint(round(8.0 * m_currentScale)));
    }

    TAttributeHandler handler(box);

    int offset = sizeX / 2 - int(box.getInputCount()) * (circleSpace + circleSize) / 2 + circleSize / 4;
    for (size_t i = 0; i < box.getInterfacorCountIncludingDeprecated(Kernel::Input); ++i) {
        CIdentifier id;
        bool isDeprecated;
        box.getInputType(i, id);
        box.getInterfacorDeprecatedStatus(Kernel::Input, i, isDeprecated);

        GdkColor color    = colorFromIdentifier(id, isDeprecated);
        const auto points = get4PointsInterfacorRedraw(circleSize, startX + int(i) * (circleSpace + circleSize) + offset, startY - (circleSize >> 1));

        UPDATE_STENCIL_IDX(m_interfacedObjectId, stencilGC)
        gdk_draw_polygon(GDK_DRAWABLE(m_stencilBuffer), stencilGC, TRUE, points.data(), 3);
        m_interfacedObjects[m_interfacedObjectId] = CInterfacedObject(box.getIdentifier(), Box_Input, i);

        drawBorderInterfacor(widget, drawGC, color, points, Color_BoxInputBorder, isDeprecated);

        int x = startX + int(i) * (circleSpace + circleSize) + (circleSize >> 1) - m_viewOffsetX + offset;
        int y = startY - (circleSize >> 1) - m_viewOffsetY;
        id    = m_Scenario.getNextLinkIdentifierToBoxInput(CIdentifier::undefined(), box.getIdentifier(), i);
        while (id != CIdentifier::undefined()) {
            Kernel::ILink* link = m_Scenario.getLinkDetails(id);
            linkHandler(link, x, y, OV_AttributeId_Link_XDst, OV_AttributeId_Link_YDst);
            id = m_Scenario.getNextLinkIdentifierToBoxInput(id, box.getIdentifier(), i);
        }

        // Display a circle above inputs that are linked to the box inputs
        for (size_t j = 0; j < m_Scenario.getInputCount(); j++) {
            size_t boxInputIdx;
            m_Scenario.getScenarioInputLink(j, id, boxInputIdx);

            if (id == box.getIdentifier() && boxInputIdx == i) {
                // Since the circle representing the input is quite large, we are going to offset each other one
                int offsetDisc = int(i % 2) * circleSize * 2;

                const int left = startX + int(i) * (circleSpace + circleSize) + offset - int(circleSize * 0.5);
                const int top  = startY - (circleSize >> 1) - circleSize * 3 - offsetDisc;

                this->m_Scenario.getInputType(j, id);
                color = colorFromIdentifier(id, false);

                UPDATE_STENCIL_IDX(m_interfacedObjectId, stencilGC)
                gdk_draw_arc(GDK_DRAWABLE(m_stencilBuffer), stencilGC, TRUE, left, top, circleSize * 2, circleSize * 2, 0, 64 * 360);
                m_interfacedObjects[m_interfacedObjectId] = CInterfacedObject(box.getIdentifier(), Box_ScenarioInput, i);

                drawCircleWithBorder(widget, drawGC, color, gColors[Color_BoxInputBorder], left, top, circleSize * 2);

                // Draw the text indicating the scenario input index
                drawScenarioTextIOIndex(widget, drawGC, j, left + marginX, top + marginY,
                                        startX + int(i) * (circleSpace + circleSize) + offset + (circleSize >> 1), top + circleSize * 2,
                                        startX + int(i) * (circleSpace + circleSize) + offset + (circleSize >> 1), startY - (circleSize >> 1));
            }
        }
    }

    gdk_gc_set_line_attributes(drawGC, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

    offset = sizeX / 2 - int(box.getOutputCount()) * (circleSpace + circleSize) / 2 + circleSize / 4;
    for (size_t i = 0; i < box.getInterfacorCountIncludingDeprecated(Kernel::Output); ++i) {
        CIdentifier id;
        bool isDeprecated;
        box.getOutputType(i, id);
        box.getInterfacorDeprecatedStatus(Kernel::Output, i, isDeprecated);
        GdkColor color = colorFromIdentifier(id, isDeprecated);

        const auto points = get4PointsInterfacorRedraw(circleSize, startX + int(i) * (circleSpace + circleSize) + offset, startY - (circleSize >> 1) + sizeY);
        UPDATE_STENCIL_IDX(m_interfacedObjectId, stencilGC)
        gdk_draw_polygon(GDK_DRAWABLE(m_stencilBuffer), stencilGC, TRUE, points.data(), 3);

        m_interfacedObjects[m_interfacedObjectId] = CInterfacedObject(box.getIdentifier(), Box_Output, i);

        drawBorderInterfacor(widget, drawGC, color, points, Color_BoxOutputBorder, isDeprecated);

        int x = startX + int(i) * (circleSpace + circleSize) + (circleSize >> 1) - m_viewOffsetX + offset;
        int y = startY + sizeY + (circleSize >> 1) + 1 - m_viewOffsetY;
        id    = m_Scenario.getNextLinkIdentifierFromBoxOutput(CIdentifier::undefined(), box.getIdentifier(), i);
        while (id != CIdentifier::undefined()) {
            Kernel::ILink* link = m_Scenario.getLinkDetails(id);
            if (link) {
                TAttributeHandler attHandler(*link);
                linkHandler(link, x, y, OV_AttributeId_Link_XSrc, OV_AttributeId_Link_YSrc);
            }
            id = m_Scenario.getNextLinkIdentifierFromBoxOutput(id, box.getIdentifier(), i);
        }

        // Display a circle below outputs that are linked to the box outputs
        for (size_t j = 0; j < m_Scenario.getOutputCount(); j++) {
            size_t boxOutputIdx;
            m_Scenario.getScenarioOutputLink(j, id, boxOutputIdx);
            if (id == box.getIdentifier() && boxOutputIdx == i) {
                // Since the circle representing the Output is quite large, we are going to offset each other one
                int offsetDisc = (int(i) % 2) * circleSize * 2;

                const int left = startX + int(i) * (circleSpace + circleSize) + offset - int(circleSize * 0.5);
                const int top  = startY - (circleSize >> 1) + sizeY + offsetDisc + circleSize * 2;

                this->m_Scenario.getOutputType(j, id);
                color = colorFromIdentifier(id);

                UPDATE_STENCIL_IDX(m_interfacedObjectId, stencilGC)
                gdk_draw_arc(GDK_DRAWABLE(m_stencilBuffer), stencilGC, TRUE, left, top, circleSize * 2, circleSize * 2, 0, 64 * 360);
                m_interfacedObjects[m_interfacedObjectId] = CInterfacedObject(box.getIdentifier(), Box_ScenarioOutput, i);

                drawCircleWithBorder(widget, drawGC, color, gColors[Color_BoxOutputBorder], left, top, circleSize * 2);
                // Draw the text indicating the scenario output index
                // This is somewhat the bottom of the triangle indicating a box output
                drawScenarioTextIOIndex(widget, drawGC, j, left + marginX, top + marginY,
                                        startX + int(i) * (circleSpace + circleSize) + offset + (circleSize >> 1), top,
                                        startX + int(i) * (circleSpace + circleSize) + offset + (circleSize >> 1), startY + (circleSize >> 2) + sizeY + 2);
            }
        }
    }

    // Draw labels
    PangoContext* ctx   = gtk_widget_get_pango_context(widget);
    PangoLayout* layout = pango_layout_new(ctx);

    // Draw box label
    PangoRectangle labelRect;
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_markup(layout, proxy.GetLabel(), -1);
    pango_layout_get_pixel_extents(layout, nullptr, &labelRect);
    gdk_draw_layout(widget->window, widget->style->text_gc[GTK_WIDGET_STATE(widget)], startX + marginX, startY + marginY, layout);

    // Draw box status label
    PangoRectangle statusRect;
    pango_layout_set_markup(layout, proxy.GetStatusLabel(), -1);
    pango_layout_get_pixel_extents(layout, nullptr, &statusRect);
    int shiftX = (std::max(labelRect.width, statusRect.width) - std::min(labelRect.width, statusRect.width)) / 2;

    UPDATE_STENCIL_IDX(m_interfacedObjectId, stencilGC)
    gdk_draw_rectangle(GDK_DRAWABLE(m_stencilBuffer), stencilGC, TRUE, startX + shiftX + marginX, startY + labelRect.height + marginY, statusRect.width,
                       statusRect.height);
    m_interfacedObjects[m_interfacedObjectId] = CInterfacedObject(box.getIdentifier(), Box_Update, 0);
    gdk_draw_layout(widget->window, widget->style->text_gc[GTK_WIDGET_STATE(widget)], startX + shiftX + marginX, startY + labelRect.height + marginY, layout);

    g_object_unref(layout);
    g_object_unref(drawGC);
    g_object_unref(stencilGC);
}

void CInterfacedScenario::Redraw(const Kernel::IComment& comment)
{
    GtkWidget* widget = GTK_WIDGET(m_scenarioDrawingArea);
    GdkGC* stencilGC  = gdk_gc_new(GDK_DRAWABLE(m_stencilBuffer));
    GdkGC* drawGC     = gdk_gc_new(widget->window);

    // size_t i;
    const int marginX = static_cast<const int>(round(16 * m_currentScale));
    const int marginY = static_cast<const int>(round(16 * m_currentScale));

    const CCommentProxy proxy(m_kernelCtx, comment);
    const int sizeX  = proxy.GetWidth(GTK_WIDGET(m_scenarioDrawingArea)) + marginX * 2;
    const int sizeY  = proxy.GetHeight(GTK_WIDGET(m_scenarioDrawingArea)) + marginY * 2;
    const int startX = int(round(proxy.GetXCenter() * m_currentScale + m_viewOffsetX - (sizeX >> 1)));
    const int startY = int(round(proxy.GetYCenter() * m_currentScale + m_viewOffsetY - (sizeY >> 1)));

    UPDATE_STENCIL_IDX(m_interfacedObjectId, stencilGC)
    GDKDrawRoundedRectangle(GDK_DRAWABLE(m_stencilBuffer), stencilGC, TRUE, startX, startY, sizeX, sizeY, gint(round(16.0 * m_currentScale)));
    m_interfacedObjects[m_interfacedObjectId] = CInterfacedObject(comment.getIdentifier());

    gdk_gc_set_rgb_fg_color(drawGC, &gColors[m_SelectedObjects.count(comment.getIdentifier()) ? Color_CommentBackgroundSelected : Color_CommentBackground]);
    GDKDrawRoundedRectangle(widget->window, drawGC, TRUE, startX, startY, sizeX, sizeY, gint(round(16.0 * m_currentScale)));
    gdk_gc_set_rgb_fg_color(drawGC, &gColors[m_SelectedObjects.count(comment.getIdentifier()) ? Color_CommentBorderSelected : Color_CommentBorder]);
    GDKDrawRoundedRectangle(widget->window, drawGC, FALSE, startX, startY, sizeX, sizeY, gint(round(16.0 * m_currentScale)));

    PangoContext* ctx   = gtk_widget_get_pango_context(widget);
    PangoLayout* layout = pango_layout_new(ctx);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    if (pango_parse_markup(comment.getText().toASCIIString(), -1, 0, nullptr, nullptr, nullptr, nullptr)) {
        pango_layout_set_markup(layout, comment.getText().toASCIIString(), -1);
    }
    else { pango_layout_set_text(layout, comment.getText().toASCIIString(), -1); }
    gdk_draw_layout(widget->window, widget->style->text_gc[GTK_WIDGET_STATE(widget)], startX + marginX, startY + marginY, layout);
    g_object_unref(layout);

    g_object_unref(drawGC);
    g_object_unref(stencilGC);
}

void CInterfacedScenario::Redraw(const Kernel::ILink& link)
{
    const GtkWidget* widget = GTK_WIDGET(m_scenarioDrawingArea);
    GdkGC* stencilGC        = gdk_gc_new(GDK_DRAWABLE(m_stencilBuffer));
    GdkGC* drawGC           = gdk_gc_new(widget->window);

    const CLinkProxy proxy(link);

    CIdentifier srcOutputTypeID;
    CIdentifier dstInputTypeID;

    m_Scenario.getBoxDetails(link.getSourceBoxIdentifier())->getOutputType(link.getSourceBoxOutputIndex(), srcOutputTypeID);
    m_Scenario.getBoxDetails(link.getTargetBoxIdentifier())->getInputType(link.getTargetBoxInputIndex(), dstInputTypeID);

    if (link.hasAttribute(OV_AttributeId_Link_Invalid)) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_LinkInvalid]); }
    else if (m_SelectedObjects.count(link.getIdentifier())) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_LinkSelected]); }
    else if (dstInputTypeID == srcOutputTypeID) { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_Link]); }
    else {
        if (m_kernelCtx.getTypeManager().isDerivedFromStream(srcOutputTypeID, dstInputTypeID)) {
            gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_LinkDownCast]);
        }
        else if (m_kernelCtx.getTypeManager().isDerivedFromStream(dstInputTypeID, srcOutputTypeID)) {
            gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_LinkUpCast]);
        }
        else { gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_LinkInvalid]); }
    }

    UPDATE_STENCIL_IDX(m_interfacedObjectId, stencilGC)
    gdk_draw_line(GDK_DRAWABLE(m_stencilBuffer), stencilGC, proxy.GetXSource() + m_viewOffsetX, proxy.GetYSource() + m_viewOffsetY,
                  proxy.GetXTarget() + m_viewOffsetX, proxy.GetYTarget() + m_viewOffsetY);
    gdk_draw_line(widget->window, drawGC, proxy.GetXSource() + m_viewOffsetX, proxy.GetYSource() + m_viewOffsetY, proxy.GetXTarget() + m_viewOffsetX,
                  proxy.GetYTarget() + m_viewOffsetY);
    m_interfacedObjects[m_interfacedObjectId] = CInterfacedObject(link.getIdentifier(), Box_Link, 0);

    g_object_unref(drawGC);
    g_object_unref(stencilGC);
}


#undef UPDATE_STENCIL_IDX

size_t CInterfacedScenario::PickInterfacedObject(const int x, const int y) const
{
    if (!GDK_DRAWABLE(m_stencilBuffer)) { return size_t(0xffffffff); }

    int maxX;
    int maxY;
    uint32_t res = 0xffffffff;
    gdk_drawable_get_size(GDK_DRAWABLE(m_stencilBuffer), &maxX, &maxY);
    if (x >= 0 && y >= 0 && x < maxX && y < maxY) {
        GdkPixbuf* pixbuf = gdk_pixbuf_get_from_drawable(nullptr, GDK_DRAWABLE(m_stencilBuffer), nullptr, x, y, 0, 0, 1, 1);
        if (!pixbuf) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_ImportantWarning <<
                    "Could not get pixbuf from stencil buffer - couldn't pick object... this should never happen !\n";
            return size_t(0xffffffff);
        }

        guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
        if (!pixels) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_ImportantWarning <<
                    "Could not get pixels from pixbuf - couldn't pick object... this should never happen !\n";
            return 0xffffffff;
        }

        res = 0;
        res += (pixels[0] << 16);
        res += (pixels[1] << 8);
        res += (pixels[2]);
        g_object_unref(pixbuf);
    }
    return size_t(res);
}

bool CInterfacedScenario::PickInterfacedObject(const int x, const int y, int sizeX, int sizeY)
{
    if (!GDK_DRAWABLE(m_stencilBuffer)) {
        // m_kernelCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "No stencil buffer defined - couldn't pick object... this should never happen !\n";
        return false;
    }

    int maxX;
    int maxY;
    gdk_drawable_get_size(GDK_DRAWABLE(m_stencilBuffer), &maxX, &maxY);

    int startX = x;
    int startY = y;
    int endX   = x + sizeX;
    int endY   = y + sizeY;

    // crops according to drawing area boundings
    if (startX < 0) { startX = 0; }
    if (startY < 0) { startY = 0; }
    if (endX < 0) { endX = 0; }
    if (endY < 0) { endY = 0; }
    if (startX >= maxX - 1) { startX = maxX - 1; }
    if (startY >= maxY - 1) { startY = maxY - 1; }
    if (endX >= maxX - 1) { endX = maxX - 1; }
    if (endY >= maxY - 1) { endY = maxY - 1; }

    // recompute new size
    sizeX = endX - startX + 1;
    sizeY = endY - startY + 1;

    GdkPixbuf* pixbuf = gdk_pixbuf_get_from_drawable(nullptr, GDK_DRAWABLE(m_stencilBuffer), nullptr, startX, startY, 0, 0, sizeX, sizeY);
    if (!pixbuf) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_ImportantWarning
                << "Could not get pixbuf from stencil buffer - couldn't pick object... this should never happen !\n";
        return false;
    }

    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    if (!pixels) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_ImportantWarning
                << "Could not get pixels from pixbuf - couldn't pick object... this should never happen !\n";
        return false;
    }

    const int nRowBytes = gdk_pixbuf_get_rowstride(pixbuf);
    const int nChannel  = gdk_pixbuf_get_n_channels(pixbuf);
    for (int j = 0; j < sizeY; ++j) {
        for (int i = 0; i < sizeX; ++i) {
            size_t idx = 0;
            idx += (pixels[j * nRowBytes + i * nChannel + 0] << 16);
            idx += (pixels[j * nRowBytes + i * nChannel + 1] << 8);
            idx += (pixels[j * nRowBytes + i * nChannel + 2]);
            if (m_interfacedObjects[idx].m_ID != CIdentifier::undefined()) { m_SelectedObjects.insert(m_interfacedObjects[idx].m_ID); }
        }
    }

    g_object_unref(pixbuf);
    return true;
}

#define OV_ClassId_Selected CIdentifier(0xC67A01DC, 0x28CE06C1)

void CInterfacedScenario::UndoCB(const bool manageModifiedStatusFlag)
{
    // When a box gets updated we generate a snapshot beforehand to enable undo in all cases
    // This will result in two indentical undo states, in order to avoid weird Redo, we drop the
    // reduntant state at this moment
    bool shouldDropLastState = false;
    if (m_Scenario.containsBoxWithDeprecatedInterfacors()) { shouldDropLastState = true; }

    if (m_StateStack->Undo()) {
        CIdentifier id;
        m_SelectedObjects.clear();
        while ((id = m_Scenario.getNextBoxIdentifier(id)) != CIdentifier::undefined()) {
            if (m_Scenario.getBoxDetails(id)->hasAttribute(OV_ClassId_Selected)) { m_SelectedObjects.insert(id); }
        }
        while ((id = m_Scenario.getNextLinkIdentifier(id)) != CIdentifier::undefined()) {
            if (m_Scenario.getLinkDetails(id)->hasAttribute(OV_ClassId_Selected)) { m_SelectedObjects.insert(id); }
        }

        if (m_DesignerVisualization) { m_DesignerVisualization->Load(); }
        if (manageModifiedStatusFlag) { m_HasBeenModified = true; }

        this->RedrawScenarioSettings();
        this->RedrawScenarioInputSettings();
        this->RedrawScenarioOutputSettings();

        this->Redraw();

        if (shouldDropLastState) { m_StateStack->DropLastState(); }

        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(this->m_Application.m_Builder, "openvibe-button_redo")), m_StateStack->IsRedoPossible());
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(this->m_Application.m_Builder, "openvibe-button_undo")), m_StateStack->IsUndoPossible());
    }
    else {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Can not undo\n";
        GtkWidget* undoButton = GTK_WIDGET(gtk_builder_get_object(this->m_Application.m_Builder, "openvibe-button_undo"));
        gtk_widget_set_sensitive(undoButton, false);
    }
}

void CInterfacedScenario::RedoCB(const bool manageModifiedStatusFlag)
{
    if (m_StateStack->Redo()) {
        CIdentifier id;
        m_SelectedObjects.clear();
        while ((id = m_Scenario.getNextBoxIdentifier(id)) != CIdentifier::undefined()) {
            if (m_Scenario.getBoxDetails(id)->hasAttribute(OV_ClassId_Selected)) { m_SelectedObjects.insert(id); }
        }
        while ((id = m_Scenario.getNextLinkIdentifier(id)) != CIdentifier::undefined()) {
            if (m_Scenario.getLinkDetails(id)->hasAttribute(OV_ClassId_Selected)) { m_SelectedObjects.insert(id); }
        }

        if (m_DesignerVisualization) { m_DesignerVisualization->Load(); }

        if (manageModifiedStatusFlag) { m_HasBeenModified = true; }
        this->RedrawScenarioSettings();
        this->RedrawScenarioInputSettings();
        this->RedrawScenarioOutputSettings();

        this->Redraw();
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(this->m_Application.m_Builder, "openvibe-button_redo")), m_StateStack->IsRedoPossible());
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(this->m_Application.m_Builder, "openvibe-button_undo")), m_StateStack->IsUndoPossible());
    }
    else {
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(this->m_Application.m_Builder, "openvibe-button_redo")), false);
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Can not redo\n";
    }
}

void CInterfacedScenario::SnapshotCB(const bool manageModifiedStatusFlag)
{
    if (m_Scenario.containsBoxWithDeprecatedInterfacors()) {
        OV_WARNING("Scenario containing boxes with deprecated I/O or Settings does not support undo", m_kernelCtx.getLogManager());
    }
    else {
        CIdentifier id;

        while ((id = m_Scenario.getNextBoxIdentifier(id)) != CIdentifier::undefined()) {
            if (m_SelectedObjects.count(id)) { m_Scenario.getBoxDetails(id)->addAttribute(OV_ClassId_Selected, ""); }
            else { m_Scenario.getBoxDetails(id)->removeAttribute(OV_ClassId_Selected); }
        }
        while ((id = m_Scenario.getNextLinkIdentifier(id)) != CIdentifier::undefined()) {
            if (m_SelectedObjects.count(id)) { m_Scenario.getLinkDetails(id)->addAttribute(OV_ClassId_Selected, ""); }
            else { m_Scenario.getLinkDetails(id)->removeAttribute(OV_ClassId_Selected); }
        }

        if (manageModifiedStatusFlag) { m_HasBeenModified = true; }
        this->UpdateScenarioLabel();
        m_StateStack->Snapshot();
    }
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(this->m_Application.m_Builder, "openvibe-button_redo")), m_StateStack->IsRedoPossible());
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(this->m_Application.m_Builder, "openvibe-button_undo")), m_StateStack->IsUndoPossible());
}

void CInterfacedScenario::AddCommentCB(int x, int y)
{
    CIdentifier id;
    m_Scenario.addComment(id, CIdentifier::undefined());
    if (x == -1 || y == -1) {
        GtkWidget* scrolledWindow  = gtk_widget_get_parent(gtk_widget_get_parent(GTK_WIDGET(m_scenarioDrawingArea)));
        GtkAdjustment* adjustmentH = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolledWindow));
        GtkAdjustment* adjustmentV = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolledWindow));

#if defined TARGET_OS_Linux && !defined TARGET_OS_MacOS
        x = int(gtk_adjustment_get_value(adjustmentH) + gtk_adjustment_get_page_size(adjustmentH) / 2);
        y = int(gtk_adjustment_get_value(adjustmentV) + gtk_adjustment_get_page_size(adjustmentV) / 2);
#elif defined TARGET_OS_Windows
        gint wx, wy;
        ::gdk_window_get_size(gtk_widget_get_parent(GTK_WIDGET(m_scenarioDrawingArea))->window, &wx, &wy);
        x = int(gtk_adjustment_get_value(adjustmentH) + int(wx / 2));
        y = int(gtk_adjustment_get_value(adjustmentV) + int(wy / 2));
#else
        x = int(gtk_adjustment_get_value(adjustmentH) + 32);
        y = int(gtk_adjustment_get_value(adjustmentV) + 32);
#endif
    }

    CCommentProxy proxy(m_kernelCtx, m_Scenario, id);
    proxy.SetCenter(x - m_viewOffsetX, y - m_viewOffsetY);

    // Aligns comemnts on grid
    proxy.SetCenter(int((proxy.GetXCenter() + 8) & 0xfffffff0L), int((proxy.GetYCenter() + 8) & 0xfffffff0L));

    // Applies modifications before snapshot
    proxy.Apply();

    CCommentEditorDialog dialog(m_kernelCtx, *m_Scenario.getCommentDetails(id), m_guiFilename.c_str());
    if (!dialog.Run()) { m_Scenario.removeComment(id); }
    else {
        m_SelectedObjects.clear();
        m_SelectedObjects.insert(id);

        this->SnapshotCB();
    }

    this->Redraw();
}

void CInterfacedScenario::ConfigureScenarioSettingsCB()
{
    this->SnapshotCB();

    // construct the dialog
    this->RedrawConfigureScenarioSettingsDialog();

    gtk_widget_show_all(m_settingsVBox);

    const gint response = gtk_dialog_run(GTK_DIALOG(m_configureSettingsDialog));

    if (response == GTK_RESPONSE_CANCEL) { this->UndoCB(false); }
    else { this->SnapshotCB(); }

    gtk_widget_hide(m_configureSettingsDialog);
    this->RedrawScenarioSettings();
}

void CInterfacedScenario::AddScenarioSettingCB()
{
    const std::string name = "Setting " + std::to_string(m_Scenario.getSettingCount() + 1);
    m_Scenario.addSetting(name.c_str(), OVTK_TypeId_Integer, "0", size_t(-1), false, m_Scenario.getUnusedSettingIdentifier(CIdentifier::undefined()));

    this->RedrawConfigureScenarioSettingsDialog();
}

void CInterfacedScenario::AddScenarioInputCB()
{
    const std::string name = "Input " + std::to_string(m_Scenario.getInputCount() + 1);
    // scenario I/O are identified by name/type combination value, at worst uniq in the scope of the inputs of the box.
    m_Scenario.addInput(name.c_str(), OVTK_TypeId_StreamedMatrix, m_Scenario.getUnusedInputIdentifier(CIdentifier::undefined()));

    CConnectorEditor editor(m_kernelCtx, m_Scenario, Box_Input, m_Scenario.getInputCount() - 1, "Add Input", m_guiFilename.c_str());
    if (editor.Run()) { this->SnapshotCB(); }
    else { m_Scenario.removeInput(m_Scenario.getInputCount() - 1); }

    this->RedrawScenarioInputSettings();
}

void CInterfacedScenario::EditScenarioInputCB(const size_t index)
{
    CConnectorEditor editor(m_kernelCtx, m_Scenario, Box_Input, index, "Edit Input", m_guiFilename.c_str());
    if (editor.Run()) { this->SnapshotCB(); }

    this->RedrawScenarioInputSettings();
}

void CInterfacedScenario::AddScenarioOutputCB()
{
    const std::string name = "Output " + std::to_string(m_Scenario.getOutputCount() + 1);
    // scenario I/O are identified by name/type combination value, at worst uniq in the scope of the outputs of the box.
    m_Scenario.addOutput(name.c_str(), OVTK_TypeId_StreamedMatrix, m_Scenario.getUnusedOutputIdentifier(CIdentifier::undefined()));

    CConnectorEditor editor(m_kernelCtx, m_Scenario, Box_Output, m_Scenario.getOutputCount() - 1, "Add Output", m_guiFilename.c_str());
    if (editor.Run()) { this->SnapshotCB(); }
    else { m_Scenario.removeOutput(m_Scenario.getOutputCount() - 1); }

    this->RedrawScenarioOutputSettings();
}

void CInterfacedScenario::EditScenarioOutputCB(const size_t index)
{
    CConnectorEditor editor(m_kernelCtx, m_Scenario, Box_Output, index, "Edit Output", m_guiFilename.c_str());
    if (editor.Run()) { this->SnapshotCB(); }

    this->RedrawScenarioOutputSettings();
}

void CInterfacedScenario::SwapScenarioSettings(const size_t indexA, const size_t indexB)
{
    m_Scenario.swapSettings(indexA, indexB);
    this->RedrawConfigureScenarioSettingsDialog();
}

void CInterfacedScenario::SwapScenarioInputs(const size_t indexA, const size_t indexB)
{
    CIdentifier idA, idB;
    size_t idxA, idxB;

    m_Scenario.getScenarioInputLink(indexA, idA, idxA);
    m_Scenario.getScenarioInputLink(indexB, idB, idxB);

    m_Scenario.swapInputs(indexA, indexB);

    m_Scenario.setScenarioInputLink(indexB, idA, idxA);
    m_Scenario.setScenarioInputLink(indexA, idB, idxB);

    this->RedrawScenarioInputSettings();
    this->Redraw();
}

void CInterfacedScenario::SwapScenarioOutputs(const size_t indexA, const size_t indexB)
{
    CIdentifier idA, idB;
    size_t idxA, idxB;

    m_Scenario.getScenarioOutputLink(indexA, idA, idxA);
    m_Scenario.getScenarioOutputLink(indexB, idB, idxB);

    m_Scenario.swapOutputs(indexA, indexB);

    m_Scenario.setScenarioOutputLink(indexB, idA, idxA);
    m_Scenario.setScenarioOutputLink(indexA, idB, idxB);

    this->RedrawScenarioOutputSettings();
    this->Redraw();
}

void CInterfacedScenario::ScenarioDrawingAreaExposeCB(GdkEventExpose* /*event*/)
{
    if (m_currentMode == Mode_None) {
        gint viewportX = -1, viewportY = -1;

        gint minX = 0x7fff, maxX = -0x7fff;
        gint minY = 0x7fff, maxY = -0x7fff;

        const gint marginX = gint(round(32.0 * m_currentScale));
        const gint marginY = gint(round(32.0 * m_currentScale));

        CIdentifier id;
        while ((id = m_Scenario.getNextBoxIdentifier(id)) != CIdentifier::undefined()) {
            //CBoxProxy proxy(m_kernelCtx, *m_scenario.getBoxDetails(l_oBoxID));
            CBoxProxy proxy(m_kernelCtx, m_Scenario, id);
            minX = std::min(minX, gint((proxy.GetXCenter() - 1.0 * proxy.GetWidth(GTK_WIDGET(m_scenarioDrawingArea)) / 2) * m_currentScale));
            maxX = std::max(maxX, gint((proxy.GetXCenter() + 1.0 * proxy.GetWidth(GTK_WIDGET(m_scenarioDrawingArea)) / 2) * m_currentScale));
            minY = std::min(minY, gint((proxy.GetYCenter() - 1.0 * proxy.GetHeight(GTK_WIDGET(m_scenarioDrawingArea)) / 2) * m_currentScale));
            maxY = std::max(maxY, gint((proxy.GetYCenter() + 1.0 * proxy.GetHeight(GTK_WIDGET(m_scenarioDrawingArea)) / 2) * m_currentScale));
        }

        while ((id = m_Scenario.getNextCommentIdentifier(id)) != CIdentifier::undefined()) {
            CCommentProxy proxy(m_kernelCtx, *m_Scenario.getCommentDetails(id));
            minX = std::min(minX, gint((proxy.GetXCenter() - 1.0 * proxy.GetWidth(GTK_WIDGET(m_scenarioDrawingArea)) / 2) * m_currentScale));
            maxX = std::max(maxX, gint((proxy.GetXCenter() + 1.0 * proxy.GetWidth(GTK_WIDGET(m_scenarioDrawingArea)) / 2) * m_currentScale));
            minY = std::min(minY, gint((proxy.GetYCenter() - 1.0 * proxy.GetHeight(GTK_WIDGET(m_scenarioDrawingArea)) / 2) * m_currentScale));
            maxY = std::max(maxY, gint((proxy.GetYCenter() + 1.0 * proxy.GetHeight(GTK_WIDGET(m_scenarioDrawingArea)) / 2) * m_currentScale));
        }

        const gint newSizeX = maxX - minX;
        const gint newSizeY = maxY - minY;
        gint oldSizeX       = -1;
        gint oldSizeY       = -1;

        gdk_window_get_size(GTK_WIDGET(m_scenarioViewport)->window, &viewportX, &viewportY);
        gtk_widget_get_size_request(GTK_WIDGET(m_scenarioDrawingArea), &oldSizeX, &oldSizeY);

        if (newSizeX >= 0 && newSizeY >= 0) {
            if (oldSizeX != newSizeX + 2 * marginX || oldSizeY != newSizeY + 2 * marginY) {
                gtk_widget_set_size_request(GTK_WIDGET(m_scenarioDrawingArea), newSizeX + 2 * marginX, newSizeY + 2 * marginY);
            }
            m_viewOffsetX = std::min(m_viewOffsetX, -maxX - marginX + std::max(viewportX, newSizeX + 2 * marginX));
            m_viewOffsetX = std::max(m_viewOffsetX, -minX + marginX);
            m_viewOffsetY = std::min(m_viewOffsetY, -maxY - marginY + std::max(viewportY, newSizeY + 2 * marginY));
            m_viewOffsetY = std::max(m_viewOffsetY, -minY + marginY);
        }
    }

    gint x, y;

    gdk_window_get_size(GTK_WIDGET(m_scenarioDrawingArea)->window, &x, &y);
    if (m_stencilBuffer) { g_object_unref(m_stencilBuffer); }
    m_stencilBuffer = gdk_pixmap_new(GTK_WIDGET(m_scenarioDrawingArea)->window, x, y, -1);

    GdkGC* stencilGC = gdk_gc_new(m_stencilBuffer);
    GdkColor color   = { 0, 0, 0, 0 };
    gdk_gc_set_rgb_fg_color(stencilGC, &color);
    gdk_draw_rectangle(GDK_DRAWABLE(m_stencilBuffer), stencilGC, TRUE, 0, 0, x, y);
    g_object_unref(stencilGC);

    if (this->IsLocked()) {
        color.pixel = 0;
        color.red   = 0x0f00;
        color.green = 0x0f00;
        color.blue  = 0x0f00;

        GdkGC* drawGC = gdk_gc_new(GTK_WIDGET(m_scenarioDrawingArea)->window);
        gdk_gc_set_rgb_fg_color(drawGC, &color);
        gdk_gc_set_function(drawGC, GDK_XOR);
        gdk_draw_rectangle(GTK_WIDGET(m_scenarioDrawingArea)->window, drawGC, TRUE, 0, 0, x, y);
        g_object_unref(drawGC);
    }
    // TODO: optimize this as this will be called endlessly
    /*
    else if (false) //m_scenario.containsBoxWithDeprecatedInterfacors() 
    {
        color.pixel = 0;
        color.red = 0xffff;
        color.green = 0xefff;
        color.blue = 0xefff;

        GdkGC* drawGC = gdk_gc_new(GTK_WIDGET(m_scenarioDrawingArea)->window);
        gdk_gc_set_rgb_fg_color(drawGC, &color);
        gdk_gc_set_function(drawGC, GDK_AND);
        gdk_draw_rectangle(GTK_WIDGET(m_pScenarioDrawingArea)->window, drawGC, TRUE, 0, 0, x, y);
        g_object_unref(l_pDrawGC);
    }
    */
    m_interfacedObjectId = 0;
    m_interfacedObjects.clear();

    size_t count = 0;
    CIdentifier id;
    while ((id = m_Scenario.getNextCommentIdentifier(id)) != CIdentifier::undefined()) {
        Redraw(*m_Scenario.getCommentDetails(id));
        count++;
    }
    m_nComment = count;

    count = 0;
    while ((id = m_Scenario.getNextBoxIdentifier(id)) != CIdentifier::undefined()) {
        Redraw(*m_Scenario.getBoxDetails(id));
        count++;
    }
    m_nBox = count;

    count = 0;
    while ((id = m_Scenario.getNextLinkIdentifier(id)) != CIdentifier::undefined()) {
        Redraw(*m_Scenario.getLinkDetails(id));
        count++;
    }
    m_nLink = count;

    if (m_currentMode == Mode_Selection || m_currentMode == Mode_SelectionAdd) {
        const int startX = int(std::min(m_pressMouseX, m_currentMouseX));
        const int startY = int(std::min(m_pressMouseY, m_currentMouseY));
        const int sizeX  = int(std::max(m_pressMouseX - m_currentMouseX, m_currentMouseX - m_pressMouseX));
        const int sizeY  = int(std::max(m_pressMouseY - m_currentMouseY, m_currentMouseY - m_pressMouseY));

        const GtkWidget* widget = GTK_WIDGET(m_scenarioDrawingArea);
        GdkGC* drawGC           = gdk_gc_new(widget->window);
        gdk_gc_set_function(drawGC, GDK_OR);
        gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_SelectionArea]);
        gdk_draw_rectangle(widget->window, drawGC, TRUE, startX, startY, sizeX, sizeY);
        gdk_gc_set_function(drawGC, GDK_COPY);
        gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_SelectionAreaBorder]);
        gdk_draw_rectangle(widget->window, drawGC, FALSE, startX, startY, sizeX, sizeY);
        g_object_unref(drawGC);
    }

    if (m_currentMode == Mode_Connect) {
        const GtkWidget* widget = GTK_WIDGET(m_scenarioDrawingArea);
        GdkGC* drawGC           = gdk_gc_new(widget->window);

        gdk_gc_set_rgb_fg_color(drawGC, &gColors[Color_Link]);
        gdk_draw_line(widget->window, drawGC, int(m_pressMouseX), int(m_pressMouseY), int(m_currentMouseX), int(m_currentMouseY));
        g_object_unref(drawGC);
    }
}

// This method inserts a box into the scenario upon receiving data
void CInterfacedScenario::ScenarioDrawingAreaDragDataReceivedCB(GdkDragContext* dc, const gint x, const gint y, GtkSelectionData* selectionData,
                                                                guint /*info*/, guint /*t*/)
{
    if (this->IsLocked()) { return; }

    // two cases: dragged from inside the program = a box ...
    if (dc->protocol == GDK_DRAG_PROTO_LOCAL || dc->protocol == GDK_DRAG_PROTO_XDND || dc->protocol == GDK_DRAG_PROTO_MOTIF) {
        CIdentifier boxID;
        CIdentifier boxAlgorithmClassID;

        // The drag data only contains one string, for a normal box this string is its algorithmClassIdentifier
        // However since all metaboxes have the same identifier, we have added the 'identifier' of a metabox after this string
        // The identifier itself is the name of the scenario which created the metabox
        std::string str(reinterpret_cast<const char*>(gtk_selection_data_get_text(selectionData)));

        // check that there is an identifier inside the string, its form is (0xXXXXXXXX, 0xXXXXXXXX)
        if (str.find(')') != std::string::npos) { boxAlgorithmClassID.fromString(str.substr(0, str.find(')'))); }

        Kernel::IBox* box                     = nullptr;
        const Plugins::IPluginObjectDesc* pod = nullptr;

        if (boxAlgorithmClassID == CIdentifier::undefined()) {
            m_currentMouseX = x;
            m_currentMouseY = y;
            return;
        }
        if (boxAlgorithmClassID == OVP_ClassId_BoxAlgorithm_Metabox) {
            // extract the name of the metabox from the drag data string
            CIdentifier id;
            id.fromString(str.substr(str.find(')') + 1));

            //m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "This is a metabox with ID " << metaboxID << "\n";
            pod = m_kernelCtx.getMetaboxManager().getMetaboxObjectDesc(id);

            // insert a box into the scenario, initialize it from the proxy-descriptor from the metabox loader
            m_Scenario.addBox(boxID, *dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(pod), CIdentifier::undefined());

            box = m_Scenario.getBoxDetails(boxID);
            box->addAttribute(OVP_AttributeId_Metabox_ID, id.toString());
        }
        else {
            m_Scenario.addBox(boxID, boxAlgorithmClassID, CIdentifier::undefined());

            box                  = m_Scenario.getBoxDetails(boxID);
            const CIdentifier id = box->getAlgorithmClassIdentifier();
            pod                  = m_kernelCtx.getPluginManager().getPluginObjectDescCreating(id);
        }

        m_SelectedObjects.clear();
        m_SelectedObjects.insert(boxID);

        // If a visualization box was dropped, add it in window manager
        if (pod && pod->hasFunctionality(Plugins::EPluginFunctionality::Visualization)) {
            // Let window manager know about new box
            if (m_DesignerVisualization) { m_DesignerVisualization->OnVisualizationBoxAdded(box); }
        }

        CBoxProxy proxy(m_kernelCtx, m_Scenario, boxID);
        proxy.SetCenter(x - m_viewOffsetX, y - m_viewOffsetY);
        // Aligns boxes on grid
        proxy.SetCenter(int((proxy.GetXCenter() + 8) & 0xfffffff0L), int((proxy.GetYCenter() + 8) & 0xfffffff0L));

        // Applies modifications before snapshot
        proxy.Apply();

        this->SnapshotCB();

        m_currentMouseX = x;
        m_currentMouseY = y;
    }

    // ... or dragged from outside the application = a file
    // ONLY AVAILABLE ON WINDOWS (known d'n'd protocol)
#if defined TARGET_OS_Windows
    if (dc->protocol == GDK_DRAG_PROTO_WIN32_DROPFILES) {
        // we get the content of the buffer: the list of files URI:
        // file:///path/to/file.ext\r\n
        // file:///path/to/file.ext\r\n
        // ...
        const std::string draggedFilesPath(reinterpret_cast<const char*>(gtk_selection_data_get_data(selectionData)));
        std::stringstream ss(draggedFilesPath);
        std::string line;
        std::vector<std::string> filesToOpen;
        while (std::getline(ss, line)) {
            // the path starts with file:/// and ends with \r\n once parsed line after line, a \r remains on Windows
            line = line.substr(8, line.length() - 9);

            // uri to path (to remove %xx escape characters):
            line = g_uri_unescape_string(line.c_str(), nullptr);

            filesToOpen.push_back(line);
        }

        for (auto& file : filesToOpen) { m_Application.OpenScenario(file.c_str()); }
    }
#endif
}

void CInterfacedScenario::ScenarioDrawingAreaMotionNotifyCB(GtkWidget* /*widget*/, GdkEventMotion* event)
{
    // m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "scenarioDrawingAreaMotionNotifyCB\n";

    if (this->IsLocked()) { return; }

    GtkWidget* tooltip = GTK_WIDGET(gtk_builder_get_object(m_guiBuilder, "tooltip"));
    gtk_widget_set_name(tooltip, "gtk-tooltips");
    const size_t objIdx          = PickInterfacedObject(int(event->x), int(event->y));
    const CInterfacedObject& obj = m_interfacedObjects[objIdx];
    if (obj.m_ID != CIdentifier::undefined() && obj.m_ConnectorType != Box_Link && obj.m_ConnectorType != Box_None) {
        Kernel::IBox* boxDetails = m_Scenario.getBoxDetails(obj.m_ID);
        if (boxDetails) {
            CString name;
            CString type;
            if (obj.m_ConnectorType == Box_Input) {
                CIdentifier typeID;
                boxDetails->getInputName(obj.m_ConnectorIdx, name);
                boxDetails->getInputType(obj.m_ConnectorIdx, typeID);
                type = m_kernelCtx.getTypeManager().getTypeName(typeID);
                type = CString("[") + type + CString("]");
            }
            else if (obj.m_ConnectorType == Box_Output) {
                CIdentifier typeID;
                boxDetails->getOutputName(obj.m_ConnectorIdx, name);
                boxDetails->getOutputType(obj.m_ConnectorIdx, typeID);
                type = m_kernelCtx.getTypeManager().getTypeName(typeID);
                type = CString("[") + type + CString("]");
            }
            else if (obj.m_ConnectorType == Box_Update) {
                //m_scenario.updateBox(boxDetails->getIdentifier());
                name = CString("Right click for");
                type = "box update";
            }
            else if (obj.m_ConnectorType == Box_ScenarioInput) {
                CIdentifier typeID;
                boxDetails->getInputName(obj.m_ConnectorIdx, name);
                boxDetails->getInputType(obj.m_ConnectorIdx, typeID);

                for (size_t i = 0; i < m_Scenario.getInputCount(); i++) {
                    CIdentifier id;
                    size_t idx;
                    m_Scenario.getScenarioInputLink(i, id, idx);
                    if (id == boxDetails->getIdentifier() && idx == obj.m_ConnectorIdx) {
                        m_Scenario.getInputName(i, name);
                        name = CString("Connected to \n") + name;
                        m_Scenario.getInputType(i, typeID);
                    }
                }
                type = m_kernelCtx.getTypeManager().getTypeName(typeID);
                type = CString("[") + type + CString("]");
            }
            else if (obj.m_ConnectorType == Box_ScenarioOutput) {
                CIdentifier typeID;
                boxDetails->getOutputName(obj.m_ConnectorIdx, name);
                boxDetails->getOutputType(obj.m_ConnectorIdx, typeID);

                for (size_t i = 0; i < m_Scenario.getOutputCount(); i++) {
                    CIdentifier id;
                    size_t idx;
                    m_Scenario.getScenarioOutputLink(i, id, idx);
                    if (id == boxDetails->getIdentifier() && idx == obj.m_ConnectorIdx) {
                        m_Scenario.getOutputName(i, name);
                        name = CString("Connected to \n") + name;
                        m_Scenario.getOutputType(i, typeID);
                    }
                }
                type = m_kernelCtx.getTypeManager().getTypeName(typeID);
                type = CString("[") + type + CString("]");
            }

            gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(m_guiBuilder, "tooltip-label_name_content")), name);
            gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(m_guiBuilder, "tooltip-label_type_content")), type);
            gtk_window_move(GTK_WINDOW(tooltip), gint(event->x_root), gint(event->y_root) + 40);
            gtk_widget_show(tooltip);
        }
    }
    else { gtk_widget_hide(tooltip); }

    if (m_currentMode != Mode_None) {
        if (m_currentMode == Mode_MoveScenario) {
            m_viewOffsetX += int(event->x - m_currentMouseX);
            m_viewOffsetY += int(event->y - m_currentMouseY);
        }
        else if (m_currentMode == Mode_MoveSelection) {
            if (m_controlPressed) { m_SelectedObjects.insert(m_currentObject.m_ID); }
            else {
                if (!m_SelectedObjects.count(m_currentObject.m_ID)) {
                    m_SelectedObjects.clear();
                    m_SelectedObjects.insert(m_currentObject.m_ID);
                }
            }
            for (auto& id : m_SelectedObjects) {
                if (m_Scenario.isBox(id)) {
                    CBoxProxy proxy(m_kernelCtx, m_Scenario, id);
                    proxy.SetCenter(proxy.GetXCenter() + int(event->x - m_currentMouseX), proxy.GetYCenter() + int(event->y - m_currentMouseY));
                }
                if (m_Scenario.isComment(id)) {
                    CCommentProxy proxy(m_kernelCtx, m_Scenario, id);
                    proxy.SetCenter(proxy.GetXCenter() + int(event->x - m_currentMouseX), proxy.GetYCenter() + int(event->y - m_currentMouseY));
                }
            }
        }

        this->Redraw();
    }
    m_currentMouseX = event->x;
    m_currentMouseY = event->y;
}

namespace {
void GtkMenuAddSeparatorMenuItem(GtkMenu* menu)
{
    GtkSeparatorMenuItem* menuitem = GTK_SEPARATOR_MENU_ITEM(gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
}

GtkImageMenuItem* GtkMenuAddNewImageMenuItem(GtkMenu* menu, const char* icon, const char* label)
{
    GtkImageMenuItem* menuitem = GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new_with_label(label));
    gtk_image_menu_item_set_image(menuitem, gtk_image_new_from_stock(icon, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
    return menuitem;
}
}  // namespace

GtkImageMenuItem* CInterfacedScenario::addNewImageMenuItemWithCBGeneric(GtkMenu* menu, const char* icon, const char* label, const menu_cb_function_t cb,
                                                                        Kernel::IBox* box, const EContextMenu command, const size_t index, const size_t index2)
{
    GtkImageMenuItem* menuItem = GtkMenuAddNewImageMenuItem(menu, icon, label);
    box_ctx_menu_cb_t menuCB;
    menuCB.command        = command;
    menuCB.index          = index;
    menuCB.secondaryIndex = index2;
    menuCB.box            = box;
    menuCB.scenario       = this;
    const auto idx        = m_boxCtxMenuCBs.size();
    m_boxCtxMenuCBs[idx]  = menuCB;
    g_signal_connect(G_OBJECT(menuItem), "activate", G_CALLBACK(cb), &m_boxCtxMenuCBs[idx]);
    return menuItem;
}

void CInterfacedScenario::ScenarioDrawingAreaButtonPressedCB(GtkWidget* widget, GdkEventButton* event)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "scenarioDrawingAreaButtonPressedCB\n";

    if (this->IsLocked()) { return; }

    GtkWidget* tooltip = GTK_WIDGET(gtk_builder_get_object(m_guiBuilder, "tooltip"));
    gtk_widget_hide(tooltip);
    gtk_widget_grab_focus(widget);

    m_buttonPressed |= ((event->type == GDK_BUTTON_PRESS) && (event->button == 1));
    m_pressMouseX = event->x;
    m_pressMouseY = event->y;

    size_t objIdx   = PickInterfacedObject(int(m_pressMouseX), int(m_pressMouseY));
    m_currentObject = m_interfacedObjects[objIdx];

    if (event->button == 1) {
        if (event->type == GDK_BUTTON_PRESS)    // Simple click
        {
            if (m_currentObject.m_ID == CIdentifier::undefined()) {
                if (m_shiftPressed) { m_currentMode = Mode_MoveScenario; }
                else {
                    if (m_controlPressed) { m_currentMode = Mode_SelectionAdd; }
                    else { m_currentMode = Mode_Selection; }
                }
            }
            else {
                if (m_currentObject.m_ConnectorType == Box_Input || m_currentObject.m_ConnectorType == Box_Output) { m_currentMode = Mode_Connect; }
                else {
                    m_currentMode = Mode_MoveSelection;
                    /*
                    if (m_controlPressed) { m_interfacedObjects[m_currentObject.m_id]=!m_interfacedObjects[m_currentObject.m_id]; }
                    else
                    {
                        m_currentObject.clear();
                        m_currentObject[m_oCurrentObject.m_id]=true;
                    }
                    */
                }
            }
        }
        else if (event->type == GDK_2BUTTON_PRESS)  // Double click
        {
            if (m_currentObject.m_ID != CIdentifier::undefined()) {
                m_currentMode    = Mode_EditSettings;
                m_shiftPressed   = false;
                m_controlPressed = false;
                m_altPressed     = false;
                m_aPressed       = false;
                m_wPressed       = false;

                if (m_currentObject.m_ConnectorType == Box_Input || m_currentObject.m_ConnectorType == Box_Output) {
                    Kernel::IBox* box = m_Scenario.getBoxDetails(m_currentObject.m_ID);
                    if (box) {
                        if ((m_currentObject.m_ConnectorType == Box_Input && box->hasAttribute(OV_AttributeId_Box_FlagCanModifyInput))
                            || (m_currentObject.m_ConnectorType == Box_Output && box->hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput))) {
                            CConnectorEditor editor(m_kernelCtx, *box, m_currentObject.m_ConnectorType, m_currentObject.m_ConnectorIdx,
                                                    m_currentObject.m_ConnectorType == Box_Input ? "Edit Input" : "Edit Output", m_guiFilename.c_str());
                            if (editor.Run()) { this->SnapshotCB(); }
                        }
                    }
                }
                else {
                    if (m_Scenario.isBox(m_currentObject.m_ID)) {
                        Kernel::IBox* box = m_Scenario.getBoxDetails(m_currentObject.m_ID);
                        if (box) {
                            CBoxConfigurationDialog dialog(m_kernelCtx, *box, m_guiFilename.c_str(), m_guiSettingsFilename.c_str(), false);
                            if (dialog.Run()) { this->SnapshotCB(); }
                        }
                    }
                    if (m_Scenario.isComment(m_currentObject.m_ID)) {
                        Kernel::IComment* comment = m_Scenario.getCommentDetails(m_currentObject.m_ID);
                        if (comment) {
                            CCommentEditorDialog dialog(m_kernelCtx, *comment, m_guiFilename.c_str());
                            if (dialog.Run()) { this->SnapshotCB(); }
                        }
                    }
                }
            }
        }
    }
    else if (event->button == 3) // right click
    {
        if (event->type == GDK_BUTTON_PRESS) {
            const auto unused = size_t(-1);
            GtkMenu* menu     = GTK_MENU(gtk_menu_new());
            m_boxCtxMenuCBs.clear();

            // -------------- SELECTION -----------

            if (this->HasSelection()) { addNewImageMenuItemWithCB(menu, GTK_STOCK_CUT, "cut", MenuCB, nullptr, EContextMenu::SelectionCut, unused); }
            if (this->HasSelection()) { addNewImageMenuItemWithCB(menu, GTK_STOCK_COPY, "copy", MenuCB, nullptr, EContextMenu::SelectionCopy, unused); }
            if ((m_Application.m_ClipboardScenario->getNextBoxIdentifier(CIdentifier::undefined()) != CIdentifier::undefined())
                || (m_Application.m_ClipboardScenario->getNextCommentIdentifier(CIdentifier::undefined()) != CIdentifier::undefined())) {
                addNewImageMenuItemWithCB(menu, GTK_STOCK_PASTE, "paste", MenuCB, nullptr, EContextMenu::SelectionPaste, unused);
            }
            if (this->HasSelection()) { addNewImageMenuItemWithCB(menu, GTK_STOCK_DELETE, "delete", MenuCB, nullptr, EContextMenu::SelectionDelete, unused); }

            if (m_currentObject.m_ID != CIdentifier::undefined() && m_Scenario.isBox(m_currentObject.m_ID)) {
                Kernel::IBox* box = m_Scenario.getBoxDetails(m_currentObject.m_ID);
                if (box) {
                    if (!m_boxCtxMenuCBs.empty()) { GtkMenuAddSeparatorMenuItem(menu); }

                    bool toBeUpdated                  = box->hasAttribute(OV_AttributeId_Box_ToBeUpdated);
                    bool pendingDeprecatedInterfacors = box->hasAttribute(OV_AttributeId_Box_PendingDeprecatedInterfacors);

                    // -------------- INPUTS --------------
                    bool canAddInput             = box->hasAttribute(OV_AttributeId_Box_FlagCanAddInput);
                    bool canModifyInput          = box->hasAttribute(OV_AttributeId_Box_FlagCanModifyInput);
                    bool canConnectScenarioInput = (box->getInputCount() > 0 && m_Scenario.getInputCount() > 0);
                    if (!pendingDeprecatedInterfacors && !toBeUpdated && (canAddInput || canModifyInput || canConnectScenarioInput)) {
                        size_t nFixedInput = 0;
                        std::stringstream ss(box->getAttributeValue(OV_AttributeId_Box_InitialInputCount).toASCIIString());
                        ss >> nFixedInput;
                        GtkMenu* menuInput      = GTK_MENU(gtk_menu_new());
                        GtkImageMenuItem* input = GtkMenuAddNewImageMenuItem(menu, GTK_STOCK_PROPERTIES, "inputs");
                        for (size_t i = 0; i < box->getInputCount(); ++i) {
                            CString name;
                            CIdentifier typeID, id;
                            box->getInputName(i, name);
                            box->getInputType(i, typeID);
                            id                         = box->getIdentifier();
                            const std::string str      = std::to_string(i + 1) + " : " + name.toASCIIString();
                            GtkImageMenuItem* menuItem = GtkMenuAddNewImageMenuItem(menuInput, GTK_STOCK_PROPERTIES, str.c_str());

                            GtkMenu* menuAction = GTK_MENU(gtk_menu_new());

                            if (canConnectScenarioInput) {
                                for (size_t j = 0; j < m_Scenario.getInputCount(); ++j) {
                                    CString scenarioInputName;
                                    CIdentifier boxID, inputTypeID;
                                    auto idx = size_t(-1);
                                    m_Scenario.getInputName(j, scenarioInputName);
                                    m_Scenario.getInputType(j, inputTypeID);
                                    m_Scenario.getScenarioInputLink(j, boxID, idx);
                                    const std::string str2 = std::to_string(j + 1) + " : " + scenarioInputName.toASCIIString();
                                    if (boxID == id && idx == i) {
                                        addNewImageMenuItemWithCBGeneric(menuAction, GTK_STOCK_DISCONNECT, ("disconnect from " + str2).c_str(), MenuCB,
                                                                         box, EContextMenu::BoxDisconnectScenarioInput, i, j);
                                    }
                                    else {
                                        if (m_kernelCtx.getTypeManager().isDerivedFromStream(inputTypeID, typeID)) {
                                            addNewImageMenuItemWithCBGeneric(menuAction, GTK_STOCK_CONNECT, ("connect to " + str2).c_str(), MenuCB,
                                                                             box, EContextMenu::BoxConnectScenarioInput, i, j);
                                        }
                                    }
                                }
                            }

                            if (canModifyInput) {
                                addNewImageMenuItemWithCB(menuAction, GTK_STOCK_EDIT, "configure...", MenuCB, box, EContextMenu::BoxEditInput, i);
                            }

                            if (canAddInput && nFixedInput <= i) {
                                addNewImageMenuItemWithCB(menuAction, GTK_STOCK_REMOVE, "delete", MenuCB, box, EContextMenu::BoxRemoveInput, i);
                            }

                            if (GtkContainerGetChildrenCount(GTK_CONTAINER(menuAction)) > 0) {
                                gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), GTK_WIDGET(menuAction));
                            }
                            else { gtk_widget_set_sensitive(GTK_WIDGET(menuItem), false); }
                        }
                        GtkMenuAddSeparatorMenuItem(menuInput);
                        if (canAddInput) { addNewImageMenuItemWithCB(menuInput, GTK_STOCK_ADD, "new...", MenuCB, box, EContextMenu::BoxAddInput, unused); }
                        gtk_menu_item_set_submenu(GTK_MENU_ITEM(input), GTK_WIDGET(menuInput));
                    }

                    // -------------- OUTPUTS --------------

                    bool canAddOutput             = box->hasAttribute(OV_AttributeId_Box_FlagCanAddOutput);
                    bool canModifyOutput          = box->hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput);
                    bool canConnectScenarioOutput = (box->getOutputCount() > 0 && m_Scenario.getOutputCount() > 0);
                    if (!pendingDeprecatedInterfacors && !toBeUpdated && (canAddOutput || canModifyOutput || canConnectScenarioOutput)) {
                        size_t nFixedOutput = 0;
                        std::stringstream ss(box->getAttributeValue(OV_AttributeId_Box_InitialOutputCount).toASCIIString());
                        ss >> nFixedOutput;
                        GtkImageMenuItem* itemOutput = GtkMenuAddNewImageMenuItem(menu, GTK_STOCK_PROPERTIES, "outputs");
                        GtkMenu* menuOutput          = GTK_MENU(gtk_menu_new());
                        for (size_t i = 0; i < box->getOutputCount(); ++i) {
                            CString name;
                            CIdentifier typeID, id;
                            box->getOutputName(i, name);
                            box->getOutputType(i, typeID);
                            id                         = box->getIdentifier();
                            const std::string str      = std::to_string(i + 1) + " : " + name.toASCIIString();
                            GtkImageMenuItem* menuItem = GtkMenuAddNewImageMenuItem(menuOutput, GTK_STOCK_PROPERTIES, str.c_str());

                            GtkMenu* menuAction = GTK_MENU(gtk_menu_new());

                            if (canConnectScenarioOutput) {
                                for (size_t j = 0; j < m_Scenario.getOutputCount(); ++j) {
                                    CString scenarioOutputName;
                                    CIdentifier boxID, outputTypeID;
                                    auto idx = size_t(-1);
                                    m_Scenario.getOutputName(j, scenarioOutputName);
                                    m_Scenario.getOutputType(j, outputTypeID);
                                    m_Scenario.getScenarioOutputLink(j, boxID, idx);
                                    const std::string str2 = std::to_string(j + 1) + " : " + scenarioOutputName.toASCIIString();
                                    if (boxID == id && idx == i) {
                                        addNewImageMenuItemWithCBGeneric(menuAction, GTK_STOCK_DISCONNECT, ("disconnect from " + str2).c_str(),
                                                                         MenuCB, box, EContextMenu::BoxDisconnectScenarioOutput, i, j);
                                    }
                                    else if (m_kernelCtx.getTypeManager().isDerivedFromStream(typeID, outputTypeID)) {
                                        addNewImageMenuItemWithCBGeneric(menuAction, GTK_STOCK_CONNECT, ("connect to " + str2).c_str(),
                                                                         MenuCB, box, EContextMenu::BoxConnectScenarioOutput, i, j);
                                    }
                                }
                            }

                            if (canModifyOutput) {
                                addNewImageMenuItemWithCB(menuAction, GTK_STOCK_EDIT, "configure...", MenuCB, box, EContextMenu::BoxEditOutput, i);
                            }
                            if (canAddOutput && nFixedOutput <= i) {
                                addNewImageMenuItemWithCB(menuAction, GTK_STOCK_REMOVE, "delete", MenuCB, box, EContextMenu::BoxRemoveOutput, i);
                            }

                            if (GtkContainerGetChildrenCount(GTK_CONTAINER(menuAction)) > 0) {
                                gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), GTK_WIDGET(menuAction));
                            }
                            else { gtk_widget_set_sensitive(GTK_WIDGET(menuItem), false); }
                        }
                        GtkMenuAddSeparatorMenuItem(menuOutput);
                        if (canAddOutput) { addNewImageMenuItemWithCB(menuOutput, GTK_STOCK_ADD, "new...", MenuCB, box, EContextMenu::BoxAddOutput, unused); }
                        gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemOutput), GTK_WIDGET(menuOutput));
                    }

                    // -------------- SETTINGS --------------

                    bool canAddSetting    = box->hasAttribute(OV_AttributeId_Box_FlagCanAddSetting);
                    bool canModifySetting = box->hasAttribute(OV_AttributeId_Box_FlagCanModifySetting);
                    if (!pendingDeprecatedInterfacors && !toBeUpdated && (canAddSetting || canModifySetting)) {
                        size_t nFixedSetting = 0;
                        std::stringstream ss(box->getAttributeValue(OV_AttributeId_Box_InitialSettingCount).toASCIIString());
                        ss >> nFixedSetting;
                        GtkImageMenuItem* itemSetting = GtkMenuAddNewImageMenuItem(menu, GTK_STOCK_PROPERTIES, "modify settings");
                        GtkMenu* menuSetting          = GTK_MENU(gtk_menu_new());
                        for (size_t i = 0; i < box->getSettingCount(); ++i) {
                            CString name;
                            CIdentifier typeID;
                            box->getSettingName(i, name);
                            box->getSettingType(i, typeID);
                            const std::string str      = std::to_string(i + 1) + " : " + name.toASCIIString();
                            GtkImageMenuItem* menuItem = GtkMenuAddNewImageMenuItem(menuSetting, GTK_STOCK_PROPERTIES, str.c_str());

                            if (canModifySetting || nFixedSetting <= i) {
                                GtkMenu* menuAction = GTK_MENU(gtk_menu_new());
                                if (canModifySetting) {
                                    addNewImageMenuItemWithCB(menuAction, GTK_STOCK_EDIT, "configure...", MenuCB, box,
                                                              EContextMenu::BoxEditSetting, i);
                                }
                                if (canAddSetting && nFixedSetting <= i) {
                                    addNewImageMenuItemWithCB(menuAction, GTK_STOCK_REMOVE, "delete", MenuCB, box, EContextMenu::BoxRemoveSetting, i);
                                }
                                gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), GTK_WIDGET(menuAction));
                            }
                            else { gtk_widget_set_sensitive(GTK_WIDGET(menuItem), false); }
                        }
                        GtkMenuAddSeparatorMenuItem(menuSetting);
                        if (canAddSetting) {
                            addNewImageMenuItemWithCB(menuSetting, GTK_STOCK_ADD, "new...", MenuCB, box, EContextMenu::BoxAddSetting, unused);
                        }
                        gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemSetting), GTK_WIDGET(menuSetting));
                    }

                    // -------------- ABOUT / RENAME --------------

                    if (!m_boxCtxMenuCBs.empty()) { GtkMenuAddSeparatorMenuItem(menu); }
                    if (box->hasAttribute(OV_AttributeId_Box_ToBeUpdated)) {
                        auto updateMenuItem = addNewImageMenuItemWithCB(menu, GTK_STOCK_REFRESH, "update box", MenuCB, box,
                                                                        EContextMenu::BoxUpdate, unused);
                        if (box->hasAttribute(OV_AttributeId_Box_FlagNeedsManualUpdate)
                            || box->hasAttribute(OV_AttributeId_Box_FlagCanAddInput)
                            || box->hasAttribute(OV_AttributeId_Box_FlagCanAddOutput)
                            || box->hasAttribute(OV_AttributeId_Box_FlagCanAddSetting)
                            || box->hasAttribute(OV_AttributeId_Box_FlagCanModifyInput)
                            || box->hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput)
                            || box->hasAttribute(OV_AttributeId_Box_FlagCanModifySetting)) {
                            gtk_widget_set_sensitive(GTK_WIDGET(updateMenuItem), FALSE);
                            gtk_widget_set_tooltip_text(GTK_WIDGET(updateMenuItem), "Box must be manually updated due to its complexity.");
                        }
                    }
                    if (box->hasAttribute(OV_AttributeId_Box_PendingDeprecatedInterfacors)) {
                        addNewImageMenuItemWithCB(menu, GTK_STOCK_REFRESH, "remove deprecated I/O/S", MenuCB, box,
                                                  EContextMenu::BoxRemoveDeprecatedInterfacors, unused);
                    }
                    addNewImageMenuItemWithCB(menu, GTK_STOCK_EDIT, "rename box...", MenuCB, box, EContextMenu::BoxRename, unused);
                    if (box->getSettingCount() != 0) {
                        addNewImageMenuItemWithCB(menu, GTK_STOCK_PREFERENCES, "configure box...", MenuCB, box, EContextMenu::BoxConfigure, unused);
                    }
                    // Add this option only if the user has the authorization to open a metabox
                    if (box->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox) {
                        CIdentifier id;
                        id.fromString(box->getAttributeValue(OVP_AttributeId_Metabox_ID));

                        std::string path(m_kernelCtx.getMetaboxManager().getMetaboxFilePath(id).toASCIIString());
                        std::string ext    = boost::filesystem::extension(path);
                        bool canImportFile = false;

                        CString fileExt;
                        while ((fileExt = m_kernelCtx.getScenarioManager().getNextScenarioImporter(ScenarioImportContext_OpenScenario, fileExt))
                               != CString("")) {
                            if (ext == fileExt.toASCIIString()) {
                                canImportFile = true;
                                break;
                            }
                        }

                        if (canImportFile) {
                            addNewImageMenuItemWithCB(menu, GTK_STOCK_PREFERENCES, "open this meta box in editor", MenuCB, box,
                                                      EContextMenu::BoxEditMetabox, unused);
                        }
                    }
                    addNewImageMenuItemWithCB(menu, GTK_STOCK_CONNECT, "enable box", MenuCB, box, EContextMenu::BoxEnable, unused);
                    addNewImageMenuItemWithCB(menu, GTK_STOCK_DISCONNECT, "disable box", MenuCB, box, EContextMenu::BoxDisable, unused);
                    addNewImageMenuItemWithCB(menu, GTK_STOCK_CUT, "delete box", MenuCB, box, EContextMenu::BoxDelete, unused);
                    addNewImageMenuItemWithCB(menu, GTK_STOCK_HELP, "box documentation...", MenuCB, box, EContextMenu::BoxDocumentation, unused);
                    addNewImageMenuItemWithCB(menu, GTK_STOCK_ABOUT, "about box...", MenuCB, box, EContextMenu::BoxAbout, unused);
                }
            }

            GtkMenuAddSeparatorMenuItem(menu);
            addNewImageMenuItemWithCB(menu, GTK_STOCK_EDIT, "add comment to scenario...", MenuCB, nullptr, EContextMenu::ScenarioAddComment, unused);
            addNewImageMenuItemWithCB(menu, GTK_STOCK_ABOUT, "about scenario...", MenuCB, nullptr, EContextMenu::ScenarioAbout, unused);

            // -------------- RUN --------------

            gtk_widget_show_all(GTK_WIDGET(menu));
            gtk_menu_popup(menu, nullptr, nullptr, nullptr, nullptr, 3, event->time);
            if (m_boxCtxMenuCBs.empty()) { gtk_menu_popdown(menu); }
        }
    }

    this->Redraw();
}

void CInterfacedScenario::ScenarioDrawingAreaButtonReleasedCB(GtkWidget* /*widget*/, GdkEventButton* event)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "scenarioDrawingAreaButtonReleasedCB\n";

    if (this->IsLocked()) { return; }

    m_buttonPressed &= !((event->type == GDK_BUTTON_RELEASE) && (event->button == 1));
    m_releaseMouseX = event->x;
    m_releaseMouseY = event->y;

    if (m_currentMode != Mode_None) {
        const int startX = int(std::min(m_pressMouseX, m_currentMouseX));
        const int startY = int(std::min(m_pressMouseY, m_currentMouseY));
        const int sizeX  = int(std::max(m_pressMouseX - m_currentMouseX, m_currentMouseX - m_pressMouseX));
        const int sizeY  = int(std::max(m_pressMouseY - m_currentMouseY, m_currentMouseY - m_pressMouseY));

        if (m_currentMode == Mode_Selection || m_currentMode == Mode_SelectionAdd) {
            if (m_currentMode == Mode_Selection) { m_SelectedObjects.clear(); }
            PickInterfacedObject(startX, startY, sizeX, sizeY);
        }
        if (m_currentMode == Mode_Connect) {
            bool isActuallyConnecting             = false;
            const size_t interfacedObjectId       = PickInterfacedObject(int(m_releaseMouseX), int(m_releaseMouseY));
            const CInterfacedObject currentObject = m_interfacedObjects[interfacedObjectId];
            CInterfacedObject srcObject;
            CInterfacedObject dstObject;
            if (currentObject.m_ConnectorType == Box_Output && m_currentObject.m_ConnectorType == Box_Input) {
                srcObject            = currentObject;
                dstObject            = m_currentObject;
                isActuallyConnecting = true;
            }
            if (currentObject.m_ConnectorType == Box_Input && m_currentObject.m_ConnectorType == Box_Output) {
                srcObject            = m_currentObject;
                dstObject            = currentObject;
                isActuallyConnecting = true;
            }
            //
            if (isActuallyConnecting) {
                CIdentifier srcTypeID;
                CIdentifier dstTypeID;
                const Kernel::IBox* srcBox = m_Scenario.getBoxDetails(srcObject.m_ID);
                const Kernel::IBox* dstBox = m_Scenario.getBoxDetails(dstObject.m_ID);
                if (srcBox && dstBox) {
                    srcBox->getOutputType(srcObject.m_ConnectorIdx, srcTypeID);
                    dstBox->getInputType(dstObject.m_ConnectorIdx, dstTypeID);

                    bool hasDeprecatedInput = false;
                    srcBox->getInterfacorDeprecatedStatus(Kernel::Output, srcObject.m_ConnectorIdx, hasDeprecatedInput);
                    bool hasDeprecatedOutput = false;
                    dstBox->getInterfacorDeprecatedStatus(Kernel::Input, dstObject.m_ConnectorIdx, hasDeprecatedOutput);

                    if ((m_kernelCtx.getTypeManager().isDerivedFromStream(srcTypeID, dstTypeID)
                         || m_kernelCtx.getConfigurationManager().expandAsBoolean("${Designer_AllowUpCastConnection}", false))) {
                        if (!hasDeprecatedInput && !hasDeprecatedOutput) {
                            CIdentifier id;
                            m_Scenario.connect(id, srcObject.m_ID, srcObject.m_ConnectorIdx, dstObject.m_ID, dstObject.m_ConnectorIdx,
                                               CIdentifier::undefined());
                            this->SnapshotCB();
                        }
                        else { m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Cannot connect to/from deprecated I/O\n"; }
                    }
                    else { m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Invalid connection\n"; }
                }
            }
        }
        if (m_currentMode == Mode_MoveSelection) {
            if (sizeX == 0 && sizeY == 0) {
                if (m_controlPressed) {
                    if (m_SelectedObjects.count(m_currentObject.m_ID)) { m_SelectedObjects.erase(m_currentObject.m_ID); }
                    else { m_SelectedObjects.insert(m_currentObject.m_ID); }
                }
                else {
                    m_SelectedObjects.clear();
                    m_SelectedObjects.insert(m_currentObject.m_ID);
                }
            }
            else {
                for (const auto& id : m_SelectedObjects) {
                    if (m_Scenario.isBox(id)) {
                        CBoxProxy proxy(m_kernelCtx, m_Scenario, id);
                        proxy.SetCenter(int((proxy.GetXCenter() + 8) & 0xfffffff0), int((proxy.GetYCenter() + 8) & 0xfffffff0));
                    }
                    if (m_Scenario.isComment(id)) {
                        CCommentProxy proxy(m_kernelCtx, m_Scenario, id);
                        proxy.SetCenter(int((proxy.GetXCenter() + 8) & 0xfffffff0), int((proxy.GetYCenter() + 8) & 0xfffffff0));
                    }
                }
                this->SnapshotCB();
            }
        }
        this->Redraw();
    }

    m_currentMode = Mode_None;
}

void CInterfacedScenario::ScenarioDrawingAreaKeyPressEventCB(GtkWidget* /*widget*/, GdkEventKey* event)
{
    m_shiftPressed |= (event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R);
    m_controlPressed |= (event->keyval == GDK_Control_L || event->keyval == GDK_Control_R);
    m_altPressed |= (event->keyval == GDK_Alt_L || event->keyval == GDK_Alt_R);
    m_aPressed |= (event->keyval == GDK_a || event->keyval == GDK_A);
    m_wPressed |= (event->keyval == GDK_w || event->keyval == GDK_W);

    // m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Key pressed " << (size_t) event->keyval << "\n";
    /*
        if((event->keyval==GDK_Z || event->keyval==GDK_z) && m_controlPressed) { this->undoCB(); }

        if((event->keyval==GDK_Y || event->keyval==GDK_y) && m_controlPressed) { this->redoCB(); }
    */
    // CTRL+A = select all
    if (m_aPressed && m_controlPressed && !m_shiftPressed && !m_altPressed) {
        CIdentifier id;
        while ((id = m_Scenario.getNextBoxIdentifier(id)) != CIdentifier::undefined()) { m_SelectedObjects.insert(id); }
        while ((id = m_Scenario.getNextLinkIdentifier(id)) != CIdentifier::undefined()) { m_SelectedObjects.insert(id); }
        while ((id = m_Scenario.getNextCommentIdentifier(id)) != CIdentifier::undefined()) { m_SelectedObjects.insert(id); }
        this->Redraw();
    }

    //CTRL+W : close current scenario
    if (m_wPressed && m_controlPressed && !m_shiftPressed && !m_altPressed) {
        m_Application.CloseScenarioCB(this);
        return;
    }

    if ((event->keyval == GDK_C || event->keyval == GDK_c) && m_currentMode == Mode_None) {
        gint x = 0;
        gint y = 0;
        gdk_window_get_pointer(GTK_WIDGET(m_scenarioDrawingArea)->window, &x, &y, nullptr);

        this->AddCommentCB(x, y);
    }

    if (event->keyval == GDK_F12 && m_shiftPressed) {
        CIdentifier id;
        while ((id = m_Scenario.getNextBoxIdentifier(id)) != CIdentifier::undefined()) {
            Kernel::IBox* box       = m_Scenario.getBoxDetails(id);
            CIdentifier algorithmID = box->getAlgorithmClassIdentifier();
            CIdentifier hashValue   = m_kernelCtx.getPluginManager().getPluginObjectHashValue(algorithmID);
            if (box->hasAttribute(OV_AttributeId_Box_InitialPrototypeHashValue)) {
                box->setAttributeValue(OV_AttributeId_Box_InitialPrototypeHashValue, hashValue.toString());
            }
            else { box->addAttribute(OV_AttributeId_Box_InitialPrototypeHashValue, hashValue.toString()); }
        }

        this->Redraw();
        this->SnapshotCB();
    }

    // F1 : browse documentation
    if (event->keyval == GDK_F1) {
        bool hasDoc = false;
        for (const auto& objectId : m_SelectedObjects) {
            if (m_Scenario.isBox(objectId)) {
                BrowseBoxDocumentation(objectId);
                hasDoc = true;
            }
        }

        if (!hasDoc) {
            const CString fullUrl = m_Scenario.getAttributeValue(OV_AttributeId_Scenario_DocumentationPage);
            if (fullUrl != CString("")) {
                BrowseUrl(fullUrl, m_kernelCtx.getConfigurationManager().expand("${Designer_WebBrowserCommand}"),
                          m_kernelCtx.getConfigurationManager().expand("${Designer_WebBrowserCommandPostfix}"));
            }
            else { m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "The scenario does not define a documentation page.\n"; }
        }
    }

    // F2 : rename all selected box(es)
    if (event->keyval == GDK_F2) { ContextMenuBoxRenameAllCB(); }

    // F8 : toggle enable/disable on all selected box(es)
    if (event->keyval == GDK_F3) {
        ContextMenuBoxToggleEnableAllCB();
        this->Redraw();
    }

    //The shortcuts respect the order in the toolbar

    // F7 :play/pause
    if (event->keyval == GDK_F7) {
        if (m_Application.GetCurrentInterfacedScenario()->m_PlayerStatus == Kernel::EPlayerStatus::Play) { m_Application.PauseScenarioCB(); }
        else { m_Application.PlayScenarioCB(); }
    }
    // F6 : step
    if (event->keyval == GDK_F6) { m_Application.NextScenarioCB(); }
    // F8 :fastforward
    if (event->keyval == GDK_F8) { m_Application.ForwardScenarioCB(); }
    // F5 : stop
    if (event->keyval == GDK_F5) { m_Application.StopScenarioCB(); }

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "scenarioDrawingAreaKeyPressEventCB (" << (m_shiftPressed ? "true" : "false") << "|"
            << (m_controlPressed ? "true" : "false") << "|" << (m_altPressed ? "true" : "false") << "|" << (m_aPressed ? "true" : "false") << "|"
            << (m_wPressed ? "true" : "false") << "|" << ")\n";

    if (this->IsLocked()) { return; }

#if defined TARGET_OS_Windows || defined TARGET_OS_Linux
    if (event->keyval == GDK_Delete || event->keyval == GDK_KP_Delete)
#elif defined TARGET_OS_MacOS
    if (event->keyval == GDK_BackSpace)
#endif
    {
        this->DeleteSelection();
    }
}

void CInterfacedScenario::ScenarioDrawingAreaKeyReleaseEventCB(GtkWidget* /*widget*/, GdkEventKey* event)
{
    m_shiftPressed &= !(event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R);
    m_controlPressed &= !(event->keyval == GDK_Control_L || event->keyval == GDK_Control_R);
    m_altPressed &= !(event->keyval == GDK_Alt_L || event->keyval == GDK_Alt_R);
    m_aPressed &= !(event->keyval == GDK_A || event->keyval == GDK_a);
    m_wPressed &= !(event->keyval == GDK_W || event->keyval == GDK_w);

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug
            << "scenarioDrawingAreaKeyReleaseEventCB ("
            << (m_shiftPressed ? "true" : "false") << "|"
            << (m_controlPressed ? "true" : "false") << "|"
            << (m_altPressed ? "true" : "false") << "|"
            << (m_aPressed ? "true" : "false") << "|"
            << (m_wPressed ? "true" : "false") << "|"
            << ")\n";

    //if (this->isLocked()) { return; }
    // ...
}

void CInterfacedScenario::CopySelection() const
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "copySelection\n";

    // Prepares copy
    std::map<CIdentifier, CIdentifier> mapping;
    m_Application.m_ClipboardScenario->clear();

    // Copies boxes to clipboard
    for (auto& objectId : m_SelectedObjects) {
        if (m_Scenario.isBox(objectId)) {
            CIdentifier id;
            const Kernel::IBox* box = m_Scenario.getBoxDetails(objectId);
            m_Application.m_ClipboardScenario->addBox(id, *box, objectId);
            mapping[objectId] = id;
        }
    }

    // Copies comments to clipboard
    for (auto& objectId : m_SelectedObjects) {
        if (m_Scenario.isComment(objectId)) {
            CIdentifier id;
            const Kernel::IComment* comment = m_Scenario.getCommentDetails(objectId);
            m_Application.m_ClipboardScenario->addComment(id, *comment, objectId);
            mapping[objectId] = id;
        }
    }

    // Copies links to clipboard
    for (auto& objectId : m_SelectedObjects) {
        if (m_Scenario.isLink(objectId)) {
            const Kernel::ILink* link = m_Scenario.getLinkDetails(objectId);

            // Connect link only if the source and target boxes are copied
            if (mapping.find(link->getSourceBoxIdentifier()) != mapping.end() && mapping.find(link->getTargetBoxIdentifier()) != mapping.end()) {
                CIdentifier id;
                m_Application.m_ClipboardScenario->connect(id, mapping[link->getSourceBoxIdentifier()], link->getSourceBoxOutputIndex(),
                                                           mapping[link->getTargetBoxIdentifier()], link->getTargetBoxInputIndex(), link->getIdentifier());
            }
        }
    }
}

void CInterfacedScenario::CutSelection()
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "cutSelection\n";

    this->CopySelection();
    this->DeleteSelection();
}

void CInterfacedScenario::PasteSelection()
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "pasteSelection\n";

    // Prepares paste
    CIdentifier id;
    std::map<CIdentifier, CIdentifier> mapping;
    // int centerX = 0, centerY = 0;
    int mostTLCopiedBoxCenterX = 1 << 15;   // most top most left 
    int mostTLCopiedBoxCenterY = 1 << 15;   // most top most left 
    // std::cout << "Mouse position : " << m_currentMouseX << "/" << m_currentMouseY << std::endl;

    // Pastes boxes from clipboard
    while ((id = m_Application.m_ClipboardScenario->getNextBoxIdentifier(id)) != CIdentifier::undefined()) {
        CIdentifier newID;
        Kernel::IBox* box = m_Application.m_ClipboardScenario->getBoxDetails(id);
        m_Scenario.addBox(newID, *box, id);
        mapping[id] = newID;

        // Updates visualization manager
        CIdentifier boxAlgorithmID            = box->getAlgorithmClassIdentifier();
        const Plugins::IPluginObjectDesc* pod = m_kernelCtx.getPluginManager().getPluginObjectDescCreating(boxAlgorithmID);

        // If a visualization box was dropped, add it in window manager
        if (pod && pod->hasFunctionality(Plugins::EPluginFunctionality::Visualization)) {
            // Let window manager know about new box
            if (m_DesignerVisualization) { m_DesignerVisualization->OnVisualizationBoxAdded(m_Scenario.getBoxDetails(newID)); }
        }

        CBoxProxy proxy(m_kernelCtx, m_Scenario, newID);

        // get the position of the topmost-leftmost box (always position on an actual box so when user pastes he sees something)
        if (proxy.GetXCenter() < mostTLCopiedBoxCenterX && proxy.GetXCenter() < mostTLCopiedBoxCenterY) {
            mostTLCopiedBoxCenterX = proxy.GetXCenter();
            mostTLCopiedBoxCenterY = proxy.GetYCenter();
        }
    }

    // Pastes comments from clipboard
    while ((id = m_Application.m_ClipboardScenario->getNextCommentIdentifier(id)) != CIdentifier::undefined()) {
        CIdentifier newID;
        Kernel::IComment* comment = m_Application.m_ClipboardScenario->getCommentDetails(id);
        m_Scenario.addComment(newID, *comment, id);
        mapping[id] = newID;

        CCommentProxy proxy(m_kernelCtx, m_Scenario, newID);

        if (proxy.GetXCenter() < mostTLCopiedBoxCenterX && proxy.GetYCenter() < mostTLCopiedBoxCenterY) {
            mostTLCopiedBoxCenterX = proxy.GetXCenter();
            mostTLCopiedBoxCenterY = proxy.GetYCenter();
        }
    }

    // Pastes links from clipboard
    while ((id = m_Application.m_ClipboardScenario->getNextLinkIdentifier(id)) != CIdentifier::undefined()) {
        CIdentifier newID;
        Kernel::ILink* link = m_Application.m_ClipboardScenario->getLinkDetails(id);
        m_Scenario.connect(newID, mapping[link->getSourceBoxIdentifier()], link->getSourceBoxOutputIndex(), mapping[link->getTargetBoxIdentifier()],
                           link->getTargetBoxInputIndex(), link->getIdentifier());
    }

    // Makes pasted stuff the default selection
    // Moves boxes under cursor
    // Moves comments under cursor
    if (m_Application.m_ClipboardScenario->getNextBoxIdentifier(CIdentifier::undefined()) != CIdentifier::undefined()
        || m_Application.m_ClipboardScenario->getNextCommentIdentifier(CIdentifier::undefined()) != CIdentifier::undefined()) {
        m_SelectedObjects.clear();
        for (auto& it : mapping) {
            m_SelectedObjects.insert(it.second);

            if (m_Scenario.isBox(it.second)) {
                // Moves boxes under cursor
                CBoxProxy proxy(m_kernelCtx, m_Scenario, it.second);
                proxy.SetCenter(int(proxy.GetXCenter() + m_currentMouseX) - mostTLCopiedBoxCenterX - m_viewOffsetX,
                                int(proxy.GetYCenter() + m_currentMouseY) - mostTLCopiedBoxCenterY - m_viewOffsetY);
                // Ok, why 32 would you ask, just because it is fine

                // Aligns boxes on grid
                proxy.SetCenter(int((proxy.GetXCenter() + 8) & 0xfffffff0L), int((proxy.GetYCenter() + 8) & 0xfffffff0L));
            }

            if (m_Scenario.isComment(it.second)) {
                // Moves commentes under cursor
                CCommentProxy proxy(m_kernelCtx, m_Scenario, it.second);
                proxy.SetCenter(int(proxy.GetXCenter() + m_currentMouseX) - mostTLCopiedBoxCenterX - m_viewOffsetX,
                                int(proxy.GetYCenter() + m_currentMouseY) - mostTLCopiedBoxCenterY - m_viewOffsetY);
                // Ok, why 32 would you ask, just because it is fine

                // Aligns commentes on grid
                proxy.SetCenter(int((proxy.GetXCenter() + 8) & 0xfffffff0L), int((proxy.GetYCenter() + 8) & 0xfffffff0L));
            }
        }
    }

    this->Redraw();
    this->SnapshotCB();
}

void CInterfacedScenario::DeleteSelection()
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "deleteSelection\n";
    for (auto& id : m_SelectedObjects) {
        if (m_Scenario.isBox(id)) { this->DeleteBox(id); }
        if (m_Scenario.isComment(id)) {
            // removes comment from scenario
            m_Scenario.removeComment(id);
        }
        if (m_Scenario.isLink(id)) {
            // removes link from scenario
            m_Scenario.disconnect(id);
        }
    }
    m_SelectedObjects.clear();

    this->Redraw();
    this->SnapshotCB();
}

void CInterfacedScenario::DeleteBox(const CIdentifier& boxID) const
{
    // removes visualization box from window manager
    if (m_DesignerVisualization) { m_DesignerVisualization->OnVisualizationBoxRemoved(boxID); }

    // removes box from scenario
    m_Scenario.removeBox(boxID);
}


void CInterfacedScenario::ContextMenuBoxUpdateCB(const Kernel::IBox& box)
{
    m_Scenario.updateBox(box.getIdentifier());
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxUpdateCB\n";
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxRemoveDeprecatedInterfacorsCB(const Kernel::IBox& box)
{
    m_Scenario.removeDeprecatedInterfacorsFromBox(box.getIdentifier());
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxRemoveDeprecatedInterfacorsCB\n";
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxRenameCB(Kernel::IBox& box)
{
    const Plugins::IPluginObjectDesc* pod = m_kernelCtx.getPluginManager().getPluginObjectDescCreating(box.getAlgorithmClassIdentifier());
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxRenameCB\n";

    if (box.getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox) {
        CIdentifier id;
        id.fromString(box.getAttributeValue(OVP_AttributeId_Metabox_ID));
        pod = m_kernelCtx.getMetaboxManager().getMetaboxObjectDesc(id);
    }

    CRenameDialog rename(m_kernelCtx, box.getName(), pod ? pod->getName() : box.getName(), m_guiFilename.c_str());
    if (rename.Run()) {
        box.setName(rename.GetResult());

        //check whether it is a visualization box
        const CIdentifier id                   = box.getAlgorithmClassIdentifier();
        const Plugins::IPluginObjectDesc* desc = m_kernelCtx.getPluginManager().getPluginObjectDescCreating(id);

        //if a visualization box was renamed, tell window manager about it
        if (desc && desc->hasFunctionality(Plugins::EPluginFunctionality::Visualization)) {
            if (m_DesignerVisualization) { m_DesignerVisualization->OnVisualizationBoxRenamed(box.getIdentifier()); }
        }
        this->SnapshotCB();
    }
}

void CInterfacedScenario::ContextMenuBoxRenameAllCB()
{
    //we find all selected boxes
    std::map<CIdentifier, CIdentifier> selectedBoxes; // map(object,class)
    for (auto& id : m_SelectedObjects) { if (m_Scenario.isBox(id)) { selectedBoxes[id] = m_Scenario.getBoxDetails(id)->getAlgorithmClassIdentifier(); } }

    if (!selectedBoxes.empty()) {
        bool dialogOk   = true;
        bool firstBox   = true;
        CString newName = "";
        for (auto it = selectedBoxes.begin(); it != selectedBoxes.end() && dialogOk; ++it) {
            if (it->second != CIdentifier::undefined()) {
                if (m_kernelCtx.getPluginManager().canCreatePluginObject(it->second) || it->second == OVP_ClassId_BoxAlgorithm_Metabox) {
                    Kernel::IBox* box = m_Scenario.getBoxDetails(it->first);
                    if (firstBox) {
                        firstBox                              = false;
                        const Plugins::IPluginObjectDesc* pod = m_kernelCtx.getPluginManager().getPluginObjectDescCreating(box->getAlgorithmClassIdentifier());
                        if (box->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox) {
                            CIdentifier metaboxId;
                            metaboxId.fromString(box->getAttributeValue(OVP_AttributeId_Metabox_ID));
                            pod = m_kernelCtx.getMetaboxManager().getMetaboxObjectDesc(metaboxId);
                        }

                        CRenameDialog rename(m_kernelCtx, box->getName(), pod ? pod->getName() : box->getName(), m_guiFilename.c_str());
                        if (rename.Run()) { newName = rename.GetResult(); }
                        else {
                            // no rename at all.
                            dialogOk = false;
                        }
                    }
                    if (dialogOk) {
                        box->setName(newName);

                        //check whether it is a visualization box
                        CIdentifier id                        = box->getAlgorithmClassIdentifier();
                        const Plugins::IPluginObjectDesc* pod = m_kernelCtx.getPluginManager().getPluginObjectDescCreating(id);

                        //if a visualization box was renamed, tell window manager about it
                        if (pod && pod->hasFunctionality(Plugins::EPluginFunctionality::Visualization)) {
                            if (m_DesignerVisualization) { m_DesignerVisualization->OnVisualizationBoxRenamed(box->getIdentifier()); }
                        }
                    }
                }
            }
        }
        if (dialogOk) { this->SnapshotCB(); }
    }
}

void CInterfacedScenario::ContextMenuBoxToggleEnableAllCB()
{
    //we find all selected boxes
    for (const auto& objectId : m_SelectedObjects) {
        if (m_Scenario.isBox(objectId)) {
            TAttributeHandler handler(*m_Scenario.getBoxDetails(objectId));
            if (handler.HasAttribute(OV_AttributeId_Box_Disabled)) { handler.RemoveAttribute(OV_AttributeId_Box_Disabled); }
            else { handler.addAttribute(OV_AttributeId_Box_Disabled, 1); }
        }
    }
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxEnableAllCB()
{
    //we find all selected boxes
    for (const auto& objectId : m_SelectedObjects) {
        if (m_Scenario.isBox(objectId)) {
            TAttributeHandler handler(*m_Scenario.getBoxDetails(objectId));
            if (handler.HasAttribute(OV_AttributeId_Box_Disabled)) { handler.RemoveAttribute(OV_AttributeId_Box_Disabled); }
        }
    }
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxDisableAllCB()
{
    //we find all selected boxes
    for (const auto& objectId : m_SelectedObjects) {
        if (m_Scenario.isBox(objectId)) {
            TAttributeHandler handler(*m_Scenario.getBoxDetails(objectId));
            if (!handler.HasAttribute(OV_AttributeId_Box_Disabled)) { handler.addAttribute(OV_AttributeId_Box_Disabled, 1); }
        }
    }
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxAddInputCB(Kernel::IBox& box)
{
    if (box.hasAttribute(OV_AttributeId_Box_PendingDeprecatedInterfacors)) {
        gtk_dialog_run(GTK_DIALOG(m_errorPendingDeprecatedInterfacorsDialog));
        return;
    }
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxAddInputCB\n";
    box.addInput("New input", OV_TypeId_EBMLStream, m_Scenario.getUnusedInputIdentifier());
    if (box.hasAttribute(OV_AttributeId_Box_FlagCanModifyInput)) {
        CConnectorEditor editor(m_kernelCtx, box, Box_Input, box.getInputCount() - 1, "Add Input", m_guiFilename.c_str());
        if (editor.Run()) { this->SnapshotCB(); }
        else { box.removeInput(box.getInputCount() - 1); }
    }
    else { this->SnapshotCB(); }
}

void CInterfacedScenario::ContextMenuBoxEditInputCB(Kernel::IBox& box, const size_t index)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxEditInputCB\n";

    CConnectorEditor editor(m_kernelCtx, box, Box_Input, index, "Edit Input", m_guiFilename.c_str());
    if (editor.Run()) { this->SnapshotCB(); }
}

void CInterfacedScenario::ContextMenuBoxRemoveInputCB(Kernel::IBox& box, const size_t index)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxRemoveInputCB\n";
    box.removeInput(index);
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxAddOutputCB(Kernel::IBox& box)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxAddOutputCB\n";
    box.addOutput("New output", OV_TypeId_EBMLStream, m_Scenario.getUnusedOutputIdentifier());
    if (box.hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput)) {
        CConnectorEditor editor(m_kernelCtx, box, Box_Output, box.getOutputCount() - 1, "Add Output", m_guiFilename.c_str());
        if (editor.Run()) { this->SnapshotCB(); }
        else { box.removeOutput(box.getOutputCount() - 1); }
    }
    else { this->SnapshotCB(); }
}

void CInterfacedScenario::ContextMenuBoxEditOutputCB(Kernel::IBox& box, const size_t index)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxEditOutputCB\n";

    CConnectorEditor editor(m_kernelCtx, box, Box_Output, index, "Edit Output", m_guiFilename.c_str());
    if (editor.Run()) { this->SnapshotCB(); }
}

void CInterfacedScenario::ContextMenuBoxRemoveOutputCB(Kernel::IBox& box, const size_t index)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxRemoveOutputCB\n";
    box.removeOutput(index);
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxConnectScenarioInputCB(Kernel::IBox& box, const size_t boxInputIdx, const size_t scenarioInputIdx)
{
    //  m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "contextMenuBoxConnectScenarioInputCB : box = " << box.getIdentifier().str() << " box input = " << boxInputIdx << " , scenario input = " << scenarioInputIdx << "\n";
    m_Scenario.setScenarioInputLink(scenarioInputIdx, box.getIdentifier(), boxInputIdx);
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxConnectScenarioOutputCB(Kernel::IBox& box, const size_t boxOutputIdx, const size_t scenarioOutputIdx)
{
    //  m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "contextMenuBoxConnectScenarioOutputCB : box = " << box.getIdentifier().str() << " box Output = " << boxOutputIdx << " , scenario Output = " << scenarioOutputIdx << "\n";
    m_Scenario.setScenarioOutputLink(scenarioOutputIdx, box.getIdentifier(), boxOutputIdx);
    this->SnapshotCB();
}

// Note: In current implementation only the scenarioInputIdx is necessary as it can only be connected to one input
// but to keep things simpler we give it all the info
void CInterfacedScenario::ContextMenuBoxDisconnectScenarioInputCB(Kernel::IBox& box, const size_t boxInputIdx, const size_t scenarioInputIdx)
{
    m_Scenario.removeScenarioInputLink(scenarioInputIdx, box.getIdentifier(), boxInputIdx);
    this->SnapshotCB();
}

// Note: In current implementation only the scenarioOutputIdx is necessary as it can only be connected to one output
// but to keep things simpler we give it all the info
void CInterfacedScenario::ContextMenuBoxDisconnectScenarioOutputCB(Kernel::IBox& box, const size_t boxOutputIdx, const size_t scenarioOutputIdx)
{
    m_Scenario.removeScenarioOutputLink(scenarioOutputIdx, box.getIdentifier(), boxOutputIdx);
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxAddSettingCB(Kernel::IBox& box)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxAddSettingCB\n";
    // Store setting count in case the custom "onSettingAdded" of the box adds more than one setting
    const size_t nOldSettings = box.getSettingCount();
    box.addSetting("New setting", CIdentifier::undefined(), "", size_t(-1), false, m_Scenario.getUnusedSettingIdentifier(CIdentifier::undefined()));
    const size_t nNewSettings = box.getSettingCount();
    // Check that at least one setting was added
    if (nNewSettings > nOldSettings && box.hasAttribute(OV_AttributeId_Box_FlagCanModifySetting)) {
        CSettingEditorDialog dialog(m_kernelCtx, box, nOldSettings, "Add Setting", m_guiFilename.c_str(), m_guiSettingsFilename.c_str());
        if (dialog.Run()) { this->SnapshotCB(); }
        else { for (size_t i = nOldSettings; i < nNewSettings; ++i) { box.removeSetting(i); } }
    }
    else {
        if (nNewSettings > nOldSettings) { this->SnapshotCB(); }
        else {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "No setting could be added to the box.\n";
            return;
        }
    }
    // Add an information message to inform the user about the new settings
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "[" << nNewSettings - nOldSettings
            << "] new setting(s) was(were) added to the box [" << box.getName().toASCIIString() << "]: ";
    for (size_t i = nOldSettings; i < nNewSettings; ++i) {
        CString name;
        box.getSettingName(i, name);
        m_kernelCtx.getLogManager() << "[" << name << "] ";
    }
    m_kernelCtx.getLogManager() << "\n";
    // After adding setting, open configuration so that the user can see the effects.
    CBoxConfigurationDialog dialog(m_kernelCtx, box, m_guiFilename.c_str(), m_guiSettingsFilename.c_str());
    dialog.Run();
}

void CInterfacedScenario::ContextMenuBoxEditSettingCB(Kernel::IBox& box, const size_t index)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxEditSettingCB\n";
    CSettingEditorDialog dialog(m_kernelCtx, box, index, "Edit Setting", m_guiFilename.c_str(), m_guiSettingsFilename.c_str());
    if (dialog.Run()) { this->SnapshotCB(); }
}

void CInterfacedScenario::ContextMenuBoxRemoveSettingCB(Kernel::IBox& box, const size_t index)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxRemoveSettingCB\n";
    const size_t nOldSettings = box.getSettingCount();
    if (box.removeSetting(index)) {
        const size_t nNewSettings = box.getSettingCount();
        this->SnapshotCB();
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "[" << nOldSettings - nNewSettings
                << "] setting(s) was(were) removed from box [" << box.getName() << "] \n";
    }
    else {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "The setting with index [" << index
                << "] could not be removed from box [" << box.getName() << "] \n";
    }
}

void CInterfacedScenario::ContextMenuBoxConfigureCB(Kernel::IBox& box)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxConfigureCB\n";
    CBoxConfigurationDialog dialog(m_kernelCtx, box, m_guiFilename.c_str(), m_guiSettingsFilename.c_str());
    dialog.Run();
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxAboutCB(Kernel::IBox& box) const
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxAboutCB\n";
    if (box.getAlgorithmClassIdentifier() != OVP_ClassId_BoxAlgorithm_Metabox) {
        CAboutPluginDialog dialog(m_kernelCtx, box.getAlgorithmClassIdentifier(), m_guiFilename.c_str());
        dialog.Run();
    }
    else {
        CIdentifier id;
        id.fromString(box.getAttributeValue(OVP_AttributeId_Metabox_ID));
        CAboutPluginDialog dialog(m_kernelCtx, m_kernelCtx.getMetaboxManager().getMetaboxObjectDesc(id), m_guiFilename.c_str());
        dialog.Run();
    }
}

void CInterfacedScenario::ContextMenuBoxEditMetaboxCB(Kernel::IBox& box) const
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxEditMetaboxCB\n";

    CIdentifier id;
    id.fromString(box.getAttributeValue(OVP_AttributeId_Metabox_ID));
    const CString path(m_kernelCtx.getMetaboxManager().getMetaboxFilePath(id));

    m_Application.OpenScenario(path.toASCIIString());
}

bool CInterfacedScenario::BrowseUrl(const CString& url, const CString& browserPrefix, const CString& browserPostfix) const
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Requesting web browser on URL " << url << "\n";

    const CString cmd = browserPrefix + CString(" \"") + url + CString("\"") + browserPostfix;
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Launching [" << cmd << "]\n";
    const int result = system(cmd.toASCIIString());
    if (result < 0) {
        OV_WARNING("Could not launch command [" << cmd << "]\n", m_kernelCtx.getLogManager());
        return false;
    }
    return true;
}

bool CInterfacedScenario::BrowseBoxDocumentation(const CIdentifier& boxID) const
{
    const CIdentifier algorithmClassID = m_Scenario.getBoxDetails(boxID)->getAlgorithmClassIdentifier();

    // Do not show documentation for non-metaboxes or boxes that can not be created
    if (!(boxID != CIdentifier::undefined() && (m_kernelCtx.getPluginManager().canCreatePluginObject(algorithmClassID) ||
                                                m_Scenario.getBoxDetails(boxID)->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox))) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Box with id " << boxID << " can not create a pluging object\n";
        return false;
    }

    const CString defaultURLBase = m_kernelCtx.getConfigurationManager().expand("${Designer_HelpBrowserURLBase}");
    CString urlBase              = defaultURLBase;
    CString browser              = m_kernelCtx.getConfigurationManager().expand("${Designer_HelpBrowserCommand}");
    CString browserPostfix       = m_kernelCtx.getConfigurationManager().expand("${Designer_HelpBrowserCommandPostfix}");
    CString boxName;

    CString html = "Doc_BoxAlgorithm_";
    if (m_Scenario.getBoxDetails(boxID)->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox) {
        CIdentifier id;
        id.fromString(m_Scenario.getBoxDetails(boxID)->getAttributeValue(OVP_AttributeId_Metabox_ID));
        boxName = m_kernelCtx.getMetaboxManager().getMetaboxObjectDesc(id)->getName();
    }
    else {
        const Plugins::IPluginObjectDesc* pod = m_kernelCtx.getPluginManager().getPluginObjectDescCreating(algorithmClassID);
        boxName                               = pod->getName();
    }
    // The documentation files do not have spaces in their name, so we remove them
    html = html + CString(getBoxAlgorithmURL(boxName.toASCIIString()).c_str());


    if (m_Scenario.getBoxDetails(boxID)->hasAttribute(OV_AttributeId_Box_DocumentationURLBase)) {
        urlBase = m_kernelCtx.getConfigurationManager().expand(m_Scenario.getBoxDetails(boxID)->getAttributeValue(OV_AttributeId_Box_DocumentationURLBase));
    }
    html = html + ".html";

    if (m_Scenario.getBoxDetails(boxID)->hasAttribute(OV_AttributeId_Box_DocumentationCommand)) {
        browser = m_kernelCtx.getConfigurationManager().expand(m_Scenario.getBoxDetails(boxID)->getAttributeValue(OV_AttributeId_Box_DocumentationCommand));
        browserPostfix = "";
    }

    CString fullUrl = urlBase + CString("/") + html;
    if (m_Scenario.getBoxDetails(boxID)->hasAttribute(OV_AttributeId_Box_DocumentationURL)) {
        fullUrl = m_kernelCtx.getConfigurationManager().expand(m_Scenario.getBoxDetails(boxID)->getAttributeValue(OV_AttributeId_Box_DocumentationURL));
    }

    return BrowseUrl(fullUrl, browser, browserPostfix);
}

void CInterfacedScenario::ContextMenuBoxDocumentationCB(Kernel::IBox& box) const
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxDocumentationCB\n";
    const CIdentifier id = box.getIdentifier();
    BrowseBoxDocumentation(id);
}

void CInterfacedScenario::ContextMenuBoxEnableCB(Kernel::IBox& box)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxEnableCB\n";
    const TAttributeHandler handler(box);
    handler.RemoveAttribute(OV_AttributeId_Box_Disabled);
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuBoxDisableCB(Kernel::IBox& box)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuBoxDisableCB\n";
    TAttributeHandler handler(box);
    if (!handler.HasAttribute(OV_AttributeId_Box_Disabled)) { handler.addAttribute(OV_AttributeId_Box_Disabled, 1); }
    else { handler.setAttributeValue(OV_AttributeId_Box_Disabled, 1); }
    this->SnapshotCB();
}

void CInterfacedScenario::ContextMenuScenarioAddCommentCB()
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuScenarioAddCommentCB\n";
    this->AddCommentCB();
}

void CInterfacedScenario::ContextMenuScenarioAboutCB()
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "contextMenuScenarioAboutCB\n";
    const CAboutScenarioDialog dialog(m_kernelCtx, m_Scenario, m_guiFilename.c_str());
    dialog.Run();
    this->SnapshotCB();
}

void CInterfacedScenario::ToggleDesignerVisualization()
{
    m_designerVisualizationToggled = !m_designerVisualizationToggled;

    if (m_DesignerVisualization) {
        if (m_designerVisualizationToggled) { m_DesignerVisualization->Show(); }
        else { m_DesignerVisualization->Hide(); }
    }
}

void CInterfacedScenario::ShowCurrentVisualization() const
{
    if (IsLocked()) { if (m_playerVisualization != nullptr) { m_playerVisualization->ShowTopLevelWindows(); } }
    else { if (m_DesignerVisualization != nullptr) { m_DesignerVisualization->Show(); } }
}

void CInterfacedScenario::HideCurrentVisualization() const
{
    if (IsLocked()) { if (m_playerVisualization != nullptr) { m_playerVisualization->HideTopLevelWindows(); } }
    else { if (m_DesignerVisualization != nullptr) { m_DesignerVisualization->Hide(); } }
}

void CInterfacedScenario::CreatePlayerVisualization(VisualizationToolkit::IVisualizationTree* tree)
{
    //hide window manager
    if (m_DesignerVisualization) { m_DesignerVisualization->Hide(); }

    if (m_playerVisualization == nullptr) {
        if (tree) { m_playerVisualization = new CPlayerVisualization(m_kernelCtx, *tree, *this); }
        else { m_playerVisualization = new CPlayerVisualization(m_kernelCtx, *m_Tree, *this); }


        //we go here when we press start
        //we have to set the modUI here
        //first, find the concerned boxes
        Kernel::IScenario& runtimeScenario = m_Player->getRuntimeScenarioManager().getScenario(m_Player->getRuntimeScenarioIdentifier());
        CIdentifier id;
        while ((id = runtimeScenario.getNextBoxIdentifier(id)) != CIdentifier::undefined()) {
            Kernel::IBox* box = runtimeScenario.getBoxDetails(id);
            if (box->hasModifiableSettings())//if the box has modUI
            {
                //create a BoxConfigurationDialog in mode true
                auto* dialog = new CBoxConfigurationDialog(m_kernelCtx, *box, m_guiFilename.c_str(), m_guiSettingsFilename.c_str(), true);
                //store it
                m_boxConfigDialogs.push_back(dialog);
            }
        }
    }

    //initialize and show windows
    m_playerVisualization->Init();
}

void CInterfacedScenario::ReleasePlayerVisualization()
{
    if (m_playerVisualization != nullptr) {
        delete m_playerVisualization;
        m_playerVisualization = nullptr;
    }

    //reload designer visualization
    if (m_DesignerVisualization) {
        m_DesignerVisualization->Load();
        //show it if it was toggled on
        if (m_designerVisualizationToggled) { m_DesignerVisualization->Show(); }
    }
}

void CInterfacedScenario::StopAndReleasePlayer()
{
    m_kernelCtx.getErrorManager().releaseErrors();
    m_Player->stop();
    m_PlayerStatus = m_Player->getStatus();
    // removes idle function
    g_idle_remove_by_data(this);

    if (!m_Player->uninitialize()) { m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to uninitialize the player" << "\n"; }

    for (const auto elem : m_boxConfigDialogs) {
        elem->RestoreState();
        delete elem;
    }
    m_boxConfigDialogs.clear();


    if (!m_kernelCtx.getPlayerManager().releasePlayer(m_PlayerID)) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to release the player" << "\n";
    }

    m_PlayerID = CIdentifier::undefined();
    m_Player   = nullptr;

    // destroy player windows
    ReleasePlayerVisualization();

    // redraws scenario
    Redraw();
}

//give the PlayerVisualisation the matching between the GtkWidget created by the CBoxConfigurationDialog and the Box CIdentifier
bool CInterfacedScenario::SetModifiableSettingsWidgets() const
{
    for (const auto& elem : m_boxConfigDialogs) { m_playerVisualization->SetWidget(elem->GetBoxID(), elem->GetWidget()); }
    return true;
}

bool CInterfacedScenario::CenterOnBox(const CIdentifier& identifier)
{
    //m_kernelCtx.getLogManager() << Kernel::LogLevel_Fatal << "CInterfacedScenario::centerOnBox" << "\n";
    bool res = false;
    if (m_Scenario.isBox(identifier)) {
        //m_kernelCtx.getLogManager() << Kernel::LogLevel_Fatal << "CInterfacedScenario::centerOnBox is box" << "\n";
        const Kernel::IBox* box = m_Scenario.getBoxDetails(identifier);

        //clear previous selection
        m_SelectedObjects.clear();
        //to select the box

        m_SelectedObjects.insert(identifier);
        //      m_bScenarioModified=true;
        Redraw();

        //CBoxProxy proxy(m_kernelCtx, *box);
        const CBoxProxy proxy(m_kernelCtx, m_Scenario, box->getIdentifier());
        const double marginX = 5.0 * m_currentScale;
        const double merginY = 5.0 * m_currentScale;
        const int sizeX      = int(round(proxy.GetWidth(GTK_WIDGET(m_scenarioDrawingArea)) + marginX * 2.0));
        const int sizeY      = int(round(proxy.GetHeight(GTK_WIDGET(m_scenarioDrawingArea)) + merginY * 2.0));
        const double centerX = proxy.GetXCenter() * m_currentScale;
        const double centerY = proxy.GetYCenter() * m_currentScale;
        int x, y;

        //get the parameters of the current adjustement
        GtkAdjustment* oldAdjustmentH = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(m_notebookPageContent));
        GtkAdjustment* oldAdjustmentV = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(m_notebookPageContent));
        gdouble upper, lower, step, page, pagesize, value;

        g_object_get(oldAdjustmentH, "upper", &upper, "lower", &lower, "step-increment", &step, "page-increment", &page, "page-size", &pagesize, "value",
                     &value, nullptr);
        //create a new adjustement with the correct value since we can not change the upper bound of the old adjustement
        auto* adjustment = reinterpret_cast<GtkAdjustment*>(gtk_adjustment_new(value, lower, upper, step, page, pagesize));
        if (centerX + m_viewOffsetX < upper / 2) { x = int(round(centerX - 2 * sizeX)) + m_viewOffsetX; }
        else { x = int(round(centerX + 2 * sizeX - pagesize)) + m_viewOffsetX; }
        gtk_adjustment_set_value(adjustment, x);
        gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(m_notebookPageContent), adjustment);

        g_object_get(oldAdjustmentV, "upper", &upper, "lower", &lower, "step-increment", &step, "page-increment", &page, "page-size", &pagesize, "value",
                     &value, nullptr);
        adjustment = reinterpret_cast<GtkAdjustment*>(gtk_adjustment_new(value, lower, upper, step, page, pagesize));
        if (centerY - m_viewOffsetY < upper / 2) { y = int(round(centerY - 2 * sizeY) + m_viewOffsetY); }
        else { y = int(round(centerY + 2 * sizeY - pagesize)) + m_viewOffsetY; }
        gtk_adjustment_set_value(adjustment, y);
        gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(m_notebookPageContent), adjustment);
        res = true;
    }
    return res;
}

void CInterfacedScenario::SetScale(const double scale)
{
    m_currentScale = std::max(scale, 0.1);

    PangoContext* ctx          = gtk_widget_get_pango_context(GTK_WIDGET(m_scenarioDrawingArea));
    PangoFontDescription* desc = pango_context_get_font_description(ctx);
    // not done in constructor because the font size is changed elsewhere after that withour our knowledge
    if (m_normalFontSize == 0) { m_normalFontSize = pango_font_description_get_size(desc); }
    pango_font_description_set_size(desc, gint(round(m_normalFontSize * m_currentScale)));

    //m_scenarioModified = true;
    Redraw();
}

}  // namespace Designer
}  // namespace OpenViBE
