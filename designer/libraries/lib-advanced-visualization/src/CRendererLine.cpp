///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererLine.cpp
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
#include "CRendererLine.hpp"

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

void CRendererLine::Rebuild(const CRendererContext& ctx)
{
    IRenderer::Rebuild(ctx);

    m_vertices.clear();
    m_vertices.resize(m_nChannel);

    for (size_t i = 0; i < m_nChannel; ++i) {
        m_vertices[i].resize(m_nSample);
        for (size_t j = 0; j < m_nSample; ++j) { m_vertices[i][j].x = float(j) * m_nInverseSample; }
    }

    m_historyIdx = 0;
}

void CRendererLine::Refresh(const CRendererContext& ctx)
{
    IRenderer::Refresh(ctx);

    if (!m_nHistory) { return; }

    size_t maxIdx;

    if (m_historyDrawIdx == 0) { maxIdx = m_nHistory; } // Draw real-time 
    else { maxIdx = m_historyDrawIdx; }                 // stay at the m_historyDrawIdx

    for (size_t i = 0; i < m_nChannel; ++i) {
        const size_t firstIdx       = ((maxIdx - 1) / m_nSample) * m_nSample;
        std::vector<float>& history = m_history[i];
        CVertex* vertex             = &m_vertices[i][0];

        for (size_t j = 0; j < m_nSample; ++j) {
            const size_t idx = firstIdx + j;

            if (idx < maxIdx) { vertex->y = history[idx]; }
            else if (idx >= m_nSample) { vertex->y = history[idx - m_nSample]; }

            vertex++;
        }
    }

    m_historyIdx = maxIdx;
}

bool CRendererLine::Render(const CRendererContext& ctx)
{
    if (!ctx.GetSelectedCount() || !m_nHistory) { return false; }

    const auto nSample = int(m_nSample);


    // When the display is in continuous mode, there will be n1 samples
    // displayed until the 'cursor' and n2 samples will be displayed to
    // complete the whole window.
    // For example at 25s with a display size of 20s
    // <- n1 samples -><------------ n2 samples ---------------->
    // | ____         |                    ___                  |
    // |/    \        |      ___          /   \     ____        |
    // |------\------/|-----/---\--------/-----\___/----\-------|
    // |       \____/ |    /     \______/                \______|
    // |              |___/                                     |
    // Time          25s              10s                      20s

    const auto n1 = int(m_historyIdx % m_nSample);
    const auto n2 = int(nSample - n1);

    if (!nSample) { return false; }

    const float t1 = float(n2) * 1.0F / float(nSample);
    const float t2 = -float(n1) * 1.0F / float(nSample);

    glDisable(GL_TEXTURE_1D);

    glPushMatrix();
    glScalef(1, 1.0F / float(ctx.GetSelectedCount()), 1);
    glTranslatef(0, ctx.IsPositiveOnly() ? 0 : 0.5F, 0);

    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0.2F, 0.2F, 0.2F);
    glBegin(GL_LINES);
    for (size_t i = 0; i < ctx.GetSelectedCount(); ++i) {
        glVertex2f(0, float(i));
        glVertex2f(1, float(i));
    }
    glEnd();
    glPopAttrib();

    glEnableClientState(GL_VERTEX_ARRAY);
    for (size_t i = 0; i < ctx.GetSelectedCount(); ++i) {
        glPushMatrix();
        glTranslatef(0, float(ctx.GetSelectedCount() - i) - 1.0F, 0);
        glScalef(1, ctx.GetScale(), 1);

        std::vector<CVertex>& vertices = m_vertices[ctx.GetSelected(i)];
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
                    glVertex2f(vertices[nSample - 1].x + t2, vertices[nSample - 1].y);
                    glVertex2f(vertices[0].x + t1, vertices[0].y);
                    glEnd();
                }
            }
        }
        else {
            glVertexPointer(3, GL_FLOAT, sizeof(CVertex), &vertices[0].x);
            glDrawArrays(GL_LINE_STRIP, 0, nSample);
        }
        glPopMatrix();
    }
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();

    return true;
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
