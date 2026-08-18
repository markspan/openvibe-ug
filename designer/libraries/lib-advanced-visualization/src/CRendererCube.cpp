///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererCube.cpp
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

#include "CRendererCube.hpp"
#include "RendererTools.hpp"

#include <cmath>

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

CRendererCube::CRendererCube() = default;

void CRendererCube::Rebuild(const CRendererContext& ctx)
{
    IRenderer::Rebuild(ctx);

    m_vertices.clear();
    m_vertices.resize(ctx.GetChannelCount());

    m_historyIdx = 0;
}

void CRendererCube::Refresh(const CRendererContext& ctx)
{
    IRenderer::Refresh(ctx);

    if (!m_nHistory) { return; }

    const float indexERP   = (m_erpFraction * float(m_nSample - 1));
    const float alpha      = indexERP - std::floor(indexERP);
    const size_t indexERP1 = size_t(indexERP) % m_nSample;
    const size_t indexERP2 = size_t(indexERP + 1) % m_nSample;

    for (size_t i = 0; i < m_vertices.size(); ++i) {
        m_vertices[i].u = m_history[i][m_nHistory - m_nSample + indexERP1] * (1 - alpha) + m_history[i][m_nHistory - m_nSample + indexERP2] * (alpha);
    }

    m_historyIdx = m_nHistory;
}

bool CRendererCube::Render(const CRendererContext& ctx)
{
    if (!ctx.GetSelectedCount() || m_vertices.empty() || !m_nHistory) { return false; }

    const float d = 3.5;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    perspectiveGL(60, double(ctx.GetAspect()), .01, 100);
    glTranslatef(0, 0, -d);
    glRotatef(ctx.GetRotationX() * 10, 1, 0, 0);
    glRotatef(ctx.GetRotationY() * 10, 0, 1, 0);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glScalef(ctx.GetScale(), 1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glScalef(ctx.GetZoom(), ctx.GetZoom(), ctx.GetZoom());

    glPushMatrix();
    glRotatef(19, 1, 0, 0);
    for (size_t j = 0; j < ctx.GetSelectedCount(); ++j) {
        CVertex v;
        const size_t k = ctx.GetSelected(j);
        ctx.GetChannelLocalisation(k, v.x, v.y, v.z);
        const float scale = 0.1F * (0.25F + fabs(m_vertices[k].u * ctx.GetScale()));

        glPushMatrix();
        glTranslatef(v.x, v.y, v.z);
        glTexCoord1f(m_vertices[k].u);
        glScalef(scale, scale, scale);

        glColor3f(1, 1, 1);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        cube();

        glColor3f(0, 0, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        cube();

        glPopMatrix();
    }
    glPopMatrix();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (ctx.GetCheckBoardVisibility()) { this->DrawCoordinateSystem(); }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    return true;
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
