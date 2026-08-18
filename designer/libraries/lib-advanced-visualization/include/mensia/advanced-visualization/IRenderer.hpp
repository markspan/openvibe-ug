///-------------------------------------------------------------------------------------------------
/// 
/// \file IRenderer.hpp
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

#include "defines.hpp"
#include "CRendererContext.hpp"
#include "CVertex.hpp"

#include <string>
#include <vector>

namespace OpenViBE {
namespace AdvancedVisualization {
enum class ERendererType
{
    Default, Topography2D, Topography3D, Bars, Bitmap, Connectivity, Cube, Flower, Line, Loreta, Mountain, MultiLine, Slice, XYZPlot, Last
};

class LMAV_API IRenderer
{
public:
    IRenderer();
    IRenderer(const IRenderer&) = delete;
    virtual ~IRenderer();

    static IRenderer* Create(const ERendererType type, const bool stimulation);
    static void Release(IRenderer* renderer) { delete renderer; }

    void SetChannelLocalisation(const char* filename) { m_channelPosFilename = filename; }
    void SetChannelCount(const size_t nChannel);
    void SetSampleCount(const size_t nSample);
    void SetHistoryDrawIndex(const size_t index);
    void Feed(const float* data);
    void Feed(const float* data, const size_t nSample);
    void Feed(const uint64_t stimDate, const uint64_t stimID) { m_stimulationHistory.emplace_back(double(stimDate >> 16) / 65536.0, stimID); }
    void Prefeed(const size_t nPreFeedSample);

    float GetSuggestedScale() const;

    void Clear(const size_t nSampleToKeep = 0);

    size_t GetChannelCount() const { return m_nChannel; }
    size_t GetSampleCount() const { return m_nSample; }
    size_t GetHistoryCount() const { return m_nHistory; }
    size_t GetHistoryIndex() const { return m_historyIdx; }
    bool GetSampleAtERPFraction(const float erpFraction, std::vector<float>& samples) const;

    void SetTimeOffset(const uint64_t offset) { m_timeOffset = offset; }
    uint64_t GetTimeOffset() const { return m_timeOffset; }

    static void Draw2DCoordinateSystem();
    static void Draw3DCoordinateSystem();
    void DrawCoordinateSystem() const { this->Draw3DCoordinateSystem(); }

    virtual void Rebuild(const CRendererContext& /*ctx*/) { }
    virtual void Refresh(const CRendererContext& ctx);
    virtual bool Render(const CRendererContext& ctx) = 0;

    virtual void ClearRegionSelection() { }
    virtual size_t GetRegionCategoryCount() { return 0; }
    virtual size_t GetRegionCount(const size_t /*category*/) { return 0; }
    virtual const char* GetRegionCategoryName(const size_t /*category*/) { return nullptr; }
    virtual const char* GetRegionName(const size_t /*category*/, const size_t /*index*/) { return nullptr; }
    virtual void SelectRegion(const size_t /*category*/, const char* /*name*/) { }
    virtual void SelectRegion(const size_t /*category*/, const size_t /*index*/) { }


protected:
    std::string m_channelPosFilename;
    size_t m_historyIdx     = 0;
    size_t m_historyDrawIdx = 0;
    size_t m_nHistory       = 0;
    size_t m_nChannel       = 0;
    size_t m_nSample        = 1;

    float m_nInverseChannel       = 1.0;
    float m_nInverseSample        = 1.0;
    size_t m_autoDecimationFactor = 1;

    float m_erpFraction     = 0.0;
    size_t m_sampleIndexERP = 0;

    uint64_t m_timeOffset = 0;

    // std::map < std::string, CVertex > m_channelPos;
    std::vector<std::pair<double, uint64_t>> m_stimulationHistory;
    std::vector<std::vector<float>> m_history;
    std::vector<std::vector<CVertex>> m_vertex;
    std::vector<size_t> m_mesh;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
