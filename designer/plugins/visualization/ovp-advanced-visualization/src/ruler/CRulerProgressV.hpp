///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerProgressV.hpp
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

#include "CRulerProgress.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
class CRulerProgressV final : public CRulerProgress
{
public:
    void RenderFinal(const float progress) override
    {
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_BLEND);

        glLineWidth(4);
        glColor3f(0, 0, 0);
        glBegin(GL_LINES);
        glVertex2f(progress, 0);
        glVertex2f(progress, 1);
        glEnd();

        glLineWidth(2);
        glColor3f(0.25, 1, 0.25);
        glBegin(GL_LINES);
        glVertex2f(progress, 0);
        glVertex2f(progress, 1);
        glEnd();
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
