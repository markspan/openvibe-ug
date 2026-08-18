///-------------------------------------------------------------------------------------------------
/// 
/// \file C3DMesh.hpp
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

#if defined TARGET_HAS_ThirdPartyOpenGL

#include <vector>
#include <array>
#include <cstdint>
#include <cstdlib>  // size_t for unix

#include "CVertex.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
class C3DMesh final
{
public:
    C3DMesh() { m_Color.fill(1.0); }
    //C3DMesh(const char* filename);
    ~C3DMesh() = default;

    void Clear();
    bool Load(const void* buffer);
    bool Compile();

    bool Project(std::vector<CVertex>& out, const std::vector<CVertex>& in) const;

    std::vector<CVertex> m_Vertices;
    std::vector<CVertex> m_Normals;
    std::vector<uint32_t> m_Triangles;
    std::array<float, 3> m_Color;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE


#endif // TARGET_HAS_ThirdPartyOpenGL
