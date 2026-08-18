///-------------------------------------------------------------------------------------------------
/// 
/// \file CLogListenerDesigner.cpp
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

#include "CLogListenerDesigner.hpp"

#include <iostream>
#include <sstream>
#include <cmath>

namespace {
void CloseMessagesAlertWindowCB(GtkButton* /*button*/, gpointer data) { gtk_widget_hide(GTK_WIDGET(data)); }
void FocusMessageWindowCB(GtkButton* /*button*/, gpointer data) { static_cast<OpenViBE::Designer::CLogListenerDesigner*>(data)->FocusMessageWindow(); }

void RefreshSearchLOGEntry(GtkEntry* text, gpointer data)
{
    auto* ptr         = static_cast<OpenViBE::Designer::CLogListenerDesigner*>(data);
    ptr->m_SearchTerm = gtk_entry_get_text(text);
    ptr->SearchMessages(ptr->m_SearchTerm);
}

void FocusOnBoxCidentifierClicked(GtkWidget* widget, const GdkEventButton* event, gpointer data)
{
    //log text view grab the focus so isLogAreaClicked() return true and CTRL+F will focus on the log searchEntry
    gtk_widget_grab_focus(widget);

    const auto* ptr = static_cast<OpenViBE::Designer::CLogListenerDesigner*>(data);

    //if left click
    if (event->button == 1) {
        GtkTextView* textView              = GTK_TEXT_VIEW(widget);
        const GtkTextWindowType windowType = gtk_text_view_get_window_type(textView, event->window);
        gint bufferX, bufferY;
        //convert event coord (mouse position) in buffer coord (character in buffer)
        gtk_text_view_window_to_buffer_coords(textView, windowType, gint(round(event->x)), gint(round(event->y)), &bufferX, &bufferY);
        //get the text iter corresponding to that position
        GtkTextIter iter;
        gtk_text_view_get_iter_at_location(textView, &iter, bufferX, bufferY);

        //if this position is not tagged, exit
        if (!gtk_text_iter_has_tag(&iter, ptr->m_IdTag)) { return; }
        //the position is tagged, we are on a CIdentifier
        GtkTextIter start = iter;
        GtkTextIter end   = iter;

        while (gtk_text_iter_has_tag(&end, ptr->m_IdTag)) { gtk_text_iter_forward_char(&end); }
        while (gtk_text_iter_has_tag(&start, ptr->m_IdTag)) { gtk_text_iter_backward_char(&start); }
        //we went one char to far for start
        gtk_text_iter_forward_char(&start);
        //this contains the CIdentifier
        const std::string link = gtk_text_iter_get_text(&start, &end);
        //std::cout << "cid is |" << link << "|" << std::endl;
        OpenViBE::CIdentifier id;
        id.fromString(link);
        ptr->m_CenterOnBoxFun(id);
    }
}
}  // namespace

namespace OpenViBE {
namespace Designer {

void CLogListenerDesigner::SearchMessages(const CString& searchTerm)
{
    //clear displayed buffer
    gtk_text_buffer_set_text(m_buffer, "", -1);
    m_SearchTerm = searchTerm;
    for (CLogObject* log : m_storedLogs) { if (log->Filter(searchTerm)) { AppendLog(log); } }   //display the log
}

void CLogListenerDesigner::AppendLog(const CLogObject* log) const
{
    GtkTextIter endIter, begin, end;
    gtk_text_buffer_get_end_iter(m_buffer, &endIter);
    //get log buffer bounds
    gtk_text_buffer_get_start_iter(log->GetTextBuffer(), &begin);
    gtk_text_buffer_get_end_iter(log->GetTextBuffer(), &end);
    //copy at the end of the displayed buffer
    gtk_text_buffer_insert_range(m_buffer, &endIter, &begin, &end);
}

CLogListenerDesigner::CLogListenerDesigner(const Kernel::IKernelContext& ctx, GtkBuilder* builder)
    : m_SearchTerm(""), m_CenterOnBoxFun([](CIdentifier& /*id*/) {}), m_builder(builder)
{
    m_textView    = GTK_TEXT_VIEW(gtk_builder_get_object(m_builder, "openvibe-textview_messages"));
    m_alertWindow = GTK_WINDOW(gtk_builder_get_object(m_builder, "dialog_error_popup"));

    m_buttonPopup = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_alert_on_error"));

    m_labelnMsg            = GTK_LABEL(gtk_builder_get_object(m_builder, "openvibe-messages_count_message_label"));
    m_labelnWarnings       = GTK_LABEL(gtk_builder_get_object(m_builder, "openvibe-messages_count_warning_label"));
    m_labelnErrors         = GTK_LABEL(gtk_builder_get_object(m_builder, "openvibe-messages_count_error_label"));
    m_labelDialognWarnings = GTK_LABEL(gtk_builder_get_object(m_builder, "dialog_error_popup-warning_count"));
    m_labelDialognErrors   = GTK_LABEL(gtk_builder_get_object(m_builder, "dialog_error_popup-error_count"));

    m_imageWarnings = GTK_WIDGET(gtk_builder_get_object(m_builder, "openvibe-messages_count_warning_image"));
    m_imageErrors   = GTK_WIDGET(gtk_builder_get_object(m_builder, "openvibe-messages_count_error_image"));

    m_buttonActiveDebug            = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_tb_debug"));
    m_buttonActiveBenchmark        = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_tb_bench"));
    m_buttonActiveTrace            = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_tb_trace"));
    m_buttonActiveInfo             = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_tb_info"));
    m_buttonActiveWarning          = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_tb_warning"));
    m_buttonActiveImportantWarning = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_tb_impwarning"));
    m_buttonActiveError            = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_tb_error"));
    m_buttonActiveFatal            = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(m_builder, "openvibe-messages_tb_fatal"));

    // set the popup-on-error checkbox according to the configuration token
    gtk_toggle_button_set_active(m_buttonPopup, bool(ctx.getConfigurationManager().expandAsBoolean("${Designer_PopUpOnError}")));

    g_signal_connect(G_OBJECT(m_alertWindow), "delete_event", G_CALLBACK(gtk_widget_hide), nullptr);
    g_signal_connect(G_OBJECT(gtk_builder_get_object(m_builder, "dialog_error_popup-button_view")), "clicked", G_CALLBACK(FocusMessageWindowCB), this);
    g_signal_connect(G_OBJECT(gtk_builder_get_object(m_builder, "dialog_error_popup-button_ok")), "clicked",
                     G_CALLBACK(CloseMessagesAlertWindowCB), m_alertWindow);

    g_signal_connect(G_OBJECT(gtk_builder_get_object(m_builder, "searchEntry")), "changed", G_CALLBACK(RefreshSearchLOGEntry), this);
    g_signal_connect(G_OBJECT(m_textView), "button-press-event", G_CALLBACK(FocusOnBoxCidentifierClicked), this);
    m_buffer = gtk_text_view_get_buffer(m_textView);

    gtk_text_buffer_create_tag(m_buffer, "f_mono", "family", "monospace", nullptr);
    gtk_text_buffer_create_tag(m_buffer, "w_bold", "weight", PANGO_WEIGHT_BOLD, nullptr);
    gtk_text_buffer_create_tag(m_buffer, "c_blue", "foreground", "#0000FF", nullptr);           // debug
    gtk_text_buffer_create_tag(m_buffer, "c_magenta", "foreground", "#FF00FF", nullptr);        // benchmark
    gtk_text_buffer_create_tag(m_buffer, "c_darkOrange", "foreground", "#FF9000", nullptr);     // important warning
    gtk_text_buffer_create_tag(m_buffer, "c_red", "foreground", "#FF0000", nullptr);            // error, fatal
    gtk_text_buffer_create_tag(m_buffer, "c_watercourse", "foreground", "#008238", nullptr);    // trace
    gtk_text_buffer_create_tag(m_buffer, "c_aqua", "foreground", "#00FFFF", nullptr);           // number
    gtk_text_buffer_create_tag(m_buffer, "c_darkViolet", "foreground", "#6900D7", nullptr);     // warning
    gtk_text_buffer_create_tag(m_buffer, "c_blueChill", "foreground", "#3d889b", nullptr);      // information
    gtk_text_buffer_create_tag(m_buffer, "link", "underline", PANGO_UNDERLINE_SINGLE, nullptr); // link for CIdentifier

    GtkTextTagTable* tagTable = gtk_text_buffer_get_tag_table(m_buffer);
    m_IdTag                   = gtk_text_tag_table_lookup(tagTable, "link");

    m_logWithHexa      = ctx.getConfigurationManager().expandAsBoolean("${Designer_ConsoleLogWithHexa}", false);
    m_logTimeInSecond  = ctx.getConfigurationManager().expandAsBoolean("${Kernel_ConsoleLogTimeInSecond}", false);
    m_logTimePrecision = size_t(ctx.getConfigurationManager().expandAsUInteger("${Designer_ConsoleLogTimePrecision}", 3));
}

bool CLogListenerDesigner::isActive(const Kernel::ELogLevel level)
{
    const auto it = m_activeLevels.find(level);
    if (it == m_activeLevels.end()) { return true; }
    return it->second;
}

bool CLogListenerDesigner::activate(const Kernel::ELogLevel level, const bool active)
{
    m_activeLevels[level] = active;
    return true;
}

bool CLogListenerDesigner::activate(const Kernel::ELogLevel startLevel, const Kernel::ELogLevel endLevel, const bool active)
{
    for (int i = startLevel; i <= endLevel; ++i) { m_activeLevels[Kernel::ELogLevel(i)] = active; }
    return true;
}

bool CLogListenerDesigner::activate(const bool active) { return activate(Kernel::LogLevel_First, Kernel::LogLevel_Last, active); }

void CLogListenerDesigner::log(const CTime value)
{
    if (m_ignoreMsg) { return; }
    checkAppendFilterCurrentLog("c_watercourse", value.str(m_logTimeInSecond, m_logWithHexa).c_str());
}

#if defined __clang__
void CLogListenerDesigner::log(const size_t value)
{
    if (m_ignoreMsg) { return; }

    std::stringstream txt;
    txt << std::dec << value;
    if (m_logWithHexa) { txt << " (0x" << std::hex << value << ")"; }

    checkAppendFilterCurrentLog("c_watercourse", txt.str().c_str());
}
#endif

void CLogListenerDesigner::log(const uint64_t value)
{
    if (m_ignoreMsg) { return; }

    std::stringstream txt;
    txt << std::dec << value;
    if (m_logWithHexa) { txt << " (0x" << std::hex << value << ")"; }

    checkAppendFilterCurrentLog("c_watercourse", txt.str().c_str());
}

void CLogListenerDesigner::log(const uint32_t value)
{
    if (m_ignoreMsg) { return; }

    std::stringstream txt;
    txt << std::dec << value;
    if (m_logWithHexa) { txt << " (0x" << std::hex << value << ")"; }

    checkAppendFilterCurrentLog("c_watercourse", txt.str().c_str());
}

void CLogListenerDesigner::log(const int64_t value)
{
    if (m_ignoreMsg) { return; }

    std::stringstream txt;
    txt << std::dec << value;
    if (m_logWithHexa) { txt << " (0x" << std::hex << value << ")"; }

    checkAppendFilterCurrentLog("c_watercourse", txt.str().c_str());
}

void CLogListenerDesigner::log(const int value)
{
    if (m_ignoreMsg) { return; }

    std::stringstream txt;
    txt << std::dec << value;
    if (m_logWithHexa) { txt << " (0x" << std::hex << value << ")"; }

    checkAppendFilterCurrentLog("c_watercourse", txt.str().c_str());
}

void CLogListenerDesigner::log(const double value)
{
    if (m_ignoreMsg) { return; }
    checkAppendFilterCurrentLog("c_watercourse", std::to_string(value).c_str());
}

void CLogListenerDesigner::log(const bool value)
{
    if (m_ignoreMsg) { return; }
    checkAppendFilterCurrentLog("c_watercourse", (value ? "true" : "false"));
}

void CLogListenerDesigner::log(const CIdentifier& value)
{
    if (m_ignoreMsg) { return; }
    checkAppendFilterCurrentLog("c_blueChill", value.str().c_str(), true);
}

void CLogListenerDesigner::log(const CString& value)
{
    if (m_ignoreMsg) { return; }
    checkAppendFilterCurrentLog("c_blueChill", value.toASCIIString());
}

void CLogListenerDesigner::log(const std::string& value)
{
    if (m_ignoreMsg) { return; }
    checkAppendFilterCurrentLog(nullptr, value.c_str());
}

void CLogListenerDesigner::log(const char* value)
{
    if (m_ignoreMsg) { return; }
    checkAppendFilterCurrentLog(nullptr, value);
}

void CLogListenerDesigner::log(const Kernel::ELogLevel level)
{
    // GtkTextIter textIter;
    // gtk_text_buffer_get_end_iter(m_Buffer, &l_oTextIter);

    //new log, will be deleted when m_storedLogs is cleared
    m_currentLog = new CLogObject(m_buffer);//m_pNonFilteredBuffer);

    //copy this newly added content in the current log
    GtkTextIter endIter;
    gtk_text_buffer_get_end_iter(m_currentLog->GetTextBuffer(), &endIter);

    const auto addTagName = [this, &endIter](GtkToggleToolButton* activeButton, size_t& countVariable, const char* state, const char* color)
    {
        m_ignoreMsg = !gtk_toggle_tool_button_get_active(activeButton);
        if (m_ignoreMsg) { return; }

        countVariable++;
        gtk_text_buffer_insert_with_tags_by_name(m_currentLog->GetTextBuffer(), &endIter, "[ ", -1, "w_bold", "f_mono", nullptr);
        gtk_text_buffer_insert_with_tags_by_name(m_currentLog->GetTextBuffer(), &endIter, state, -1, "w_bold", "f_mono", color, nullptr);
        gtk_text_buffer_insert_with_tags_by_name(m_currentLog->GetTextBuffer(), &endIter, " ] ", -1, "w_bold", "f_mono", nullptr);
    };

    switch (level) {
        case Kernel::LogLevel_Debug:
            addTagName(m_buttonActiveDebug, m_nMsg, "DEBUG", "c_blue");
            break;

        case Kernel::LogLevel_Benchmark:
            addTagName(m_buttonActiveBenchmark, m_nMsg, "BENCH", "c_magenta");
            break;

        case Kernel::LogLevel_Trace:
            addTagName(m_buttonActiveTrace, m_nMsg, "TRACE", "c_watercourse");
            break;

        case Kernel::LogLevel_Info:
            addTagName(m_buttonActiveInfo, m_nMsg, "INF", "c_blueChill");
            break;

        case Kernel::LogLevel_Warning:
            addTagName(m_buttonActiveWarning, m_nWarning, "WARNING", "c_darkViolet");
            break;

        case Kernel::LogLevel_ImportantWarning:
            addTagName(m_buttonActiveImportantWarning, m_nWarning, "WARNING", "c_darkOrange");
            break;

        case Kernel::LogLevel_Error:
            addTagName(m_buttonActiveError, m_nError, "ERROR", "c_red");
            break;

        case Kernel::LogLevel_Fatal:
            addTagName(m_buttonActiveFatal, m_nError, "FATAL", "c_red");
            break;

        case Kernel::LogLevel_First: break;
        case Kernel::LogLevel_None: break;
        case Kernel::LogLevel_Last: break;

        default:
            addTagName(nullptr, m_nMsg, "UNKNOWN", nullptr);
            break;
    }

    if (gtk_toggle_button_get_active(m_buttonPopup) && (level == Kernel::LogLevel_Warning || level == Kernel::LogLevel_ImportantWarning
                                                        || level == Kernel::LogLevel_Error || level == Kernel::LogLevel_Fatal)) {
        if (!gtk_widget_get_visible(GTK_WIDGET(m_alertWindow))) {
            gtk_window_set_position(GTK_WINDOW(m_alertWindow), GTK_WIN_POS_CENTER);
            gtk_window_present(GTK_WINDOW(m_alertWindow));
            gtk_window_set_keep_above(GTK_WINDOW(m_alertWindow), true);
        }
    }
    m_storedLogs.push_back(m_currentLog);

    //see if the log passes the filter
    const bool passFilter = m_currentLog->Filter(m_SearchTerm);
    //if it does mark this position and insert the log in the text buffer displayed
    GtkTextIter endDisplayedTextIter;
    gtk_text_buffer_get_end_iter(m_buffer, &endDisplayedTextIter);
    gtk_text_buffer_create_mark(m_buffer, "current_log", &endDisplayedTextIter,
                                true);  //creating a mark will erase the previous one with the same name so no worry here
    if (passFilter) { DisplayLog(m_currentLog); }

    this->updateMessageCounts();

    GtkTextMark mark;
    mark = *(gtk_text_buffer_get_mark(gtk_text_view_get_buffer(m_textView), "insert"));
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(m_textView), &mark, 0.0, FALSE, 0.0, 0.0);
}

void CLogListenerDesigner::log(const Kernel::ELogColor /*color*/) { }

void CLogListenerDesigner::updateMessageCounts() const
{
    std::stringstream ss;
    ss << "<b>" << m_nMsg << "</b> Message";

    if (m_nMsg > 1) { ss << "s"; }

    gtk_label_set_markup(m_labelnMsg, ss.str().data());

    if (m_nWarning > 0) {
        ss.str("");
        ss << "<b>" << m_nWarning << "</b> Warning";
        if (m_nWarning > 1) { ss << "s"; }

        gtk_label_set_markup(m_labelnWarnings, ss.str().data());
        gtk_label_set_markup(m_labelDialognWarnings, ss.str().data());
        gtk_widget_set_visible(GTK_WIDGET(m_labelnWarnings), true);
        gtk_widget_set_visible(GTK_WIDGET(m_imageWarnings), true);
    }

    if (m_nError > 0) {
        ss.str("");
        ss << "<b>" << m_nError << "</b> Error";
        if (m_nError > 1) { ss << "s"; }

        gtk_label_set_markup(m_labelnErrors, ss.str().data());
        gtk_label_set_markup(m_labelDialognErrors, ss.str().data());
        gtk_widget_set_visible(GTK_WIDGET(m_labelnErrors), true);
        gtk_widget_set_visible(GTK_WIDGET(m_imageErrors), true);
    }
}

void CLogListenerDesigner::ClearMessages()
{
    m_nMsg     = 0;
    m_nWarning = 0;
    m_nError   = 0;

    gtk_label_set_markup(m_labelnMsg, "<b>0</b> Messages");
    gtk_label_set_markup(m_labelnWarnings, "<b>0</b> Warnings");
    gtk_label_set_markup(m_labelnErrors, "<b>0</b> Errors");
    gtk_label_set_markup(m_labelDialognWarnings, "<b>0</b> Warnings");
    gtk_label_set_markup(m_labelDialognErrors, "<b>0</b> Errors");

    gtk_widget_set_visible(m_imageWarnings, false);
    gtk_widget_set_visible(GTK_WIDGET(m_labelnWarnings), false);
    gtk_widget_set_visible(m_imageErrors, false);
    gtk_widget_set_visible(GTK_WIDGET(m_labelnErrors), false);

    gtk_text_buffer_set_text(m_buffer, "", -1);

    std::for_each(m_storedLogs.begin(), m_storedLogs.end(), [](CLogObject* elem) { delete elem; });
    m_storedLogs.clear();

    m_currentLog = nullptr;
}

void CLogListenerDesigner::FocusMessageWindow() const
{
    gtk_widget_hide(GTK_WIDGET(m_alertWindow));
    gtk_expander_set_expanded(GTK_EXPANDER(gtk_builder_get_object(m_builder, "openvibe-expander_messages")), true);
}


void CLogListenerDesigner::checkAppendFilterCurrentLog(const char* textColor, const char* logMessage, const bool isLink) const
{
    if (!m_currentLog) {
        std::cout << "Ouch, current log had been deleted before creating new, this shouldn't happen...\n";
        return;
    }
    m_currentLog->AppendToCurrentLog(textColor, logMessage, isLink);

    if (m_currentLog->Filter(m_SearchTerm)) { DisplayLog(m_currentLog); }
}

void CLogListenerDesigner::DisplayLog(CLogObject* log) const
{
    GtkTextMark* mark = gtk_text_buffer_get_mark(m_buffer, "current_log");
    GtkTextIter iter, endIter, begin, end;
    gtk_text_buffer_get_iter_at_mark(m_buffer, &iter, mark);
    gtk_text_buffer_get_end_iter(m_buffer, &endIter);

    //delete what after the mark
    gtk_text_buffer_delete(m_buffer, &iter, &endIter);
    //get iter
    gtk_text_buffer_get_iter_at_mark(m_buffer, &iter, mark);
    //rewrite the log
    gtk_text_buffer_get_start_iter(log->GetTextBuffer(), &begin);
    gtk_text_buffer_get_end_iter(log->GetTextBuffer(), &end);
    gtk_text_buffer_insert_range(m_buffer, &iter, &begin, &end);
}

}  // namespace Designer
}  // namespace OpenViBE
