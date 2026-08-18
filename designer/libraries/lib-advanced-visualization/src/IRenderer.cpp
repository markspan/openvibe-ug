///-------------------------------------------------------------------------------------------------
/// 
/// \file IRenderer.cpp
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

#include "IRenderer.hpp"

#include "CRendererBars.hpp"
#include "CRendererBitmap.hpp"
#include "CRendererConnectivity.hpp"
#include "CRendererCube.hpp"
#include "CRendererFlower.hpp"
#include "CRendererLine.hpp"
#include "CRendererLoreta.hpp"
#include "CRendererMountain.hpp"
#include "CRendererMultiLine.hpp"
#include "CRendererSlice.hpp"
#include "TRendererStimulation.hpp"
#include "CRendererTopo2D.hpp"
#include "CRendererTopo3D.hpp"
#include "CRendererXYZPlot.hpp"

#include <cmath>
#include <algorithm>    // std::min_element, std::max_element

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

static int iCount = 0;

IRenderer* IRenderer::Create(const ERendererType type, const bool stimulation)
{
    switch (type) {
        case ERendererType::Topography2D: return (stimulation ? nullptr : new CRendererTopo2D);
        case ERendererType::Topography3D: return (stimulation ? nullptr : new CRendererTopo3D);
        case ERendererType::Bars: return (stimulation ? new TRendererStimulation<true, CRendererBars> : new CRendererBars);
        case ERendererType::Bitmap: return (stimulation ? new TRendererStimulation<true, CRendererBitmap> : new CRendererBitmap);
        case ERendererType::Connectivity: return (stimulation ? nullptr : new CRendererConnectivity);
        case ERendererType::Cube: return (stimulation ? nullptr : new CRendererCube);
        case ERendererType::Flower: return (stimulation ? nullptr : new CRendererFlower);
        case ERendererType::Line: return (stimulation ? new TRendererStimulation<false, CRendererLine> : new CRendererLine);
        case ERendererType::Loreta: return (stimulation ? nullptr : new CRendererLoreta);
        case ERendererType::Mountain: return (stimulation ? nullptr : new CRendererMountain);
        case ERendererType::MultiLine: return (stimulation ? new TRendererStimulation<false, CRendererMultiLine> : new CRendererMultiLine);
        case ERendererType::Slice: return (stimulation ? nullptr : new CRendererSlice);
        case ERendererType::XYZPlot: return (stimulation ? nullptr : new CRendererXYZPlot);
        case ERendererType::Default: // return (stimulation ? new TRendererStimulation<false, CRenderer> : new CRenderer);
        case ERendererType::Last:
        default: return nullptr;
    }
}


IRenderer::IRenderer() { iCount++; }
IRenderer::~IRenderer() { iCount--; }

void IRenderer::SetChannelCount(const size_t nChannel)
{
    m_nChannel        = nChannel;
    m_nInverseChannel = (nChannel != 0 ? 1.0F / float(nChannel) : 1);
    m_vertex.clear();
    m_mesh.clear();

    m_historyIdx = 0;
    m_nHistory   = 0;
    m_history.clear();
    m_history.resize(nChannel);
}

void IRenderer::SetSampleCount(const size_t nSample)
{
    m_nSample        = nSample == 0 ? 1 : nSample;
    m_nInverseSample = (m_nSample != 0 ? 1.0F / float(m_nSample) : 1);
    m_vertex.clear();
    m_mesh.clear();
}

void IRenderer::Feed(const float* data)
{
    for (size_t i = 0; i < m_nChannel; ++i) { m_history[i].push_back(data[i]); }
    m_nHistory++;
}

void IRenderer::Feed(const float* data, const size_t nSample)
{
    for (size_t i = 0; i < m_nChannel; ++i) {
        for (size_t j = 0; j < nSample; ++j) { m_history[i].push_back(data[j]); }
        data += nSample;
    }
    m_nHistory += nSample;
}

void IRenderer::Prefeed(const size_t nPreFeedSample)
{
    for (size_t i = 0; i < m_nChannel; ++i) { m_history[i].insert(m_history[i].begin(), nPreFeedSample, 0.0F); }
    m_nHistory += nPreFeedSample;
    m_historyIdx = 0;
}

float IRenderer::GetSuggestedScale() const
{
    if (m_nChannel != 0) {
        std::vector<float> averages;

        for (size_t i = 0; i < m_nChannel; ++i) {
            averages.push_back(0);

            const size_t n = (m_history[i].size() < m_nSample) ? m_history[i].size() : m_nSample;

            for (size_t j = m_history[i].size(); j > (m_history[i].size() - n); --j) { averages.back() += m_history[i][j - 1]; }

            averages.back() /= float(n);
        }

        return (1 / *std::max_element(averages.begin(), averages.end()));
    }
    return 0;
}

void IRenderer::Clear(const size_t nSampleToKeep)
{
    if (!m_history.empty()) {
        if (nSampleToKeep == 0) {
            for (auto& vec : m_history) { vec.clear(); }
            m_nHistory = 0;
        }
        else if (nSampleToKeep < m_history[0].size()) {
            const size_t sampleToDelete = m_history[0].size() - nSampleToKeep;

            if (sampleToDelete > 1) {
                for (auto& vec : m_history) { std::vector<float>(vec.begin() + sampleToDelete, vec.end()).swap(vec); }
                m_nHistory -= size_t(sampleToDelete);
            }
        }
    }
    // We always delete all of the stimulations, ideally we would know the time
    // scale so we can keep the stimulations according to the kept samples
    m_stimulationHistory.clear();
    m_historyIdx = 0;
}

void IRenderer::SetHistoryDrawIndex(const size_t index)
{
    m_historyDrawIdx = index;
    m_historyIdx     = 0;
}

bool IRenderer::GetSampleAtERPFraction(const float erpFraction, std::vector<float>& samples) const
{
    samples.resize(m_nChannel);

    if (m_nSample > m_nHistory) { return false; }

    const float sampleIndexERP   = (erpFraction * float(m_nSample - 1));
    const float alpha            = sampleIndexERP - std::floor(sampleIndexERP);
    const size_t sampleIndexERP1 = size_t(sampleIndexERP) % m_nSample;
    const size_t sampleIndexERP2 = size_t(sampleIndexERP + 1) % m_nSample;

    for (size_t i = 0; i < m_nChannel; ++i) {
        samples[i] = m_history[i][m_nHistory - m_nSample + sampleIndexERP1] * (1 - alpha) + m_history[i][m_nHistory - m_nSample + sampleIndexERP2] * (alpha);
    }

    return true;
}

void IRenderer::Refresh(const CRendererContext& ctx)
{
    if (!m_nSample) {
        m_erpFraction    = 0;
        m_sampleIndexERP = 0;
        return;
    }

    m_erpFraction    = ctx.GetERPFraction();
    m_sampleIndexERP = size_t(m_erpFraction * float(m_nSample - 1)) % m_nSample;
}

/*
bool IRenderer::render(const CRendererContext & ctx)
{
    ::glLineWidth(7);
    ::glColor3f(1.f, 0.9f, 0.1f);
    ::glDisable(GL_TEXTURE_1D);
    ::glBegin(GL_LINES);
    ::glVertex2f(0, 0);
    ::glVertex2f(1, 1);
    ::glVertex2f(0, 1);
    ::glVertex2f(1, 0);
    ::glEnd();
    ::glBegin(GL_LINE_LOOP);
    ::glVertex2f(0, 0);
    ::glVertex2f(0, 1);
    ::glVertex2f(1, 1);
    ::glVertex2f(1, 0);
    ::glEnd();
}
*/

void IRenderer::Draw3DCoordinateSystem()

{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_1D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2);

    glPushMatrix();
    glColor3f(0.2F, 0.2F, 0.2F);
    glScalef(0.2F, 0.2F, 0.2F);
    glBegin(GL_LINES);
    for (int x = -10; x <= 10; ++x) {
        for (int z = -10; z <= 10; ++z) {
            if (x != 0) {
                glVertex3f(float(x), 0, 10.0F);
                glVertex3f(float(x), 0, -10.0F);
            }
            if (z != 0) {
                glVertex3f(10.0F, 0, float(z));
                glVertex3f(-10.0F, 0, float(z));
            }
        }
    }
    glEnd();
    glPopMatrix();

    glBegin(GL_LINES);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 2.0F);
    glVertex3f(0, 0, -3.0F);
    glColor3f(0, 1, 0);
    glVertex3f(0, 1.25F, 0);
    glVertex3f(0, -1.25F, 0);
    glColor3f(1, 0, 0);
    glVertex3f(2.0F, 0, 0);
    glVertex3f(-2.0F, 0, 0);
    glEnd();

    glPopAttrib();
}

void IRenderer::Draw2DCoordinateSystem()

{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_1D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2);

    glPushMatrix();
    glColor3f(0.2F, 0.2F, 0.2F);
    glScalef(0.2F, 0.2F, 0.2F);
    glBegin(GL_LINES);
    for (int x = -10; x <= 10; ++x) {
        for (int y = -10; y <= 10; ++y) {
            if (x != 0) {
                glVertex2f(float(x), 10.0F);
                glVertex2f(float(x), -10.0F);
            }
            if (y != 0) {
                glVertex2f(10.0F, float(y));
                glVertex2f(-10.0F, float(y));
            }
        }
    }
    glEnd();
    glPopMatrix();

    glBegin(GL_LINES);
    glColor3f(0, 1, 0);
    glVertex2f(0, 2.0F);
    glVertex2f(0, -2.0F);
    glColor3f(1, 0, 0);
    glVertex2f(2.0F, 0);
    glVertex2f(-2.0F, 0);
    glEnd();

    glPopAttrib();
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
