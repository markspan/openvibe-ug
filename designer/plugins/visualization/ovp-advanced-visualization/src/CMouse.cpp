///-------------------------------------------------------------------------------------------------
/// 
/// \file CMouse.cpp
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

#include "CMouse.hpp"
#include "CBoxAlgorithmViz.hpp"

#include <cmath>

namespace OpenViBE {
namespace AdvancedVisualization {

void CMouse::MouseButton(CRendererContext& ctx, const int x, const int y, const int button, const int status)
{
    m_Buttons[button] = status;

    if (m_Buttons[1] == 2) {
        ctx.SetScaleVisibility(!ctx.GetScaleVisibility());
        m_BoxAlgorithmViz.RedrawTopLevelWindow();
    }

    m_X = x;
    m_Y = y;
}

void CMouse::MouseMotion(CRendererContext& ctx, const int x, const int y)
{
    if (m_Buttons[3]) { ctx.ScaleBy(powf(0.99F, float(y - m_Y))); }
    if (m_Buttons[2]) { ctx.ZoomBy(powf(0.99F, float(y - m_Y))); }
    if (m_Buttons[1]) {
        ctx.RotateByY(float(x - m_X) * 0.1F);
        ctx.RotateByX(float(y - m_Y) * 0.1F);
    }

    m_X = x;
    m_Y = y;
}

bool CMouse::HasButtonPressed() const
{
    for (const auto& button : m_Buttons) { if (button.second) { return true; } }
    return false;
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
