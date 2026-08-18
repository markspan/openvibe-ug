///-------------------------------------------------------------------------------------------------
/// 
/// \file CFloatSettingView.hpp
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

namespace OpenViBE {
namespace Designer {
namespace Setting {
class CFloatSettingView final : public CAbstractSettingView
{
public:
    CFloatSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx);

    void GetValue(CString& value) const override;
    void SetValue(const CString& value) override;

    void AdjustValue(double amount);
    void OnChange();


private:
    GtkEntry* m_entry = nullptr;

    const Kernel::IKernelContext& m_kernelCtx;
    bool m_onValueSetting = false;
};
}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
