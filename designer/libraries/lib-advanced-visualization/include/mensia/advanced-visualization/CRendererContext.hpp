///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererContext.hpp
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

#include <cstdint>
#include <string>
#include <cstdlib>  // size_t for unix
#include <vector>
#include "CVertex.hpp"
#include <map>

namespace OpenViBE {
namespace AdvancedVisualization {
class LMAV_API CRendererContext
{
public:
    enum class EDataType { Matrix, Signal, Spectrum, TimeFrequency };

    CRendererContext() { Clear(); }
    explicit CRendererContext(CRendererContext* parentCtx);

    ~CRendererContext() = default;

    void Clear();

    void SetParentRendererContext(CRendererContext* ctx) { m_parentCtx = ctx; }

    void ClearChannelInfo();
    void AddChannel(const std::string& name, float x = 0, float y = 0, float z = 0);
    void SelectChannel(const size_t index);
    void UnselectChannel(const size_t index);
    void SortSelectedChannel(const size_t mode);

    void SetDimensionLabel(const size_t idx1, const size_t idx2, const char* label);
    size_t GetDimensionLabelCount(const size_t index) const;
    const char* GetDimensionLabel(const size_t idx1, const size_t idx2) const;

    void ClearTransformInfo();
    void ScaleBy(const float scale) { m_scale *= scale; }
    void SetScale(const float scale) { m_scale = scale; }
    void ZoomBy(const float zoom) { m_zoom *= zoom; }
    void RotateByX(const float rotation) { m_rotationX += rotation; }
    void RotateByY(const float rotation) { m_rotationY += rotation; }

    void SetTranslucency(const float translucency) { m_translucency = translucency; }
    void SetAspect(const float aspect) { m_aspect = aspect; }
    void SetSampleDuration(const uint64_t sampleDuration) { m_sampleDuration = sampleDuration; }
    void SetTimeScale(const size_t timeScale) { m_timeScale = timeScale; }
    void SetElementCount(const size_t elementCount) { m_nElement = elementCount; }
    void SetFlowerRingCount(const size_t flowerRingCount) { m_nFlowerRing = flowerRingCount; }
    void SetXyzPlotDepth(const bool hasDepth) { m_hasXYZPlotDepth = hasDepth; }
    void SetAxisDisplay(const bool isAxisDisplayed) { m_isAxisDisplayed = isAxisDisplayed; }
    void SetPositiveOnly(const bool bPositiveOnly) { m_isPositiveOnly = bPositiveOnly; }
    void SetTimeLocked(const bool timeLocked) { m_isTimeLocked = timeLocked; }
    void SetScrollModeActive(const bool scrollModeActive) { m_isScrollModeActive = scrollModeActive; }
    void SetScaleVisibility(const bool visibility) { m_scaleVisiblity = visibility; }
    void SetCheckBoardVisibility(const bool visibility) { m_checkBoardVisiblity = visibility; }
    void SetDataType(const EDataType type) { m_dataType = type; }
    void SetSpectrumFrequencyRange(const size_t range) { m_spectrumFreqRange = range; }
    void SetMinimumSpectrumFrequency(const size_t frequency) { m_minSpectrumFreq = frequency; }
    void SetMaximumSpectrumFrequency(const size_t frequency) { m_maxSpectrumFreq = frequency; }
    void SetStackCount(const size_t count) { m_nStack = count; }
    void SetStackIndex(const size_t index) { m_stackIdx = index; }
    void SetFaceMeshVisible(const bool visible) { m_faceMeshVisible = visible; }
    void SetScalpMeshVisible(const bool visible) { m_scalpMeshVisible = visible; }

    void SetERPPlayerActive(const bool active) { m_erpPlayerActive = active; }
    void StepERPFractionBy(const float erpFraction) { m_erpFraction += erpFraction; }

    std::string GetChannelName(const size_t index) const;
    bool GetChannelLocalisation(const size_t index, float& x, float& y, float& z) const;
    size_t GetChannelCount() const { return m_channelName.size(); }
    size_t GetSelectedCount() const { return m_channelLookup.size(); }
    size_t GetSelected(const size_t index) const { return m_channelLookup[index]; }
    bool IsSelected(const size_t index) const;

    float GetScale() const { return m_scale * (m_parentCtx ? m_parentCtx->GetScale() : 1); }
    float GetZoom() const { return m_zoom * (m_parentCtx ? m_parentCtx->GetZoom() : 1); }
    float GetRotationX() const { return m_rotationX + (m_parentCtx ? m_parentCtx->GetRotationX() : 0); }
    float GetRotationY() const { return m_rotationY + (m_parentCtx ? m_parentCtx->GetRotationY() : 0); }

    float GetTranslucency() const { return m_translucency * (m_parentCtx ? m_parentCtx->GetTranslucency() : 1); }
    float GetAspect() const { return m_aspect * (m_parentCtx ? m_parentCtx->GetAspect() : 1); }
    uint64_t GetSampleDuration() const { return m_sampleDuration; }
    size_t GetTimeScale() const { return m_timeScale; }
    size_t GetElementCount() const { return m_nElement; }
    size_t GetFlowerRingCount() const { return m_nFlowerRing; }
    bool HasXyzPlotDepth() const { return m_hasXYZPlotDepth; }
    bool IsAxisDisplayed() const { return m_isAxisDisplayed; }
    bool IsPositiveOnly() const { return m_isPositiveOnly; }
    bool IsTimeLocked() const { return m_isTimeLocked; }
    bool IsScrollModeActive() const { return m_isScrollModeActive; }
    bool GetScaleVisibility() const { return (m_parentCtx ? m_parentCtx->GetScaleVisibility() : m_scaleVisiblity); }
    bool GetCheckBoardVisibility() const { return (m_parentCtx ? m_parentCtx->GetCheckBoardVisibility() : m_checkBoardVisiblity); }
    EDataType GetDataType() const { return m_dataType; }
    size_t GetSpectrumFrequencyRange() const { return m_spectrumFreqRange; }
    size_t GetMinSpectrumFrequency() const { return m_minSpectrumFreq > m_spectrumFreqRange ? m_spectrumFreqRange : m_minSpectrumFreq; }
    size_t GetMaxSpectrumFrequency() const { return m_maxSpectrumFreq > m_spectrumFreqRange ? m_spectrumFreqRange : m_maxSpectrumFreq; }
    size_t GetStackCount() const { return m_nStack; }
    size_t GetStackIndex() const { return m_stackIdx; }
    bool IsFaceMeshVisible() const { return m_faceMeshVisible; }
    bool IsScalpMeshVisible() const { return m_scalpMeshVisible; }

    bool IsERPPlayerActive() const { return m_erpPlayerActive || (m_parentCtx ? m_parentCtx->IsERPPlayerActive() : false); }
    float GetERPFraction() const;

    static size_t GetMaximumSampleCountPerDisplay() { return 1000; } /*500;*/ /*128*/

protected:
    CRendererContext* m_parentCtx = nullptr;

    std::vector<size_t> m_channelLookup;
    std::vector<std::string> m_channelName;
    std::vector<CVertex> m_channelPos;
    std::map<size_t, std::vector<std::string>> m_dimLabels;

    std::map<std::string, float> m_leftRightScore;
    std::map<std::string, float> m_frontBackScore;

    float m_scale     = 1;
    float m_zoom      = 1;
    float m_rotationX = 2;
    float m_rotationY = 1;

    float m_translucency       = 1;
    float m_aspect             = 1;
    uint64_t m_sampleDuration  = 0;
    size_t m_timeScale         = 1;
    size_t m_nElement          = 1;
    size_t m_nFlowerRing       = 1;
    bool m_hasXYZPlotDepth     = false;
    bool m_isAxisDisplayed     = false;
    bool m_isPositiveOnly      = false;
    bool m_isTimeLocked        = true;
    bool m_isScrollModeActive  = false;
    bool m_scaleVisiblity      = true;
    bool m_checkBoardVisiblity = false;
    EDataType m_dataType       = EDataType::Matrix;
    size_t m_spectrumFreqRange = 0;
    size_t m_minSpectrumFreq   = 0;
    size_t m_maxSpectrumFreq   = 0;
    size_t m_nStack            = 1;
    size_t m_stackIdx          = 1;
    bool m_faceMeshVisible     = true;
    bool m_scalpMeshVisible    = true;

    bool m_erpPlayerActive = false;
    float m_erpFraction    = 0;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
