///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerTexture.hpp
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

#include "../IRuler.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
class CRulerTexture : public IRuler
{
protected:
    virtual void preRender()

    {
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_1D);
        glColor3f(1, 1, 1);

        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
    }

    virtual void postRender()

    {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glMatrixMode(GL_TEXTURE);
        glPopMatrix();

        glMatrixMode(GL_MODELVIEW);
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
