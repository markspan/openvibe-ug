///-------------------------------------------------------------------------------------------------
/// 
/// \file CSettingEditorDialog.hpp
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

#include "CSettingCollectionHelper.hpp"

#include <string>
#include <map>

namespace OpenViBE {
namespace Designer {
class CSettingEditorDialog final
{
public:
    CSettingEditorDialog(const Kernel::IKernelContext& ctx, Kernel::IBox& box, const size_t index, const char* title,
                         const char* guiFilename, const char* guiSettingsFilename)
        : m_kernelCtx(ctx), m_box(box), m_helper(ctx, guiFilename), m_settingIdx(index), m_guiFilename(guiFilename),
          m_guiSettingsFilename(guiSettingsFilename), m_title(title) { }

    ~CSettingEditorDialog() = default;

    bool Run();
    void TypeChangedCB();

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    Kernel::IBox& m_box;
    CSettingCollectionHelper m_helper;
    size_t m_settingIdx = 0;
    CString m_guiFilename;
    CString m_guiSettingsFilename;
    std::string m_title;
    GtkWidget* m_table        = nullptr;
    GtkWidget* m_type         = nullptr;
    GtkWidget* m_defaultValue = nullptr;
    std::map<std::string, CIdentifier> m_settingTypes;
};
}  // namespace Designer
}  // namespace OpenViBE
