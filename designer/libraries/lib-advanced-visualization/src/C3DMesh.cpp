///-------------------------------------------------------------------------------------------------
/// 
/// \file C3DMesh.cpp
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

#if defined TARGET_HAS_ThirdPartyOpenGL

#include "C3DMesh.hpp"

#include <cstring>

namespace OpenViBE {
namespace AdvancedVisualization {

namespace {
template <typename T>
bool littleEndianToHost(const uint8_t* buffer, T* value)
{
    if (!buffer || !value) { return false; }
    memset(value, 0, sizeof(T));
    for (uint32_t i = 0; i < sizeof(T); ++i) { reinterpret_cast<uint8_t*>(value)[i] = buffer[i]; }
    return true;
}
}  // namespace


void C3DMesh::Clear()
{
    m_Color.fill(1.0);

    m_Vertices.clear();
    m_Normals.clear();
    m_Triangles.clear();
}

bool C3DMesh::Load(const void* buffer)
{
    const auto* tmp = reinterpret_cast<const uint32_t*>(buffer);

    uint32_t nVertex = 0, nTriangle = 0;

    littleEndianToHost<uint32_t>(reinterpret_cast<const uint8_t*>(&tmp[0]), &nVertex);
    littleEndianToHost<uint32_t>(reinterpret_cast<const uint8_t*>(&tmp[1]), &nTriangle);

    m_Vertices.resize(nVertex);
    m_Triangles.resize(size_t(nTriangle) * 3);

    size_t j = 2;

    for (size_t i = 0; i < nVertex; ++i) {
        littleEndianToHost<float>(reinterpret_cast<const uint8_t*>(&tmp[j++]), &m_Vertices[i].x);
        littleEndianToHost<float>(reinterpret_cast<const uint8_t*>(&tmp[j++]), &m_Vertices[i].y);
        littleEndianToHost<float>(reinterpret_cast<const uint8_t*>(&tmp[j++]), &m_Vertices[i].z);
    }

    for (size_t i = 0; i < size_t(nTriangle * 3); ++i) { littleEndianToHost<uint32_t>(reinterpret_cast<const uint8_t*>(&tmp[j++]), &m_Triangles[i]); }

    this->Compile();

    return true;
}

bool C3DMesh::Compile()
{
    m_Normals.clear();
    m_Normals.resize(m_Vertices.size());
    for (size_t i = 0; i < m_Triangles.size(); i += 3) {
        const uint32_t i1 = m_Triangles[i];
        const uint32_t i2 = m_Triangles[i + 1];
        const uint32_t i3 = m_Triangles[i + 2];
        CVertex v1        = m_Vertices[i1];
        CVertex v2        = m_Vertices[i2];
        CVertex v3        = m_Vertices[i3];
        v2.x -= v1.x;
        v2.y -= v1.y;
        v2.z -= v1.z;
        v3.x -= v1.x;
        v3.y -= v1.y;
        v3.z -= v1.z;
        v1 = CVertex::Cross(v2, v3);
        v1.Normalize();
        m_Normals[i1].x += v1.x;
        m_Normals[i1].y += v1.y;
        m_Normals[i1].z += v1.z;
        m_Normals[i2].x += v1.x;
        m_Normals[i2].y += v1.y;
        m_Normals[i2].z += v1.z;
        m_Normals[i3].x += v1.x;
        m_Normals[i3].y += v1.y;
        m_Normals[i3].z += v1.z;
    }

    for (auto& normal : m_Normals) { normal.Normalize(); }
    return true;
}

bool C3DMesh::Project(std::vector<CVertex>& out, const std::vector<CVertex>& in) const
{
    out.resize(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        CVertex p, q;
        p = in[i];
        // q = vChannelCoordinate[i];
        for (size_t j = 0; j < this->m_Triangles.size(); j += 3) {
            const uint32_t i1 = this->m_Triangles[j];
            const uint32_t i2 = this->m_Triangles[j + 1];
            const uint32_t i3 = this->m_Triangles[j + 2];

            CVertex v1, v2, v3;
            v1 = this->m_Vertices[i1];
            v2 = this->m_Vertices[i2];
            v3 = this->m_Vertices[i3];

            CVertex e1(v1, v2);
            CVertex e2(v1, v3);
            CVertex n = CVertex::Cross(e1, e2).Normalize();

            const float t = CVertex::Dot(v1, n) / CVertex::Dot(p, n);
            q.x           = t * p.x;
            q.y           = t * p.y;
            q.z           = t * p.z;

            if (CVertex::IsInTriangle(q, v1, v2, v3) && t >= 0) { out[i] = q; }
        }
        //  if (q.x == 0 && q.y == 0 && q.z == 0) { ::printf("Could not project coordinates on mesh for channel %i [%s]\n", i+1, ctx.getChannelName(i).c_str()); }
    }
    return true;
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyOpenGL
