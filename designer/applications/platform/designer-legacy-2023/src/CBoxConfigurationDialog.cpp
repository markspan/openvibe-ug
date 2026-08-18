///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxConfigurationDialog.cpp
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

#include "CBoxConfigurationDialog.hpp"
#include "CSettingCollectionHelper.hpp"

#include <vector>
#include <string>

#include "tinyxml2.h"

namespace OpenViBE {
namespace Designer {

static const char* const ROOT_NAME    = "OpenViBE-SettingsOverride";
static const char* const SETTING_NAME = "SettingValue";

static void onFileOverrideCheckToggled(GtkToggleButton* button, gpointer data)
{
    gtk_widget_set_sensitive(static_cast<GtkWidget*>(data), !gtk_toggle_button_get_active(button));
}

static void OnButtonLoadClicked(GtkButton* /*button*/, gpointer data) { static_cast<CBoxConfigurationDialog*>(data)->LoadConfig(); }
static void OnButtonSaveClicked(GtkButton* /*button*/, gpointer data) { static_cast<CBoxConfigurationDialog*>(data)->SaveConfig(); }
static void OnOverrideBrowseClicked(GtkButton* /*button*/, gpointer data) { static_cast<CBoxConfigurationDialog*>(data)->OnOverrideBrowse(); }
static void CollectWidgetCB(GtkWidget* widget, gpointer data) { static_cast<std::vector<GtkWidget*>*>(data)->push_back(widget); }

CBoxConfigurationDialog::CBoxConfigurationDialog(const Kernel::IKernelContext& ctx, Kernel::IBox& box, const char* guiFilename,
                                                 const char* guiSettingsFilename, const bool isScenarioRunning)
    : m_kernelCtx(ctx), m_box(box), m_guiFilename(guiFilename), m_guiSettingsFilename(guiSettingsFilename),
      m_settingFactory(m_guiSettingsFilename.toASCIIString(), ctx), m_isScenarioRunning(isScenarioRunning)
{
    m_box.addObserver(this);

    if (m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting)) {
        GtkBuilder* builder = gtk_builder_new(); // glade_xml_new(m_guiFilename.toASCIIString(), "box_configuration", nullptr);
        gtk_builder_add_from_file(builder, m_guiFilename.toASCIIString(), nullptr);
        gtk_builder_connect_signals(builder, nullptr);

        if (!m_isScenarioRunning) {
            // TODO : This is not a modal dialog. It would be better if it was.
            m_settingDialog         = GTK_WIDGET(gtk_builder_get_object(builder, "box_configuration"));
            const std::string title = std::string("Configure ") + m_box.getName().toASCIIString() + " settings";
            gtk_window_set_title(GTK_WINDOW(m_settingDialog), title.c_str());
        }
        else {
            // This is actually *not* a dialog
            m_settingDialog = GTK_WIDGET(gtk_builder_get_object(builder, "box_configuration-scrolledwindow"));
        }
        m_settingsTable  = GTK_TABLE(gtk_builder_get_object(builder, "box_configuration-table"));
        m_scrolledWindow = GTK_SCROLLED_WINDOW(gtk_builder_get_object(builder, "box_configuration-scrolledwindow"));
        m_viewPort       = GTK_VIEWPORT(gtk_builder_get_object(builder, "box_configuration-viewport"));

        gtk_table_resize(m_settingsTable, guint(m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting)), 4);

        generateSettingsTable();

        const CSettingCollectionHelper helper(m_kernelCtx, m_guiSettingsFilename.toASCIIString());

        if (!m_isScenarioRunning) {
            GtkContainer* fileOverrideContainer = GTK_CONTAINER(gtk_builder_get_object(builder, "box_configuration-hbox_filename_override"));
            m_fileOverrideCheck                 = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "box_configuration-checkbutton_filename_override"));
            GtkButton* buttonLoad               = GTK_BUTTON(gtk_builder_get_object(builder, "box_configuration-button_load_current_from_file"));
            GtkButton* buttonSave               = GTK_BUTTON(gtk_builder_get_object(builder, "box_configuration-button_save_current_to_file"));

            const std::string settingOverrideWidgetName   = helper.GetSettingWidgetName(OV_TypeId_Filename).toASCIIString();
            GtkBuilder* builderInterfaceSettingCollection = gtk_builder_new();
            gtk_builder_add_from_file(builderInterfaceSettingCollection, m_guiSettingsFilename.toASCIIString(), nullptr);

            m_overrideEntryContainer = GTK_WIDGET(gtk_builder_get_object(builderInterfaceSettingCollection, settingOverrideWidgetName.c_str()));

            std::vector<GtkWidget*> widgets;
            gtk_container_foreach(GTK_CONTAINER(m_overrideEntryContainer), CollectWidgetCB, &widgets);

            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(m_overrideEntryContainer)), m_overrideEntryContainer);
            gtk_container_add(fileOverrideContainer, m_overrideEntryContainer);
            m_overrideEntry = GTK_ENTRY(widgets[0]);


            g_signal_connect(G_OBJECT(m_fileOverrideCheck), "toggled", G_CALLBACK(onFileOverrideCheckToggled), GTK_WIDGET(m_settingsTable));
            g_signal_connect(G_OBJECT(widgets[1]), "clicked", G_CALLBACK(OnOverrideBrowseClicked), this);
            g_signal_connect(G_OBJECT(buttonLoad), "clicked", G_CALLBACK(OnButtonLoadClicked), this);
            g_signal_connect(G_OBJECT(buttonSave), "clicked", G_CALLBACK(OnButtonSaveClicked), this);

            if (m_box.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename)) {
                GtkExpander* expander = GTK_EXPANDER(gtk_builder_get_object(builder, "box_configuration-expander"));
                gtk_expander_set_expanded(expander, true);

                gtk_entry_set_text(m_overrideEntry, m_box.getAttributeValue(OV_AttributeId_Box_SettingOverrideFilename).toASCIIString());
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_fileOverrideCheck), true);
                gtk_widget_set_sensitive(GTK_WIDGET(m_settingsTable), false);
            }
            else { gtk_entry_set_text(m_overrideEntry, ""); }

            g_object_unref(builder);
            g_object_unref(builderInterfaceSettingCollection);
        }
    }
}

CBoxConfigurationDialog::~CBoxConfigurationDialog()
{
    m_box.deleteObserver(this);
    if (m_settingDialog) { gtk_widget_destroy(m_settingDialog); }
}

bool CBoxConfigurationDialog::Run()
{
    bool modified = false;
    if (m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting)) {
        //CSettingCollectionHelper helper(m_kernelCtx, m_guiSettingsFilename.toASCIIString());
        StoreState();
        bool finished = false;
        while (!finished) {
            const gint result = gtk_dialog_run(GTK_DIALOG(m_settingDialog));
            if (result == GTK_RESPONSE_APPLY) {
                if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_fileOverrideCheck))) {
                    const gchar* fileName = gtk_entry_get_text(m_overrideEntry);
                    if (m_box.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename)) {
                        m_box.setAttributeValue(OV_AttributeId_Box_SettingOverrideFilename, fileName);
                    }
                    else { m_box.addAttribute(OV_AttributeId_Box_SettingOverrideFilename, fileName); }
                }
                else {
                    if (m_box.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename)) { m_box.removeAttribute(OV_AttributeId_Box_SettingOverrideFilename); }
                }

                finished = true;
                modified = true;
            }
            else if (result == GTK_RESPONSE_CANCEL) {
                RestoreState();
                finished = true;
            }
            else if (result == 1) // default
            {
                // Some settings will add/remove other settings;
                // by evaluating m_box.getSettingCount() each time we ensure not ending somewhere in the oblivion
                for (size_t i = 0; i < m_settingViews.size(); ++i) {
                    CString value;
                    m_box.getSettingDefaultValue(i, value);
                    m_box.setSettingValue(i, value);
                    //m_settingViews[i]->setValue(value);
                    //helper.setValue(settingType, i < m_settingViews.size()? m_settingViews[i]->getEntryWidget() : nullptr, value);
                }
                gtk_entry_set_text(GTK_ENTRY(m_overrideEntryContainer), "");
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_fileOverrideCheck), false);
                //gtk_widget_set_sensitive(GTK_WIDGET(m_settingsTable), true);
                modified = false;
            }
            else if (result == 2) // revert
            {
                RestoreState();

                if (m_box.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename)) {
                    gtk_entry_set_text(
                        GTK_ENTRY(m_overrideEntryContainer), m_box.getAttributeValue(OV_AttributeId_Box_SettingOverrideFilename).toASCIIString());
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_fileOverrideCheck), true);
                }
                else {
                    gtk_entry_set_text(GTK_ENTRY(m_overrideEntryContainer), "");
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_fileOverrideCheck), false);
                }
            }
            else if (result == 3) { modified = true; }  // load
            else if (result == 4) { }                   // save 
            else { finished = true; }
        }
    }
    return modified;
}

void CBoxConfigurationDialog::update(CObservable& /*o*/, void* data)
{
    const Kernel::BoxEventMessage* event = static_cast<Kernel::BoxEventMessage*>(data);

    switch (event->m_Type) {
        case Kernel::SettingsAllChange:
            generateSettingsTable();
            break;

        case Kernel::SettingValueUpdate:
        {
            CString value;
            m_box.getSettingValue(event->m_FirstIdx, value);

            m_settingViews[event->m_FirstIdx]->SetValue(value);
            break;
        }

        case Kernel::SettingDelete:
            removeSetting(event->m_FirstIdx);
            break;

        case Kernel::SettingAdd:
            addSetting(event->m_FirstIdx);
            break;

        case Kernel::SettingChange:
            settingChange(event->m_FirstIdx);
            break;

        case Kernel::SettingsReorder: break;
        default: break;     //OV_ERROR_KRF("wtf", Kernel::ErrorType::BadSetting);
    }
}

void CBoxConfigurationDialog::generateSettingsTable()
{
    std::for_each(m_settingViews.begin(), m_settingViews.end(), [](Setting::CAbstractSettingView* elem) { delete elem; });
    m_settingViews.clear();
    //Remove rows
    gtk_container_foreach(GTK_CONTAINER(GTK_WIDGET(m_settingsTable)),
                          [](GtkWidget* widget, gpointer data) { gtk_container_remove(GTK_CONTAINER(data), widget); },
                          GTK_WIDGET(m_settingsTable));

    size_t size = 0;
    if (m_isScenarioRunning) {
        for (size_t i = 0; i < m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting); ++i) {
            bool mod = false;
            m_box.getSettingMod(i, mod);
            if (mod) { size++; }
        }
    }
    else { size = m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting); }
    gtk_table_resize(m_settingsTable, guint(size + 2), 4);

    // Iterate over box settings, generate corresponding gtk widgets. If the scenario is running, we are making a
    // 'modifiable settings' dialog and use a subset of widgets with a slightly different layout and buttons.
    for (size_t settingIdx = 0, tableIdx = 0; settingIdx < m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting); ++settingIdx) {
        if (addSettingsToView(settingIdx, tableIdx)) { ++tableIdx; }
    }
    updateSize();
}

bool CBoxConfigurationDialog::addSettingsToView(const size_t settingIdx, const size_t tableIdx)
{
    bool modifiable;
    m_box.getSettingMod(settingIdx, modifiable);

    if ((!m_isScenarioRunning) || (m_isScenarioRunning && modifiable)) {
        CString name;

        m_box.getSettingName(settingIdx, name);
        Setting::CAbstractSettingView* view = m_settingFactory.GetSettingView(m_box, settingIdx);

        bool isDeprecated = false;
        m_box.getInterfacorDeprecatedStatus(Kernel::EBoxInterfacorType::Setting, settingIdx, isDeprecated);
        if (isDeprecated) {
            gtk_widget_set_sensitive(GTK_WIDGET(view->GetNameWidget()), false);
            gtk_widget_set_sensitive(GTK_WIDGET(view->GetEntryWidget()), false);
        }

        gtk_table_attach(m_settingsTable, view->GetNameWidget(), 0, 1, guint(tableIdx), guint(tableIdx + 1), GtkAttachOptions(GTK_FILL),
                         GtkAttachOptions(GTK_FILL), 0, 0);
        gtk_table_attach(m_settingsTable, view->GetEntryWidget(), 1, 4, guint(tableIdx), guint(tableIdx + 1),
                         GtkAttachOptions(GTK_SHRINK | GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_SHRINK), 0, 0);

        m_settingViews.insert(m_settingViews.begin() + tableIdx, view);

        return true;
    }
    return false;
}

void CBoxConfigurationDialog::settingChange(const size_t index)
{
    //We remeber the place to add the new setting at the same place
    const size_t indexTable = getTableIndex(index);

    removeSetting(index, false);
    addSettingsToView(index, indexTable);
}

void CBoxConfigurationDialog::addSetting(const size_t index)
{
    bool modifiable;
    m_box.getSettingMod(index, modifiable);

    if ((!m_isScenarioRunning) || (m_isScenarioRunning && modifiable)) {
        const size_t size = m_settingViews.size();
        /*There is two case.
        1) we just add at the end of the setting box
        2) we add it in the middle end we need to shift
        */
        const size_t tableIdx = (index > m_settingViews[size - 1]->GetSettingIndex()) ? size : getTableIndex(index);

        gtk_table_resize(m_settingsTable, guint(size + 2), 4);

        if (index <= m_settingViews[size - 1]->GetSettingIndex()) {
            for (size_t i = size - 1; i >= tableIdx; --i) {
                Setting::CAbstractSettingView* view = m_settingViews[i];

                //We need to update the index
                view->SetSettingIndex(view->GetSettingIndex() + 1);

                gtk_container_remove(GTK_CONTAINER(m_settingsTable), view->GetNameWidget());
                gtk_table_attach(m_settingsTable, view->GetNameWidget(), 0, 1, guint(i + 1), guint(i + 2), GtkAttachOptions(GTK_FILL),
                                 GtkAttachOptions(GTK_FILL), 0, 0);

                gtk_container_remove(GTK_CONTAINER(m_settingsTable), view->GetEntryWidget());
                gtk_table_attach(m_settingsTable, view->GetEntryWidget(), 1, 4, guint(i + 1), guint(i + 2),
                                 GtkAttachOptions(GTK_SHRINK | GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_SHRINK), 0, 0);
            }
        }
        addSettingsToView(size_t(tableIdx), index);
        updateSize();
    }
    //Even if nothing is add to the interface, we still need to update index
    else {
        for (Setting::CAbstractSettingView* view : m_settingViews) {
            if (view->GetSettingIndex() >= index) { view->SetSettingIndex(view->GetSettingIndex() + 1); }
        }
    }
}

void CBoxConfigurationDialog::removeSetting(const size_t index, const bool shift)
{
    const int tableIdx = getTableIndex(index);

    if (tableIdx != -1) {
        Setting::CAbstractSettingView* view = m_settingViews[tableIdx];
        GtkWidget* name                     = view->GetNameWidget();
        GtkWidget* entry                    = view->GetEntryWidget();

        gtk_container_remove(GTK_CONTAINER(m_settingsTable), name);
        gtk_container_remove(GTK_CONTAINER(m_settingsTable), entry);

        delete view;
        m_settingViews.erase(m_settingViews.begin() + tableIdx);

        //Now if we need to do it we shift everything to avoid an empty row in the table
        if (shift) {
            for (size_t i = tableIdx; i < m_settingViews.size(); ++i) {
                view = m_settingViews[i];
                view->SetSettingIndex(view->GetSettingIndex() - 1);

                gtk_container_remove(GTK_CONTAINER(m_settingsTable), view->GetNameWidget());
                gtk_table_attach(m_settingsTable, view->GetNameWidget(), 0, 1, guint(i), guint(i + 1), GtkAttachOptions(GTK_FILL), GtkAttachOptions(GTK_FILL),
                                 0, 0);

                gtk_container_remove(GTK_CONTAINER(m_settingsTable), view->GetEntryWidget());
                gtk_table_attach(m_settingsTable, view->GetEntryWidget(), 1, 4, guint(i), guint(i + 1), GtkAttachOptions(GTK_SHRINK | GTK_FILL | GTK_EXPAND),
                                 GtkAttachOptions(GTK_SHRINK), 0, 0);
            }
            //Now let's resize everything
            gtk_table_resize(m_settingsTable, guint(m_settingViews.size() + 2), 4);
            updateSize();
        }
    }
    //Even if we delete an "invisible" setting we need to update every index.
    else {
        for (Setting::CAbstractSettingView* view : m_settingViews) {
            if (view->GetSettingIndex() >= index) { view->SetSettingIndex(view->GetSettingIndex() - 1); }
        }
    }
}

int CBoxConfigurationDialog::getTableIndex(const size_t index)
{
    size_t tableIdx = 0;
    for (auto it = m_settingViews.begin(); it != m_settingViews.end(); ++it, ++tableIdx) {
        Setting::CAbstractSettingView* view = *it;
        if (view->GetSettingIndex() == index) { return int(index); }
    }

    return -1;
}

void CBoxConfigurationDialog::updateSize() const
{
    // Resize the window to fit as much of the table as possible, but keep the max size
    // limited so it doesn't get outside the screen. For safety, we cap to 800x600
    // anyway to hopefully prevent the window from going under things such as the gnome toolbar.
    // The ui file at the moment does not allow resize of this window because the result
    // looked ugly if the window was made overly large, and no satisfying solution at the time was
    // found by the limited intellectual resources available.
    const gint maxWidth  = std::min(800, gdk_screen_get_width(gdk_screen_get_default()));
    const gint maxHeight = std::min(600, gdk_screen_get_height(gdk_screen_get_default()));
    GtkRequisition size;
    gtk_widget_size_request(GTK_WIDGET(m_viewPort), &size);
    gtk_widget_set_size_request(GTK_WIDGET(m_scrolledWindow), gint(std::min(maxWidth, size.width)), gint(std::min(maxHeight, size.height)));
}

void CBoxConfigurationDialog::SaveConfig() const
{
    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Select file to save settings to...", nullptr, GTK_FILE_CHOOSER_ACTION_SAVE,
                                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, nullptr);

    const gchar* initialFileNameToExpand = gtk_entry_get_text(GTK_ENTRY(m_overrideEntryContainer));
    const CString initialFileName        = m_kernelCtx.getConfigurationManager().expand(initialFileNameToExpand);
    if (g_path_is_absolute(initialFileName.toASCIIString())) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widgetDialogOpen), initialFileName.toASCIIString());
    }
    else {
        char* fullPath = g_build_filename(g_get_current_dir(), initialFileName.toASCIIString(), nullptr);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widgetDialogOpen), fullPath);
        g_free(fullPath);
    }

    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        char* fileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widgetDialogOpen));

        tinyxml2::XMLDocument xmlDoc;
        tinyxml2::XMLNode* root = xmlDoc.NewElement(ROOT_NAME);         // Create root node
        xmlDoc.InsertFirstChild(root);                                  // Add root to XML
        for (size_t i = 0; i < m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting); ++i) {
            tinyxml2::XMLElement* e = xmlDoc.NewElement(SETTING_NAME);  // Create data node
            CString value;
            m_box.getSettingValue(i, value);
            e->SetText(value.toASCIIString());
            root->InsertEndChild(e);                                    // Add node to root
        }
        xmlDoc.SaveFile(fileName);

        g_free(fileName);
    }
    gtk_widget_destroy(widgetDialogOpen);
}

void CBoxConfigurationDialog::LoadConfig() const
{
    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Select file to load settings from...", nullptr, GTK_FILE_CHOOSER_ACTION_SAVE,
                                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);

    const gchar* filenameToExpand = gtk_entry_get_text(GTK_ENTRY(m_overrideEntryContainer));

    const CString filename = m_kernelCtx.getConfigurationManager().expand(filenameToExpand);
    if (g_path_is_absolute(filename.toASCIIString())) { gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widgetDialogOpen), filename.toASCIIString()); }
    else {
        char* fullPath = g_build_filename(g_get_current_dir(), filename.toASCIIString(), nullptr);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widgetDialogOpen), fullPath);
        g_free(fullPath);
    }

    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        char* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widgetDialogOpen));

        // Load File
        tinyxml2::XMLDocument xmlDoc;
        if (xmlDoc.LoadFile(path) == 0) {                   // Check File Exist and Loading
            tinyxml2::XMLNode* root = xmlDoc.FirstChild();  // Get Root Node
            if (root != nullptr) {                          // Check Root Node Exist
                size_t i                = 0;
                tinyxml2::XMLElement* e = root->FirstChildElement(SETTING_NAME);
                while (e != nullptr) {
                    m_box.setSettingValue(i++, e->GetText());
                    e = e->NextSiblingElement(SETTING_NAME);
                }
            }
        }

        g_free(path);
    }
    gtk_widget_destroy(widgetDialogOpen);
}

void CBoxConfigurationDialog::OnOverrideBrowse() const
{
    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Select file to open...", nullptr, GTK_FILE_CHOOSER_ACTION_SAVE,
                                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);

    const CString initialFilename = m_kernelCtx.getConfigurationManager().expand(gtk_entry_get_text(GTK_ENTRY(m_overrideEntry)));
    if (g_path_is_absolute(initialFilename.toASCIIString())) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widgetDialogOpen), initialFilename.toASCIIString());
    }
    else {
        char* fullPath = g_build_filename(g_get_current_dir(), initialFilename.toASCIIString(), nullptr);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widgetDialogOpen), fullPath);
        g_free(fullPath);
    }

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(widgetDialogOpen), false);

    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        gchar* name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widgetDialogOpen));
        std::string filename(name);
        g_free(name);
        std::replace(filename.begin(), filename.end(), '\\', '/');
        gtk_entry_set_text(GTK_ENTRY(m_overrideEntry), filename.c_str());
    }
    gtk_widget_destroy(widgetDialogOpen);
}


void CBoxConfigurationDialog::StoreState()
{
    m_settingsMemory.clear();
    for (size_t i = 0; i < m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting); ++i) {
        CString temp;
        m_box.getSettingValue(i, temp);
        m_settingsMemory.push_back(temp);
    }
}

void CBoxConfigurationDialog::RestoreState() const
{
    for (size_t i = 0; i < m_settingsMemory.size(); ++i) {
        if (i >= m_box.getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting)) { return; }  // This is not supposed to happen
        m_box.setSettingValue(i, m_settingsMemory[i]);
    }
}

}  // namespace Designer
}  // namespace OpenViBE
