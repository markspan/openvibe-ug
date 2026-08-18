///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererTopo.cpp
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

#include "CRendererTopo.hpp"
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

const bool MULTI_SLICE = false;

namespace {
const size_t S = 1000;

// Legendre polynomials
// http://en.wikipedia.org/wiki/Legendre_polynomials

void legendre(const size_t n, const double x, std::vector<double>& legendres)
{
    legendres.resize(n + 1);
    legendres[0] = 1;
    legendres[1] = x;
    for (size_t i = 2; i <= n; ++i) {
        const double invi = 1.0 / double(i);
        legendres[i]      = (2 - invi) * x * legendres[i - 1] - (1 - invi) * legendres[i - 2];
    }
}

// G function :
// Spherical splines for scalp potential and current density mapping
// http://www.sciencedirect.com/science/article/pii/0013469489901806

double g(const size_t n, const size_t m, const std::vector<double>& legendres)
{
    double result = 0;
    for (size_t i = 1; i <= n; ++i) { result += double(2 * i + 1) / pow(double(i * (i + 1)), int(m)) * legendres[i]; }
    return result / (4 * M_PI);
}

// H function :
// Spherical splines for scalp potential and current density mapping
// http://www.sciencedirect.com/science/article/pii/0013469489901806

double h(const size_t n, const size_t m, const std::vector<double>& legendres)
{
    double result = 0;
    for (size_t i = 1; i <= n; ++i) { result += double(2 * i + 1) / pow(double(i * (i + 1)), int(m - 1)) * legendres[i]; }
    return result / (4 * M_PI);
}

// Caching system

void build(const size_t n, const size_t m, std::vector<double>& gCache, std::vector<double>& hCache)
{
    gCache.resize(2 * S + 1);
    hCache.resize(2 * S + 1);
    for (size_t i = 0; i <= 2 * S; ++i) {
        std::vector<double> legendres;
        const double cosine = (double(i) - S) / S;

        legendre(n, cosine, legendres);
        gCache[i] = g(n, m, legendres);
        hCache[i] = h(n, m, legendres);
    }
    gCache.push_back(gCache.back());
    hCache.push_back(hCache.back());
}

double cache(const double x, const std::vector<double>& rCache)
{
    if (x < -1) { return rCache[0]; }
    if (x > 1) { return rCache[2 * S]; }
    double t     = (x + 1) * S;
    const int i1 = int(t);
    const int i2 = int(t + 1);
    t -= i1;
    return rCache[i1] * (1 - t) + rCache[i2] * t;
}
}  // namespace

void CRendererTopo::Rebuild(const CRendererContext& ctx)
{
    IRenderer::Rebuild(ctx);

    this->Rebuild3DMeshesPre(ctx);

    // Projects electrode coordinates to 3D mesh

    std::vector<CVertex> projectedPositions;
    std::vector<CVertex> positions;
    positions.resize(ctx.GetChannelCount());
    for (size_t i = 0; i < ctx.GetChannelCount(); ++i) { ctx.GetChannelLocalisation(i, positions[i].x, positions[i].y, positions[i].z); }
    m_scalp.Project(projectedPositions, positions);
    m_projectedPositions = projectedPositions;

#if 0

    m_projectedPositions.resize(ctx.getChannelCount());
    for (size_t i = 0; i < ctx.getChannelCount(); ++i)
    {
        CVertex p, q;
        ctx.getChannelLocalisation(i, p.x, p.y, p.z);
        for (size_t j = 0; j < m_scalp.m_Triangles.size(); j += 3)
        {
            size_t i1, i2, i3;
            i1 = m_scalp.m_Triangles[j];
            i2 = m_scalp.m_Triangles[j + 1];
            i3 = m_scalp.m_Triangles[j + 2];

            CVertex v1, v2, v3;
            v1 = m_scalp.m_vertex[i1];
            v2 = m_scalp.m_vertex[i2];
            v3 = m_scalp.m_vertex[i3];

            CVertex e1(v1, v2);
            CVertex e2(v1, v3);
            CVertex n = CVertex::cross(e1, e2).normalize();

            float t = CVertex::dot(v1, n) / CVertex::dot(p, n);
            q.x = t * p.x;
            q.y = t * p.y;
            q.z = t * p.z;

            if (CVertex::isInTriangle(q, v1, v2, v3) && t >= 0)
            {
                m_projectedPositions[i].x = q.x;
                m_projectedPositions[i].y = q.y;
                m_projectedPositions[i].z = q.z;
            }
        }
        if (m_projectedPositions[i].x == 0 && m_projectedPositions[i].y == 0 && m_projectedPositions[i].z == 0)
        {
            // ::printf("Could not project coordinates on mesh for channel %i [%s]\n", i+1, rContext.getChannelName(i).c_str());
        }
    }

#endif

    // Generates transformation matrices based spherical spline interpolations

    const size_t m = 3;
    const auto n   = size_t(pow(10., 10. / (2 * m - 2)));

    std::vector<double> gCaches;
    std::vector<double> hCaches;
    build(n, m, gCaches, hCaches);

    const Eigen::Index nc = Eigen::Index(ctx.GetChannelCount());
    const Eigen::Index vc = Eigen::Index(m_scalp.m_Vertices.size());

    A         = Eigen::MatrixXd(nc + 1, nc + 1);
    A(nc, nc) = 0;
    for (Eigen::Index i = 0; i < nc; ++i) {
        A(i, nc) = 1;
        A(nc, i) = 1;
        for (Eigen::Index j = 0; j <= i; ++j) {
            CVertex v1, v2;
            ctx.GetChannelLocalisation(i, v1.x, v1.y, v1.z);
            ctx.GetChannelLocalisation(j, v2.x, v2.y, v2.z);

            const double cosine = double(CVertex::Dot(v1, v2));
            A(i, j)             = cache(cosine, gCaches);
            A(j, i)             = cache(cosine, gCaches);
        }
    }

    B         = Eigen::MatrixXd(vc + 1, nc + 1);
    D         = Eigen::MatrixXd(vc + 1, nc + 1);
    B(vc, nc) = 0;
    D(vc, nc) = 0;
    for (Eigen::Index i = 0; i < vc; ++i) {
        B(i, nc) = 1;
        D(i, nc) = 1;
        for (Eigen::Index j = 0; j < nc; ++j) {
            B(vc, j) = 1;
            D(vc, j) = 1;
            CVertex v1, v2;
            v1 = m_scalp.m_Vertices[i];
            v1.Normalize();
            ctx.GetChannelLocalisation(j, v2.x, v2.y, v2.z);

            const double cosine = double(CVertex::Dot(v1, v2));
            B(i, j)             = cache(cosine, gCaches);
            D(i, j)             = cache(cosine, hCaches);
        }
    }

    Ai = A.inverse();

    // Post processed 3D meshes when needed

    this->Rebuild3DMeshesPost(ctx);

    // Rebuilds texture coordinates array

    if (MULTI_SLICE) {
        m_interpolatedSamples.clear();
        m_interpolatedSamples.resize(m_nSample, Eigen::VectorXd::Zero(Eigen::Index(m_scalp.m_Vertices.size())));
    }

    // Finalizes

    m_historyIdx = 0;
}

// V has sensor potentials
// W has interpolated potentials
// Z has interpolated current densities
void CRendererTopo::interpolate(const Eigen::VectorXd& v, Eigen::VectorXd& w, Eigen::VectorXd& z) const
{
    Eigen::VectorXd c = Ai * v;
    w                 = B * c;
    c[v.size() - 1]   = 0;
    z                 = D * c;
}

void CRendererTopo::Refresh(const CRendererContext& ctx)
{
    IRenderer::Refresh(ctx);

    if (!m_nHistory) { return; }

    const Eigen::Index nc = Eigen::Index(ctx.GetChannelCount());
    const Eigen::Index vc = Eigen::Index(m_scalp.m_Vertices.size());

    Eigen::VectorXd v = Eigen::VectorXd::Zero(nc + 1);
    Eigen::VectorXd w;
    Eigen::VectorXd z;

    if (!MULTI_SLICE) {
        std::vector<float> samples;
        this->GetSampleAtERPFraction(m_erpFraction, samples);
        for (Eigen::Index i = 0; i < nc; ++i) { v(i) = double(samples[i]); }
        this->interpolate(v, w, z);
        for (Eigen::Index j = 0; j < vc; ++j) { m_scalp.m_Vertices[j].u = float(w(j)); }
    }
    else {
        if (m_nHistory >= m_nSample) {
            for (size_t k = 0; k < m_nSample; ++k) {
                for (Eigen::Index i = 0; i < nc; ++i) { v(i) = double(m_history[i][m_nHistory - m_nSample + k]); }
                this->interpolate(v, w, z);
                m_interpolatedSamples[k] = w;
            }
        }
    }

    m_historyIdx = m_nHistory;
}

bool CRendererTopo::Render(const CRendererContext& ctx)
{
    if (!ctx.GetSelectedCount() || m_scalp.m_Vertices.empty() || !m_nHistory) { return false; }

    const float d = 3.5;

    // ::glEnable(GL_DEPTH_TEST);
    // ::glDisable(GL_BLEND);

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

    // Now renders

    glPushMatrix();
#if 1
    glTranslatef(0, 0.5F, 0);
    glRotatef(19, 1, 0, 0);
    glTranslatef(0, -.2F, 0.35F);
    // ::glScalef(1.8f, 1.8f, 1.8f);
#else
    ::glRotatef(19, 1, 0, 0);
    ::glTranslatef(0, -.2f, .35f);
    // ::glScalef(1.8f, 1.8f, 1.8f);
#endif

    if (ctx.IsFaceMeshVisible()) {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_1D);
        if (!m_face.m_Triangles.empty()) {
            if (!m_face.m_Normals.empty()) {
                glEnable(GL_LIGHTING);
                glEnableClientState(GL_NORMAL_ARRAY);
            }
            glColor3f(m_face.m_Color[0], m_face.m_Color[1], m_face.m_Color[2]);
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, sizeof(CVertex), &m_face.m_Vertices[0].x);
            if (!m_face.m_Normals.empty()) { glNormalPointer(GL_FLOAT, sizeof(CVertex), &m_face.m_Normals[0].x); }
            glDrawElements(GL_TRIANGLES, GLsizei(m_face.m_Triangles.size()), GL_UNSIGNED_INT, &m_face.m_Triangles[0]);
            glDisableClientState(GL_NORMAL_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisable(GL_LIGHTING);
        }
    }

    if (ctx.IsScalpMeshVisible()) {
        glEnable(GL_TEXTURE_1D);
        if (!m_scalp.m_Triangles.empty()) {
            if (!m_scalp.m_Normals.empty()) {
                glEnable(GL_LIGHTING);
                glEnableClientState(GL_NORMAL_ARRAY);
            }
            glColor3f(m_scalp.m_Color[0], m_scalp.m_Color[1], m_scalp.m_Color[2]);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glVertexPointer(3, GL_FLOAT, sizeof(CVertex), &m_scalp.m_Vertices[0].x);
            if (!m_scalp.m_Normals.empty()) { glNormalPointer(GL_FLOAT, sizeof(CVertex), &m_scalp.m_Normals[0].x); }

            if (!MULTI_SLICE) {
                glColor3f(1, 1, 1);
                glEnable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);
                glTexCoordPointer(1, GL_FLOAT, sizeof(CVertex), &m_scalp.m_Vertices[0].u);
                glDrawElements(GL_TRIANGLES, GLsizei(m_scalp.m_Triangles.size()), GL_UNSIGNED_INT, &m_scalp.m_Triangles[0]);
            }
            else {
                glColor4f(1.0F, 1.0F, 1.0F, 4.0F / float(m_nSample));
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                for (size_t i = 0; i < m_nSample; ++i) {
                    float scale = 1.0F + float(i) * 0.25F / float(m_nSample);
                    glPushMatrix();
                    glScalef(scale, scale, scale);
                    glTexCoordPointer(1, GL_DOUBLE, 0, &m_interpolatedSamples[i][0]);
                    glDrawElements(GL_TRIANGLES, GLsizei(m_scalp.m_Triangles.size()), GL_UNSIGNED_INT, &m_scalp.m_Triangles[0]);
                    glPopMatrix();
                }
            }
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_NORMAL_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisable(GL_LIGHTING);
        }
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_1D);

    glLineWidth(3);
    for (size_t j = 0; j < ctx.GetChannelCount(); ++j) {
        const float scale = 0.025F;
        const CVertex v   = m_projectedPositions[j];
        //ctx.getChannelLocalisation(j, v.x, v.y, v.z);

        glPushMatrix();
        glTranslatef(v.x, v.y, v.z);
        glScalef(scale, scale, scale);

        const float value                = ctx.IsSelected(j) ? 1.0F : 0.2F;
        const std::array<float, 3> color = { value, value, value };
        glColor3fv(color.data());
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
