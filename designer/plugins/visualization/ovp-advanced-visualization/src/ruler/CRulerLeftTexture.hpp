///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerLeftTexture.hpp
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

#include "CRulerTexture.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
class CRulerLeftTexture final : public CRulerTexture
{
public:
    void render() override

    {
        this->preRender();

        glColor4f(0, 0, 0, m_blackAlpha);
        glBegin(GL_QUADS);
        glTexCoord1f(0);
        glVertex2f(0.00F, 0);
        glVertex2f(0.05F, 0);
        glTexCoord1f(1);
        glVertex2f(0.05F, 1);
        glVertex2f(0.00F, 1);
        glEnd();

        glColor4f(1, 1, 1, m_whiteAlpha);
        glBegin(GL_QUADS);
        glTexCoord1f(0);
        glVertex2f(0.00F, 0);
        glVertex2f(0.04F, 0);
        glTexCoord1f(1);
        glVertex2f(0.04F, 1);
        glVertex2f(0.00F, 1);
        glEnd();

        this->postRender();
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
