///-------------------------------------------------------------------------------------------------
/// 
/// \file CMouse.hpp
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

#include <mensia/advanced-visualization.hpp>

#include <map>

namespace OpenViBE {
namespace AdvancedVisualization {
class CBoxAlgorithmViz;

class CMouse
{
public:
    explicit CMouse(CBoxAlgorithmViz& boxAlgorithmViz) : m_BoxAlgorithmViz(boxAlgorithmViz) { }
    void MouseButton(CRendererContext& ctx, const int x, const int y, const int button, const int status);
    void MouseMotion(CRendererContext& ctx, const int x, const int y);
    bool HasButtonPressed() const;


    CBoxAlgorithmViz& m_BoxAlgorithmViz;
    std::map<int, int> m_Buttons;
    int m_X = 0;
    int m_Y = 0;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
