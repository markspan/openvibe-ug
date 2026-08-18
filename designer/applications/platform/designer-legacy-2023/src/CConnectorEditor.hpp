///-------------------------------------------------------------------------------------------------
/// 
/// \file CConnectorEditor.hpp
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
class CConnectorEditor final
{
public:
    CConnectorEditor(const Kernel::IKernelContext& ctx, Kernel::IBox& box, const size_t type, const size_t index, const char* title, const char* guiFilename)
        : m_Box(box), m_kernelCtx(ctx), m_type(type), m_index(index), m_guiFilename(guiFilename), m_title(title ? title : "") { }

    ~CConnectorEditor() = default;
    bool Run();

    Kernel::IBox& m_Box;
    GtkEntry* m_IDEntry = nullptr;

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    size_t m_type  = 0;
    size_t m_index = 0;
    const std::string m_guiFilename;
    const std::string m_title;

    typedef bool (Kernel::IBox::*get_identifier_t)(size_t index, CIdentifier& identifier) const;
    typedef bool (Kernel::IBox::*get_type_t)(size_t index, CIdentifier& typeID) const;
    typedef bool (Kernel::IBox::*get_name_t)(size_t index, CString& name) const;
    typedef bool (Kernel::IBox::*set_type_t)(size_t index, const CIdentifier& typeID);
    typedef bool (Kernel::IBox::*set_name_t)(size_t index, const CString& name);
    typedef bool (Kernel::IBox::*is_type_supported_t)(const CIdentifier& typeID) const;
    typedef bool (Kernel::IBox::*update_identifier_t)(size_t index, const CIdentifier& newID);
};
}  //namespace Designer
}  //namespace OpenViBE
