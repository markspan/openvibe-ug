///-------------------------------------------------------------------------------------------------
/// 
/// \file CInputDialog.hpp
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

namespace OpenViBE {
namespace Designer {
typedef void (*fpButtonCB)(GtkWidget* pWidget, gpointer data);

class CInputDialog
{
public:
    CInputDialog(const char* gtkBuilder, fpButtonCB okButtonCB, void* data = nullptr, const char* title = nullptr, const char* label = nullptr,
                 const char* entry                                         = nullptr);
    ~CInputDialog();

    void Run();
    void* GetUserData() const { return m_userData; }
    const char* GetEntry() const { return static_cast<const char*>(gtk_entry_get_text(m_dialogEntry)); }

private:
    static gboolean keyPressEventCB(GtkWidget* widget, GdkEventKey* eventKey, gpointer data);
    static void buttonClickedCB(GtkButton* button, gpointer data);
    void buttonClicked(GtkButton* button) const;

    void* m_userData = nullptr;
    fpButtonCB m_okButtonCB;
    GtkDialog* m_dialog             = nullptr;
    GtkLabel* m_dialogLabel         = nullptr;
    GtkEntry* m_dialogEntry         = nullptr;
    GtkButton* m_dialogOkButton     = nullptr;
    GtkButton* m_dialogCancelButton = nullptr;
};
}  // namespace Designer
}  // namespace OpenViBE
