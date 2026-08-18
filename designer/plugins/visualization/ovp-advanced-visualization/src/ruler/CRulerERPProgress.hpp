///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerERPProgress.hpp
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
class CRulerERPProgress final : public IRuler
{
public:
    void render() override
    {
        if (m_renderer == nullptr) { return; }
        if (m_renderer->GetSampleCount() == 0) { return; }
        if (m_renderer->GetHistoryCount() == 0) { return; }
        if (m_renderer->GetHistoryIndex() == 0) { return; }

        const float progress = m_rendererCtx->GetERPFraction();
        if (std::fabs(progress) > FLT_EPSILON && std::fabs(progress - 1) > FLT_EPSILON) {
            glDisable(GL_TEXTURE_1D);

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
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
