///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererMultiLine.cpp
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

#include <cstdint>
#include "CRendererMultiLine.hpp"

#if defined TARGET_OS_Windows
#include <Windows.h>
#endif // TARGET_OS_Windows

#if defined TARGET_OS_MacOS
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace OpenViBE {
namespace AdvancedVisualization {

bool CRendererMultiLine::Render(const CRendererContext& ctx)
{
    if (!ctx.GetSelectedCount() || !m_nHistory) { return false; }

    const auto nSample = int(m_nSample);
    const auto n1      = int(m_historyIdx % m_nSample);
    const auto n2      = int(nSample - n1);

    if (!nSample) { return false; }

    const float t1 = float(n2) * 1.0F / float(nSample);
    const float t2 = -float(n1) * 1.0F / float(nSample);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(0, ctx.IsPositiveOnly() ? 0 : 0.5F, 0);
    glScalef(1, ctx.GetScale(), 1);
    glEnableClientState(GL_VERTEX_ARRAY);
    for (size_t i = 0; i < ctx.GetSelectedCount(); ++i) {
        std::vector<CVertex>& vertices = m_vertices[ctx.GetSelected(i)];
        glTexCoord1f(1 - (float(i) + 0.5F) / float(ctx.GetSelectedCount()));
        if (ctx.IsScrollModeActive()) {
            glPushMatrix();
            glTranslatef(t1, 0, 0);
            glVertexPointer(3, GL_FLOAT, sizeof(CVertex), &vertices[0].x);
            glDrawArrays(GL_LINE_STRIP, 0, n1);
            glPopMatrix();
            if (n2 > 0) {
                glPushMatrix();
                glTranslatef(t2, 0, 0);
                glVertexPointer(3, GL_FLOAT, sizeof(CVertex), &vertices[n1].x);
                glDrawArrays(GL_LINE_STRIP, 0, n2);
                glPopMatrix();

                if (n1 > 0) {
                    glBegin(GL_LINES);
                    glVertex2f(vertices[m_nSample - 1].x + t2, vertices[m_nSample - 1].y);
                    glVertex2f(vertices[0].x + t1, vertices[0].y);
                    glEnd();
                }
            }
        }
        else {
            glVertexPointer(3, GL_FLOAT, sizeof(CVertex), &vertices[0].x);
            glDrawArrays(GL_LINE_STRIP, 0, nSample);
        }
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_TEXTURE_1D);
    glColor3f(0.2F, 0.2F, 0.2F);
    glBegin(GL_LINES);
    glVertex2f(0, ctx.IsPositiveOnly() ? 0 : 0.5F);
    glVertex2f(1, ctx.IsPositiveOnly() ? 0 : 0.5F);
    glEnd();

    return true;
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
