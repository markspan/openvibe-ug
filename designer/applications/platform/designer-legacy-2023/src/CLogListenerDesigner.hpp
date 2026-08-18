///-------------------------------------------------------------------------------------------------
/// 
/// \file CLogListenerDesigner.hpp
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

#include <vector>
#include <map>

#define OVK_ClassId_Designer_LogListener        OpenViBE::CIdentifier(0x0FE155FA, 0x313C17A7)

namespace OpenViBE {
namespace Designer {

class CApplication;

class CLogListenerDesigner final : public Kernel::ILogListener
{
public:
    class CLogObject
    {
    public:
        explicit CLogObject(GtkTextBuffer* buffer) : m_Buffer(gtk_text_buffer_new(gtk_text_buffer_get_tag_table(buffer))) { }
        GtkTextBuffer* GetTextBuffer() const { return m_Buffer; }

        //determine if the log contains the searchTerm and tag the part with the sSerachTerm in gray
        bool Filter(const CString& searchTerm)
        {
            m_PassedFilter = false;
            GtkTextIter startFind, endFind;
            gtk_text_buffer_get_start_iter(m_Buffer, &startFind);
            gtk_text_buffer_get_end_iter(m_Buffer, &endFind);

            //tag for highlighting the search term
            GtkTextTag* tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(m_Buffer), "gray_bg");
            if (tag == nullptr) { gtk_text_buffer_create_tag(m_Buffer, "gray_bg", "background", "gray", nullptr); }

            //remove previous tagging
            gtk_text_buffer_remove_tag_by_name(m_Buffer, "gray_bg", &startFind, &endFind);

            //no term means no research so no filter we let all pass
            if (searchTerm == CString("")) {
                m_PassedFilter = true;
                return m_PassedFilter;
            }


            GtkTextIter startMatch, endMatch;
            const gchar* text = searchTerm.toASCIIString();
            while (gtk_text_iter_forward_search(&startFind, text, GTK_TEXT_SEARCH_TEXT_ONLY, &startMatch, &endMatch, nullptr)) {
                gtk_text_buffer_apply_tag_by_name(m_Buffer, "gray_bg", &startMatch, &endMatch);
                //offset to end_match
                const int offset = gtk_text_iter_get_offset(&endMatch);
                //begin next search at end match
                gtk_text_buffer_get_iter_at_offset(m_Buffer, &startFind, offset);
                m_PassedFilter = true;
            }
            return m_PassedFilter;
        }

        void AppendToCurrentLog(const char* textColor, const char* logMessage, const bool isLink /* = false */) const
        {
            GtkTextIter endIter;
            gtk_text_buffer_get_end_iter(m_Buffer, &endIter);

            if (isLink) { gtk_text_buffer_insert_with_tags_by_name(m_Buffer, &endIter, logMessage, -1, "f_mono", textColor, "link", nullptr); }
            else { gtk_text_buffer_insert_with_tags_by_name(m_Buffer, &endIter, logMessage, -1, "f_mono", textColor, nullptr); }
        }

        GtkTextBuffer* m_Buffer = nullptr;
        bool m_PassedFilter     = false;    //by default the log does not pass the filter;
    };

    CLogListenerDesigner(const Kernel::IKernelContext& ctx, GtkBuilder* builder);

    bool isActive(const Kernel::ELogLevel level) override;
    bool activate(const Kernel::ELogLevel level, const bool active) override;
    bool activate(const Kernel::ELogLevel startLevel, const Kernel::ELogLevel endLevel, const bool active) override;
    bool activate(const bool active) override;

    void log(const CTime value) override;
#if defined __clang__
    void log(const size_t value) override;
#endif
    void log(const uint64_t value) override;
    void log(const uint32_t value) override;

    void log(const int64_t value) override;
    void log(const int value) override;

    void log(const double value) override;

    void log(const bool value) override;

    void log(const CIdentifier& value) override;
    void log(const CString& value) override;
    void log(const std::string& value) override;
    void log(const char* value) override;

    void log(const Kernel::ELogLevel level) override;
    void log(const Kernel::ELogColor color) override;

    void ClearMessages();
    void FocusMessageWindow() const;

    // TODO
    void SearchMessages(const CString& searchTerm);
    void DisplayLog(CLogObject* log) const;
    void AppendLog(const CLogObject* log) const;

    _IsDerivedFromClass_Final_(Kernel::ILogListener, CIdentifier::undefined())

    CString m_SearchTerm;
    GtkTextTag* m_IdTag = nullptr;
    std::function<void(CIdentifier&)> m_CenterOnBoxFun;

protected:
    std::map<Kernel::ELogLevel, bool> m_activeLevels;

    //logs
    std::vector<CLogObject*> m_storedLogs;

private:
    GtkBuilder* m_builder   = nullptr;
    GtkTextView* m_textView = nullptr;
    GtkTextBuffer* m_buffer = nullptr;

    GtkToggleButton* m_buttonPopup = nullptr;

    GtkToggleToolButton* m_buttonActiveDebug            = nullptr;
    GtkToggleToolButton* m_buttonActiveBenchmark        = nullptr;
    GtkToggleToolButton* m_buttonActiveTrace            = nullptr;
    GtkToggleToolButton* m_buttonActiveInfo             = nullptr;
    GtkToggleToolButton* m_buttonActiveWarning          = nullptr;
    GtkToggleToolButton* m_buttonActiveImportantWarning = nullptr;
    GtkToggleToolButton* m_buttonActiveError            = nullptr;
    GtkToggleToolButton* m_buttonActiveFatal            = nullptr;

    GtkLabel* m_labelnMsg            = nullptr;
    GtkLabel* m_labelnWarnings       = nullptr;
    GtkLabel* m_labelnErrors         = nullptr;
    GtkLabel* m_labelDialognWarnings = nullptr;
    GtkLabel* m_labelDialognErrors   = nullptr;

    GtkWidget* m_imageWarnings = nullptr;
    GtkWidget* m_imageErrors   = nullptr;

    GtkWindow* m_alertWindow = nullptr;

    bool m_ignoreMsg = false;

    size_t m_nMsg     = 0;
    size_t m_nWarning = 0;
    size_t m_nError   = 0;

    bool m_logWithHexa        = false;
    bool m_logTimeInSecond    = false;
    size_t m_logTimePrecision = 0;

    CLogObject* m_currentLog = nullptr;

    void updateMessageCounts() const;
    void checkAppendFilterCurrentLog(const char* textColor, const char* logMessage, bool isLink = false) const;
};

}  // namespace Designer
}  // namespace OpenViBE
