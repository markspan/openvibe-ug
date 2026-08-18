///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererTopo2D.cpp
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

#include "CRendererTopo2D.hpp"
#include <cmath>

namespace OpenViBE {
namespace AdvancedVisualization {

// constexpr constexpr float OFFSET = 0.0001f; //Macro modernization, Not yet with jenkins (not the last visual 2013 which it works)
#define OFFSET 0.0001f

void CRendererTopo2D::Rebuild3DMeshesPre(const CRendererContext& /*rContext*/)
{
    {
        const size_t nVertex1 = 32;
        const size_t nVertex2 = 32;
        // Scalp
        m_scalp.Clear();

        std::vector<CVertex>& vertices   = m_scalp.m_Vertices;
        std::vector<uint32_t>& triangles = m_scalp.m_Triangles;

        vertices.resize(nVertex1 * nVertex2);
        for (size_t i = 0, k = 0; i < nVertex1; ++i) {
            for (size_t j = 0; j < nVertex2; j++, k++) {
                const auto a  = float(double(i) * M_PI / (nVertex1 - 1));
                const auto b  = float(double(j) * 1.25 * M_PI / (nVertex2 - 1) - M_PI * .2);
                vertices[k].x = cosf(a);
                vertices[k].y = sinf(a) * sinf(b) - OFFSET;
                vertices[k].z = sinf(a) * cosf(b);
                vertices[k].u = float(k) * 200.0F / (nVertex1 * nVertex2);
            }
        }

        triangles.resize((nVertex1 - 1) * (nVertex2 - 1) * 6);
        for (size_t i = 0, k = 0; i < nVertex1 - 1; ++i) {
            for (size_t j = 0; j < nVertex2 - 1; j++, k += 6) {
                triangles[k]     = uint32_t((i) * nVertex2 + (j));
                triangles[k + 1] = uint32_t((i + 1) * nVertex2 + (j));
                triangles[k + 2] = uint32_t((i + 1) * nVertex2 + (j + 1));

                triangles[k + 3] = triangles[k];
                triangles[k + 4] = triangles[k + 2];
                triangles[k + 5] = uint32_t((i) * nVertex2 + (j + 1));
            }
        }

        m_scalp.m_Color.fill(1.0);
        // m_scalp.compile();
    }

    {
        const size_t nCircleVertex = 128;
        // Face
        m_face.Clear();

        std::vector<CVertex>& vertices   = m_face.m_Vertices;
        std::vector<uint32_t>& triangles = m_face.m_Triangles;

        // Ribbon mesh

        vertices.resize(nCircleVertex * 2/*+6*/);
        for (size_t i = 0, k = 0; i < nCircleVertex; ++i, ++k) {
            const auto a = float(double(i * 4) * M_PI / nCircleVertex);

            vertices[k].x = cosf(a);
            vertices[k].y = 0.01F;
            vertices[k].z = sinf(a);
            k++;
            vertices[k].x = cosf(a);
            vertices[k].y = -.01F;
            vertices[k].z = sinf(a);
        }

        // Nose mesh
        /*
        vertices[k].x=-1;   vertices[k].y=.01;  vertices[k].z=-.5;
        k++;
        vertices[k].x=-1;   vertices[k].y=-.01; vertices[k].z=-.5;
        k++;
        vertices[k].x=0;    vertices[k].y=.01;  vertices[k].z=-1.5;
        k++;
        vertices[k].x=0;    vertices[k].y=-.01; vertices[k].z=-1.5;
        k++;
        vertices[k].x=1;    vertices[k].y=.01;  vertices[k].z=-.5;
        k++;
        vertices[k].x=1;    vertices[k].y=-.01; vertices[k].z=-.5;
        */
        // Ribon mesh

        triangles.resize(nCircleVertex * 6/*+12*/);
        const size_t mod = nCircleVertex * 2;
        for (size_t i = 0, k = 0; i < nCircleVertex; ++i) {
            triangles[k++] = (i) % mod;
            triangles[k++] = (i + 1) % mod;
            triangles[k++] = (i + 2) % mod;

            triangles[k++] = (i + 1) % mod;
            triangles[k++] = (i + 2) % mod;
            triangles[k++] = (i + 3) % mod;
        }

        // Nose mesh
        /*
        triangles[k++] = mod;       triangles[k++] = mod + 1;   triangles[k++] = mod + 2;
        triangles[k++] = mod + 1;   triangles[k++] = mod + 2;   triangles[k++] = mod + 3;
        triangles[k++] = mod + 2;   triangles[k++] = mod + 3;   triangles[k++] = mod + 4;
        triangles[k++] = mod + 3;   triangles[k++] = mod + 4;   triangles[k++] = mod + 5;
        */
        m_face.m_Color.fill(1.15F);
        // m_face.compile();
    }
}

namespace {
void unfold(std::vector<CVertex>& vertices, const float layer = 0)
{
    for (auto& v : vertices) {
        v.y += OFFSET;
        const float phi = float(M_PI) * 0.5F - asinf(v.y);
        const float psi = atan2f(v.z, v.x);

        v.x = phi * cos(psi);
        v.y = layer;
        v.z = phi * sin(psi);
    }
}
}  // namespace

void CRendererTopo2D::Rebuild3DMeshesPost(const CRendererContext& /*ctx*/)
{
    const float layer = 1E-3F;

    unfold(m_scalp.m_Vertices, -layer);
    unfold(m_face.m_Vertices, layer);
    unfold(m_projectedPositions);
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
