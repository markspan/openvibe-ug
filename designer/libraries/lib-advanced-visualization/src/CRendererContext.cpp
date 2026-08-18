///-------------------------------------------------------------------------------------------------
/// 
/// \file CRendererContext.cpp
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

#include "CRendererContext.hpp"

#include <algorithm>
#include <map>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <float.h>

namespace OpenViBE {
namespace AdvancedVisualization {

namespace {
void getLeftRightScore(std::map<std::string, float>& scores, const std::vector<std::string>& names, const std::vector<CVertex>& positions)
{
    for (size_t i = 0; i < names.size() && i < positions.size(); ++i) {
        std::string name = std::string(",") + names[i] + std::string(",");
        std::transform(name.begin(), name.end(), name.begin(), tolower);
        scores[name] = positions[i].x * 1E-0F + positions[i].y * 1E-10F + positions[i].z * 1E-5F;
    }
}

void getFrontBackScore(std::map<std::string, float>& scores, const std::vector<std::string>& names, const std::vector<CVertex>& positions)
{
    for (size_t i = 0; i < names.size() && i < positions.size(); ++i) {
        std::string name = std::string(",") + names[i] + std::string(",");
        std::transform(name.begin(), name.end(), name.begin(), tolower);
        scores[name] = positions[i].x * 1E-5F + positions[i].y * 1E-10F + positions[i].z * 1E-0F;
    }
}

struct SSortAlpha
{
    explicit SSortAlpha(const std::vector<std::string>& channels) : names(channels) { }

    bool operator()(const size_t i, const size_t j) const
    {
        std::string nameI = names[i];
        std::string nameJ = names[j];
        std::transform(nameI.begin(), nameI.end(), nameI.begin(), tolower);
        std::transform(nameJ.begin(), nameJ.end(), nameJ.begin(), tolower);

        return nameI < nameJ;
    }

    const std::vector<std::string>& names;
};

struct SSortSpecial
{
    SSortSpecial(const std::vector<std::string>& channels, const std::map<std::string, float>& scoreMap) : names(channels), scores(scoreMap) { }

    bool operator()(const size_t i, const size_t j) const
    {
        float scoreI = 0;
        float scoreJ = 0;

        std::string nameI = std::string(",") + names[i] + std::string(",");
        std::string nameJ = std::string(",") + names[j] + std::string(",");
        std::transform(nameI.begin(), nameI.end(), nameI.begin(), tolower);
        std::transform(nameJ.begin(), nameJ.end(), nameJ.begin(), tolower);

        for (auto it = scores.begin(); it != scores.end(); ++it) {
            if (it->first == nameI) { scoreI = it->second; }
            if (it->first == nameJ) { scoreJ = it->second; }
        }

        return scoreI < scoreJ;
    }

    const std::vector<std::string>& names;
    const std::map<std::string, float>& scores;
};
}  // namespace

CRendererContext::CRendererContext(CRendererContext* parentCtx)
{
    Clear();
    SetParentRendererContext(parentCtx);
}

void CRendererContext::Clear()
{
    this->ClearChannelInfo();
    this->ClearTransformInfo();
    m_dimLabels.clear();
}

void CRendererContext::ClearChannelInfo()
{
    m_channelLookup.clear();
    m_channelName.clear();
    m_channelPos.clear();

    m_leftRightScore.clear();
    m_frontBackScore.clear();
}

void CRendererContext::AddChannel(const std::string& name, const float x, const float y, const float z)
{
    const auto norm     = float(sqrt(x * x + y * y + z * z));
    const float invNorm = (norm > FLT_EPSILON ? 1.0F / norm : 0);
    CVertex pos;
    pos.x = x * invNorm;
    pos.y = y * invNorm;
    pos.z = z * invNorm;
    m_channelLookup.push_back(m_channelName.size());
    m_channelName.push_back(name);
    m_channelPos.push_back(pos);
}

void CRendererContext::SelectChannel(const size_t index)
{
    for (const auto& i : m_channelLookup) { if (i == index) { return; } }
    m_channelLookup.push_back(index);
}

void CRendererContext::UnselectChannel(const size_t index)
{
    for (size_t i = 0; i < m_channelLookup.size(); ++i) {
        if (m_channelLookup[i] == index) {
            m_channelLookup.erase(m_channelLookup.begin() + i);
            return;
        }
    }
}

void CRendererContext::SortSelectedChannel(const size_t mode)
{
    if (m_leftRightScore.empty()) { getLeftRightScore(m_leftRightScore, m_channelName, m_channelPos); }
    if (m_frontBackScore.empty()) { getFrontBackScore(m_frontBackScore, m_channelName, m_channelPos); }

    switch (mode) {
        case 0: break;

        case 1: std::stable_sort(m_channelLookup.begin(), m_channelLookup.end());
            break;

        case 2: std::stable_sort(m_channelLookup.begin(), m_channelLookup.end(), SSortAlpha(m_channelName));
            break;

        case 3: std::reverse(m_channelLookup.begin(), m_channelLookup.end());
            break;

        case 4: std::stable_sort(m_channelLookup.begin(), m_channelLookup.end(), SSortSpecial(m_channelName, m_leftRightScore));
            break;

        case 5: std::stable_sort(m_channelLookup.begin(), m_channelLookup.end(), SSortSpecial(m_channelName, m_frontBackScore));
            break;

        default: break;
    }
}

///-------------------------------------------------------------------------------------------------
void CRendererContext::SetDimensionLabel(const size_t idx1, const size_t idx2, const char* label)
{
    if (m_dimLabels[idx1].size() <= idx2) { m_dimLabels[idx1].resize(idx2 + 1); }
    m_dimLabels[idx1][idx2] = label;
}

size_t CRendererContext::GetDimensionLabelCount(const size_t index) const
{
    if (m_dimLabels.count(index) == 0) { return 0; }
    return m_dimLabels.at(index).size();
}

const char* CRendererContext::GetDimensionLabel(const size_t idx1, const size_t idx2) const
{
    if (m_dimLabels.count(idx1) == 0 || m_dimLabels.at(idx1).size() <= idx2) { return nullptr; }
    return m_dimLabels.at(idx1)[idx2].c_str();
}

///-------------------------------------------------------------------------------------------------
void CRendererContext::ClearTransformInfo()
{
    m_parentCtx           = nullptr;
    m_scale               = 1;
    m_zoom                = 1;
    m_rotationX           = 2;
    m_rotationY           = 1;
    m_translucency        = 1;
    m_aspect              = 1;
    m_sampleDuration      = 0;
    m_timeScale           = 1;
    m_nElement            = 1;
    m_nFlowerRing         = 1;
    m_hasXYZPlotDepth     = false;
    m_isAxisDisplayed     = false;
    m_isPositiveOnly      = false;
    m_isTimeLocked        = true;
    m_isScrollModeActive  = false;
    m_checkBoardVisiblity = false;
    m_scaleVisiblity      = true;
    m_dataType            = EDataType::Matrix;
    m_spectrumFreqRange   = 0;
    m_minSpectrumFreq     = 0;
    m_maxSpectrumFreq     = 0;
    m_erpPlayerActive     = false;
    m_erpFraction         = 0;
    m_nStack              = 1;
    m_stackIdx            = 1;
    m_faceMeshVisible     = true;
    m_scalpMeshVisible    = true;
}

std::string CRendererContext::GetChannelName(const size_t index) const
{
    if (index < m_channelName.size()) { return m_channelName[index]; }
    std::cout << "No name for channel " << index << std::endl;
    return std::string();
}

bool CRendererContext::GetChannelLocalisation(const size_t index, float& x, float& y, float& z) const
{
    const CVertex& tmp = m_channelPos[index];
    x                  = tmp.x;
    y                  = tmp.y;
    z                  = tmp.z;
    return true;
}

bool CRendererContext::IsSelected(const size_t index) const
{
    for (auto& i : m_channelLookup) { if (i == index) { return true; } }
    return false;
}

float CRendererContext::GetERPFraction() const
{
    const float erpFraction = m_erpFraction + (m_parentCtx ? m_parentCtx->GetERPFraction() : 0);
    return erpFraction - floorf(erpFraction);
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
