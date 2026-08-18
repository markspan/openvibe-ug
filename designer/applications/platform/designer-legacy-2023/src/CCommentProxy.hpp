///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommentProxy.hpp
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

#include <string>

namespace OpenViBE {
namespace Designer {
class CCommentProxy final
{
public:
    CCommentProxy(const Kernel::IKernelContext& ctx, const Kernel::IComment& comment);
    CCommentProxy(const Kernel::IKernelContext& ctx, Kernel::IScenario& scenario, const CIdentifier& commentID);
    ~CCommentProxy() { if (!m_applied) { this->Apply(); } }

    explicit operator Kernel::IComment*() const { return m_comment; }
    explicit operator const Kernel::IComment*() const { return m_constComment; }

    int GetWidth(GtkWidget* widget) const;
    int GetHeight(GtkWidget* widget) const;

    int GetXCenter() const { return m_centerX; }
    int GetYCenter() const { return m_centerY; }

    void SetCenter(int centerX, int centerY);

    void Apply();

    const char* GetLabel() const;

protected:
    static void updateSize(GtkWidget* widget, const char* text, int* xSize, int* ySize);

    const Kernel::IKernelContext& m_kernelCtx;
    const Kernel::IComment* m_constComment = nullptr;
    Kernel::IComment* m_comment            = nullptr;
    bool m_applied                         = false;
    int m_centerX                          = 0;
    int m_centerY                          = 0;
    mutable std::string m_label;
};
}  // namespace Designer
}  // namespace OpenViBE
