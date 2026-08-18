///-------------------------------------------------------------------------------------------------
/// 
/// \file CScriptSettingView.cpp
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

#include "CScriptSettingView.hpp"
#include "../base.hpp"

#include <cstring>
#include <cstdlib>

namespace OpenViBE {
namespace Designer {
namespace Setting {

static void OnButtonSettingFilenameBrowsePressed(GtkButton* /*button*/, gpointer data) { static_cast<CScriptSettingView*>(data)->Browse(); }
static void OnButtonSettingScriptEditPressed(GtkButton* /*button*/, gpointer data) { static_cast<CScriptSettingView*>(data)->Edit(); }
static void OnChangeCB(GtkEntry* /*entry*/, gpointer data) { static_cast<CScriptSettingView*>(data)->OnChange(); }

#if defined TARGET_OS_Windows
static gboolean OnFocusOutEvent(GtkEntry* /*entry*/, GdkEvent* /*event*/, gpointer data)
{
    static_cast<CScriptSettingView*>(data)->OnFocusLost();
    return FALSE;
}
#endif

CScriptSettingView::CScriptSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx)
    : CAbstractSettingView(box, index, builderName, "settings_collection-hbox_setting_script"), m_kernelCtx(ctx)
{
    GtkWidget* settingWidget = CAbstractSettingView::getEntryFieldWidget();

    std::vector<GtkWidget*> widgets;
    CAbstractSettingView::extractWidget(settingWidget, widgets);
    m_entry = GTK_ENTRY(widgets[0]);

    g_signal_connect(G_OBJECT(m_entry), "changed", G_CALLBACK(OnChangeCB), this);
#if defined TARGET_OS_Windows
    // Only called for Windows path
    g_signal_connect(G_OBJECT(m_entry), "focus_out_event", G_CALLBACK(OnFocusOutEvent), this);
#endif
    g_signal_connect(G_OBJECT(widgets[1]), "clicked", G_CALLBACK(OnButtonSettingScriptEditPressed), this);
    g_signal_connect(G_OBJECT(widgets[2]), "clicked", G_CALLBACK(OnButtonSettingFilenameBrowsePressed), this);

    CAbstractSettingView::initializeValue();
}

void CScriptSettingView::GetValue(CString& value) const { value = CString(gtk_entry_get_text(m_entry)); }

void CScriptSettingView::SetValue(const CString& value)
{
    m_onValueSetting = true;
    gtk_entry_set_text(m_entry, value);
    m_onValueSetting = false;
}

void CScriptSettingView::Browse() const
{
    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Select file to open...", nullptr, GTK_FILE_CHOOSER_ACTION_SAVE,
                                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);

    const CString initialFileName = m_kernelCtx.getConfigurationManager().expand(gtk_entry_get_text(m_entry));
    if (g_path_is_absolute(initialFileName.toASCIIString())) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widgetDialogOpen), initialFileName.toASCIIString());
    }
    else {
        char* fullPath = g_build_filename(g_get_current_dir(), initialFileName.toASCIIString(), nullptr);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widgetDialogOpen), fullPath);
        g_free(fullPath);
    }

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(widgetDialogOpen), false);

    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        char* fileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widgetDialogOpen));
        char* backslash;
        while ((backslash = strchr(fileName, '\\')) != nullptr) { *backslash = '/'; }
        gtk_entry_set_text(m_entry, fileName);
        g_free(fileName);
    }
    gtk_widget_destroy(widgetDialogOpen);
}

void CScriptSettingView::Edit() const
{
    const CString fileName  = m_kernelCtx.getConfigurationManager().expand(gtk_entry_get_text(m_entry));
    const CString editorCmd = m_kernelCtx.getConfigurationManager().expand("${Designer_ScriptEditorCommand}");

    if (editorCmd != CString("")) {
        CString fullCmd = editorCmd + CString(" \"") + fileName + CString("\"");
#if defined TARGET_OS_Windows
        fullCmd = "START " + fullCmd;
#elif defined TARGET_OS_Linux
        fullCmd = fullCmd + " &";
#else
#endif
        if (system(fullCmd.toASCIIString()) < 0) { m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not run command " << fullCmd << "\n"; }
    }
}

void CScriptSettingView::OnChange()
{
    if (!m_onValueSetting) {
        const gchar* value = gtk_entry_get_text(m_entry);
        getBox().setSettingValue(GetSettingIndex(), value);
    }
}

#if defined TARGET_OS_Windows
void CScriptSettingView::OnFocusLost()
{
    // We replace antislash, interpreted as escape, by slash in Windows path
    if (!m_onValueSetting) {
        std::string fileName = gtk_entry_get_text(m_entry);
        auto it              = fileName.begin();

        while ((it = std::find(it, fileName.end(), '\\')) != fileName.end()) {
            if (it == std::prev(fileName.end())) {
                *it = '/';
                break;
            }
            if (*std::next(it) != '{' && *std::next(it) != '$' && *std::next(it) != '}') { *it = '/'; }

            std::advance(it, 1);
        }

        gtk_entry_set_text(m_entry, fileName.c_str());
        getBox().setSettingValue(this->GetSettingIndex(), fileName.c_str());
    }
}
#endif

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
