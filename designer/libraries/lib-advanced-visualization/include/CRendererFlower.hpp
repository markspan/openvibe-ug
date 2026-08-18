///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererFlower.hpp
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

#include "IRenderer.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
class CRendererFlower final : public IRenderer
{
public:
    explicit CRendererFlower(size_t multiCount = 1);

    void Rebuild(const CRendererContext& ctx) override;
    void Refresh(const CRendererContext& ctx) override;
    bool Render(const CRendererContext& ctx) override;

protected:
    std::vector<std::vector<std::vector<CVertex>>> m_muliVertices;
    std::vector<CVertex> m_circles;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
