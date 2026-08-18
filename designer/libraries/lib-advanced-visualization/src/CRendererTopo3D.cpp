///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererTopo3D.cpp
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

#include "CRendererTopo3D.hpp"

#include "content/Face.obj.hpp"
#include "content/Scalp.obj.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {

void CRendererTopo3D::Rebuild3DMeshesPre(const CRendererContext& /*ctx*/)
{
    m_face.Clear();
    m_scalp.Clear();

    //m_face.load(OpenViBE::Directories::getDataDir() + "/content/Face.obj");
    //m_scalp.load(OpenViBE::Directories::getDataDir() + "/content/Scalp.obj");
    m_face.Load(FACE_DATA.data());
    m_scalp.Load(SCALP_DATA.data());

    m_face.m_Color[0] = 0.8F;
    m_face.m_Color[1] = 0.6F;
    m_face.m_Color[2] = 0.5F;
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
