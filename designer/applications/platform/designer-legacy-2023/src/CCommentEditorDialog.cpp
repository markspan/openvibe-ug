///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommentEditorDialog.cpp
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

#include "CCommentEditorDialog.hpp"

#include <cstring>

namespace OpenViBE {
namespace Designer {

static void BoldSelectionCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<b>", "</b>"); }
static void ItalicSelectionCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<i>", "</i>"); }
static void UnderlineCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<u>", "</u>"); }
static void StrikethroughCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<s>", "</s>"); }
static void MonoCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<tt>", "</tt>"); }
static void SubscriptCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<sub>", "</sub>"); }
static void SuperscriptCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<sup>", "</sup>"); }
static void BigCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<big>", "</big>"); }
static void SmallCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<small>", "</small>"); }
static void RedCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<span color=\"red\">", "</span>"); }
static void GreenCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<span color=\"green\">", "</span>"); }
static void BlueCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->ApplyTagCB("<span color=\"blue\">", "</span>"); }
static void InfoCB(GtkButton* /*button*/, gpointer data) { static_cast<CCommentEditorDialog*>(data)->HelpCB(); }

bool CCommentEditorDialog::Run()
{
    bool res = false;

    m_interface = gtk_builder_new(); // glade_xml_new(m_guiFilename.toASCIIString(), "comment", nullptr);
    gtk_builder_add_from_file(m_interface, m_guiFilename.toASCIIString(), nullptr);
    gtk_builder_connect_signals(m_interface, nullptr);

    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_bold"), "clicked", G_CALLBACK(BoldSelectionCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_italic"), "clicked", G_CALLBACK(ItalicSelectionCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_underline"), "clicked", G_CALLBACK(UnderlineCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_strikethrough"), "clicked", G_CALLBACK(StrikethroughCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_mono"), "clicked", G_CALLBACK(MonoCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_subscript"), "clicked", G_CALLBACK(SubscriptCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_superscript"), "clicked", G_CALLBACK(SuperscriptCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_big"), "clicked", G_CALLBACK(BigCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_small"), "clicked", G_CALLBACK(SmallCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_red"), "clicked", G_CALLBACK(RedCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_green"), "clicked", G_CALLBACK(GreenCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_blue"), "clicked", G_CALLBACK(BlueCB), this);
    ::g_signal_connect(gtk_builder_get_object(m_interface, "comment_toolbutton_info"), "clicked", G_CALLBACK(InfoCB), this);

    m_dialog = GTK_WIDGET(gtk_builder_get_object(m_interface, "comment"));
    m_desc   = GTK_WIDGET(gtk_builder_get_object(m_interface, "comment-textview_description"));

    m_infoDialog = GTK_WIDGET(gtk_builder_get_object(m_interface, "messagedialog_howto_comment"));
    ::g_signal_connect(m_infoDialog, "close", G_CALLBACK(gtk_widget_hide), nullptr);
    ::g_signal_connect(m_infoDialog, "delete-event", G_CALLBACK(gtk_widget_hide), nullptr);

    //::g_signal_connect(GTK_WIDGET(gtk_builder_get_object(m_interface, "messagedialog_howto_comment_button_close")), "clicked", G_CALLBACK(gtk_widget_hide), nullptr);

    g_object_unref(m_interface);

    m_buffer = gtk_text_buffer_new(nullptr);
    gtk_text_buffer_set_text(m_buffer, m_comment.getText().toASCIIString(), -1);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(m_desc), m_buffer);
    g_object_unref(m_buffer);

    gtk_widget_grab_focus(m_desc);

    const gint result = gtk_dialog_run(GTK_DIALOG(m_dialog));
    if (result == GTK_RESPONSE_APPLY) {
        res = true;
        GtkTextIter start, end;
        m_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_desc));
        gtk_text_buffer_get_start_iter(m_buffer, &start);
        gtk_text_buffer_get_end_iter(m_buffer, &end);
        m_comment.setText(gtk_text_buffer_get_text(m_buffer, &start, &end, TRUE));
    }
    gtk_widget_destroy(m_infoDialog);
    gtk_widget_destroy(m_dialog);

    return res;
}

//-----------------------------------------------------------------------------------
void CCommentEditorDialog::ApplyTagCB(const char* in, const char* out) const
{
    GtkTextIter start, end;

    if (gtk_text_buffer_get_has_selection(m_buffer)) {
        gtk_text_buffer_get_selection_bounds(m_buffer, &start, &end);
        gtk_text_buffer_insert(m_buffer, &start, in, gint(strlen(in)));
        gtk_text_buffer_get_selection_bounds(m_buffer, &start, &end);
        gtk_text_buffer_insert(m_buffer, &end, out, gint(strlen(out)));

        // reset selection to the selected text, as the tagOut is now selected
        gtk_text_buffer_get_selection_bounds(m_buffer, &start, &end);
        gtk_text_iter_backward_chars(&end, gint(strlen(out)));
        gtk_text_buffer_select_range(m_buffer, &start, &end);
    }
    else {
        gtk_text_buffer_get_selection_bounds(m_buffer, &start, &end);
        const gint offset = gtk_text_iter_get_offset(&start);

        gtk_text_buffer_insert_at_cursor(m_buffer, in, gint(strlen(in)));
        gtk_text_buffer_insert_at_cursor(m_buffer, out, gint(strlen(out)));

        gtk_text_buffer_get_iter_at_offset(m_buffer, &start, gint(offset + strlen(in)));
        gtk_text_buffer_place_cursor(m_buffer, &start);
    }

    // set focus on the text, to get back in edition mode directly
    gtk_widget_grab_focus(m_desc);
}

}  // namespace Designer
}  // namespace OpenViBE
