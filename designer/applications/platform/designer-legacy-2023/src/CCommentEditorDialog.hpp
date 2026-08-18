///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommentEditorDialog.hpp
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
class CCommentEditorDialog final
{
public:
    CCommentEditorDialog(const Kernel::IKernelContext& ctx, Kernel::IComment& comment, const char* guiFilename)
        : m_kernelCtx(ctx), m_comment(comment), m_guiFilename(guiFilename) { }

    ~CCommentEditorDialog() = default;

    bool Run();

    // Callback for text formatting
    void ApplyTagCB(const char* in, const char* out) const;

    // help formatting pango
    void HelpCB() const { gtk_widget_show(m_infoDialog); }
protected:
    const Kernel::IKernelContext& m_kernelCtx;
    Kernel::IComment& m_comment;
    CString m_guiFilename;

    GtkBuilder* m_interface = nullptr;
    GtkWidget* m_dialog     = nullptr;
    GtkWidget* m_infoDialog = nullptr;
    GtkWidget* m_desc       = nullptr;
    GtkTextBuffer* m_buffer = nullptr;

    CCommentEditorDialog() = delete;
};
}  // namespace Designer
}  // namespace OpenViBE
