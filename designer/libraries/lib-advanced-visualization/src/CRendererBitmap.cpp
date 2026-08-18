///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererBitmap.cpp
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

#include "CRendererBitmap.hpp"

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

void CRendererBitmap::Rebuild(const CRendererContext& ctx)
{
    IRenderer::Rebuild(ctx);

    m_autoDecimationFactor = 1 + size_t((m_nSample - 1) / CRendererContext::GetMaximumSampleCountPerDisplay());

    m_vertex.clear();
    m_vertex.resize(m_nChannel);
    for (size_t i = 0; i < m_nChannel; ++i) {
        m_vertex[i].resize((m_nSample / m_autoDecimationFactor) * 4);
        for (size_t j = 0; j < m_nSample - m_autoDecimationFactor + 1; j += m_autoDecimationFactor) {
            const size_t l     = j / m_autoDecimationFactor;
            const size_t id    = l * 4;
            const float factor = float(m_autoDecimationFactor) * m_nInverseSample;
            const float value  = float(l) * factor;
            m_vertex[i][id].x  = value;
            m_vertex[i][id].y  = 0;

            m_vertex[i][id + 1].x = value + factor;
            m_vertex[i][id + 1].y = 0;

            m_vertex[i][id + 2].x = value + factor;
            m_vertex[i][id + 2].y = 1;

            m_vertex[i][id + 3].x = value;
            m_vertex[i][id + 3].y = 1;
        }
    }

    m_historyIdx = 0;
}

void CRendererBitmap::Refresh(const CRendererContext& ctx)
{
    IRenderer::Refresh(ctx);

    if (!m_nHistory) { return; }
    if (m_vertex.empty()) { return; }

    for (size_t i = 0; i < m_nChannel; ++i) {
        size_t k                    = ((m_nHistory - 1) / m_nSample) * m_nSample;
        std::vector<float>& history = m_history[i];
        CVertex* vertex             = &m_vertex[i][0];
        for (size_t j = 0; j < m_nSample - m_autoDecimationFactor + 1; j += m_autoDecimationFactor, k += m_autoDecimationFactor) {
            if (k >= m_historyIdx && k < m_nHistory) {
                const float value = history[k];
                vertex++->u       = value;
                vertex++->u       = value;
                vertex++->u       = value;
                vertex++->u       = value;
            }
            else { vertex += 4; }
        }
    }
    m_historyIdx = m_nHistory;
}

bool CRendererBitmap::Render(const CRendererContext& ctx)
{
    if (!ctx.GetSelectedCount() || m_vertex.empty() || !m_nHistory) { return false; }

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glScalef(ctx.GetScale(), 1, 1);
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glScalef(1, 1.0F / float(ctx.GetSelectedCount()), 1);
    for (size_t i = 0; i < ctx.GetSelectedCount(); ++i) {
        glPushMatrix();
        glTranslatef(0, float(ctx.GetSelectedCount() - i) - 1.0F, 0);
        glVertexPointer(3, GL_FLOAT, sizeof(CVertex), &m_vertex[ctx.GetSelected(i)][0].x);
        glTexCoordPointer(1, GL_FLOAT, sizeof(CVertex), &m_vertex[ctx.GetSelected(i)][0].u);
        glDrawArrays(GL_QUADS, 0, GLsizei((m_nSample / m_autoDecimationFactor) * 4));
        glPopMatrix();
    }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    return true;
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
