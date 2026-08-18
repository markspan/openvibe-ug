///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererConnectivity.cpp
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

#include "CRendererConnectivity.hpp"
#include "C3DMesh.hpp"
#include "RendererTools.hpp"

// #include "content/Face.obj.hpp"
#include "content/Scalp.obj.hpp"

#include <Eigen/Geometry>

#if defined TARGET_OS_Windows
#include <Windows.h>
#endif // TARGET_OS_Windows

#if defined TARGET_OS_MacOS
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

//constexpr size_t COUNT = 16; //Macro modernization, Not yet with jenkins (not the last visual 2013 which it works)
#define COUNT 16

namespace OpenViBE {
namespace AdvancedVisualization {

static void q_rotate(Eigen::VectorXd& dst, const Eigen::VectorXd& src, const Eigen::Quaterniond& q) { dst = q.matrix() * src; }

static void q_from_polar(Eigen::Quaterniond& q, Eigen::VectorXd& v1, Eigen::VectorXd& v2, const CVertex& cv1, const CVertex& cv2)
{
    v1(0) = double(cv1.x);
    v1(1) = double(cv1.y);
    v1(2) = double(cv1.z);

    v2(0) = double(cv2.x);
    v2(1) = double(cv2.y);
    v2(2) = double(cv2.z);

    q.setFromTwoVectors(v1, v2);
}

void CRendererConnectivity::Rebuild(const CRendererContext& ctx)
{
    IRenderer::Rebuild(ctx);


    Eigen::Quaterniond q         = Eigen::Quaterniond::Identity();
    const Eigen::Quaterniond qId = Eigen::Quaterniond::Identity();
    Eigen::Quaterniond qDiff     = Eigen::Quaterniond::Identity();
    Eigen::VectorXd v(3);
    Eigen::VectorXd v1(3);
    Eigen::VectorXd v2(3);

    C3DMesh scalp;
    scalp.Load(SCALP_DATA.data());

    // Projects electrode coordinates to 3D mesh

    std::vector<CVertex> projectedChannelPos;
    std::vector<CVertex> channelPos;
    channelPos.resize(ctx.GetChannelCount());
    for (size_t i = 0; i < ctx.GetChannelCount(); ++i) { ctx.GetChannelLocalisation(i, channelPos[i].x, channelPos[i].y, channelPos[i].z); }
    scalp.Project(projectedChannelPos, channelPos);

    // Generates arcs

    m_vertex.clear();
    m_vertex.resize(m_nChannel * (m_nChannel - 1) / 2);
    size_t l = 0;
    for (size_t i = 0; i < m_nChannel; ++i) {
        for (size_t j = 0; j < i; ++j) {
            m_vertex[l].resize(COUNT);

            CVertex vi, vj;
            vi = channelPos[i];
            vj = channelPos[j];

            const double viLen = double(projectedChannelPos[i].Length());
            const double vjLen = double(projectedChannelPos[j].Length());

            q_from_polar(qDiff, v1, v2, vi, vj);

            const double dot = double(1 - CVertex::Dot(vi, vj)) * 0.5;

            for (size_t k = 0; k < COUNT; ++k) {
                const double t = double(k) * 1.0 / (COUNT - 1);
                auto s         = (t - 0.5) * 2;
                s              = 1 + 0.5 * (1 - s * s) * dot;

                q = qId.slerp(t, qDiff);

                q_rotate(v, v1, q);

                const double len = (viLen * (1 - t) + vjLen * t);
                m_vertex[l][k].x = float(s * v[0] * len);
                m_vertex[l][k].y = float(s * v[1] * len);
                m_vertex[l][k].z = float(s * v[2] * len);
                m_vertex[l][k].u = 0.0F;
            }

            l++;
        }
    }

    m_historyIdx = 0;
}

void CRendererConnectivity::Refresh(const CRendererContext& ctx)
{
    IRenderer::Refresh(ctx);

    if (!m_nHistory) { return; }
    if (m_nHistory < m_nChannel) { return; }

    size_t l = 0;
    for (size_t i = 0; i < m_nChannel; ++i) {
        for (size_t j = 0; j < i; ++j) {
            for (size_t k = 0; k < COUNT; ++k) { m_vertex[l][k].u = m_history[i][m_nHistory - 1 - j]; }
            l++;
        }
    }

    m_historyIdx = m_nHistory;
}

bool CRendererConnectivity::Render(const CRendererContext& ctx)
{
    if (!ctx.GetSelectedCount() || m_vertex.empty() || !m_nHistory) { return false; }

    const float d = 3.5;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

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

    const float rgb = 1.0F;
    glColor4f(rgb, rgb, rgb, ctx.GetTranslucency());
    glPushMatrix();

    glTranslatef(0, 0.5F, 0);
    glRotatef(19, 1, 0, 0);
    glTranslatef(0, -.2F, 0.35F);
    // ::glScalef(1.8f, 1.8f, 1.8f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    for (size_t i = 0; i < m_nChannel * (m_nChannel - 1) / 2; ++i) {
        glVertexPointer(3, GL_FLOAT, sizeof(CVertex), &m_vertex[i][0].x);
        glTexCoordPointer(1, GL_FLOAT, sizeof(CVertex), &m_vertex[i][0].u);
        glDrawArrays(GL_LINE_STRIP, 0, COUNT);
    }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();

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
