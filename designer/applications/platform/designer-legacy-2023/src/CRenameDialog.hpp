///-------------------------------------------------------------------------------------------------
/// 
/// \file CRenameDialog.hpp
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
class CRenameDialog final
{
public:
    CRenameDialog(const Kernel::IKernelContext& ctx, const CString& initialName, const CString& defaultName, const char* guiFilename)
        : m_kernelCtx(ctx), m_initialName(initialName), m_defaultName(defaultName), m_result(initialName), m_guiFilename(guiFilename) { }

    ~CRenameDialog() = default;

    bool Run();
    CString GetResult() const { return m_result; }

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    CString m_initialName;
    CString m_defaultName;
    CString m_result;
    CString m_guiFilename;

private:
    CRenameDialog() = delete;
};
}  // namespace Designer
}  // namespace OpenViBE
