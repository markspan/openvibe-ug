///-------------------------------------------------------------------------------------------------
/// 
/// \file CEnumerationSettingView.hpp
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

#include "../base.hpp"
#include "CAbstractSettingView.hpp"

#include <map>

namespace OpenViBE {
namespace Designer {
namespace Setting {
class CEnumerationSettingView final : public CAbstractSettingView
{
public:
    CEnumerationSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx, const CIdentifier& typeID);

    void GetValue(CString& value) const override;
    void SetValue(const CString& value) override;

    void OnChange();

private:
    GtkComboBox* m_comboBox = nullptr;
    CIdentifier m_typeID    = CIdentifier::undefined();

    std::map<CString, size_t> m_entriesIdx;

    const Kernel::IKernelContext& m_kernelCtx;
    bool m_onValueSetting = false;
};
}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
