///-------------------------------------------------------------------------------------------------
/// 
/// \file CColorGradientSettingView.hpp
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

#include <string>
#include <map>
#include <vector>

namespace OpenViBE {
namespace Designer {
namespace Setting {
typedef struct SColorGradientDataNode
{
    double percent;
    GdkColor color;
    GtkColorButton* colorButton;
    GtkSpinButton* spinButton;
} color_gradient_data_node_t;


class CColorGradientSettingView final : public CAbstractSettingView
{
public:
    CColorGradientSettingView(Kernel::IBox& box, const size_t index, const CString& builderName, const Kernel::IKernelContext& ctx);

    void GetValue(CString& value) const override;
    void SetValue(const CString& value) override;

    void ConfigurePressed();

    void InitializeGradient();
    void RefreshColorGradient() const;
    void AddColor();
    void RemoveColor();

    void SpinChange(GtkSpinButton* button);
    void ColorChange(GtkColorButton* button);

    void OnChange();


private:
    GtkEntry* m_entry = nullptr;
    const Kernel::IKernelContext& m_kernelCtx;
    CString m_builderName;

    GtkWidget* m_dialog      = nullptr;
    GtkWidget* m_container   = nullptr;
    GtkWidget* m_drawingArea = nullptr;
    std::vector<color_gradient_data_node_t> m_colorGradient;
    std::map<GtkColorButton*, size_t> m_colorButtons;
    std::map<GtkSpinButton*, size_t> m_spinButtons;

    bool m_onValueSetting = false;
};
}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
