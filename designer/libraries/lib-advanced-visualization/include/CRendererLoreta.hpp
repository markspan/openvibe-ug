///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererLoreta.hpp
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

#include "IRenderer.hpp"
#include "C3DMesh.hpp"
#include <vector>
#include <map>
#include <string>

namespace OpenViBE {
namespace AdvancedVisualization {
class CRendererLoreta final : public IRenderer
{
public:
    CRendererLoreta();

    void Rebuild(const CRendererContext& ctx) override { IRenderer::Rebuild(ctx); }
    void Refresh(const CRendererContext& ctx) override { IRenderer::Refresh(ctx); }
    bool Render(const CRendererContext& ctx) override;

    void ClearRegionSelection() override;
    size_t GetRegionCategoryCount() override { return m_lookups.size(); }
    size_t GetRegionCount(const size_t category) override;
    const char* GetRegionCategoryName(const size_t category) override;
    const char* GetRegionName(const size_t category, const size_t index) override;
    void SelectRegion(const size_t category, const char* name) override;
    void SelectRegion(const size_t category, const size_t index) override;

    void RefreshBrainSubset();

protected:
    std::vector<std::map<std::string, std::vector<size_t>>> m_lookups;
    std::vector<bool> m_selecteds;

    C3DMesh m_face, m_scalp, m_brain;

    std::vector<uint32_t> m_brainSubsetTriangles;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenGL
