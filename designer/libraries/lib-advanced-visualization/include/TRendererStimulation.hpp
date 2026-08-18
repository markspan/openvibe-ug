///-------------------------------------------------------------------------------------------------
/// 
/// \file TRendererStimulation.hpp
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

#include <string>
#include <vector>
#include <array>
#include <map>

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
static const int STIMULATION_INDICATOR_SMOOTHNESS = 10;
static const float STIMULATION_INDICATOR_RADIUS   = 0.01F;
static const float STIMULATION_INDICATOR_SPACING  = 0.03F;

template <bool TPreRender, class T>
class TRendererStimulation : public T
{
public:
    std::vector<std::pair<float, float>> m_Circles;
    std::map<uint64_t, int> m_Stimulations;

    TRendererStimulation()
    {
        // Render a circle into a buffer so we don't have to do this each time

        for (int i = 0; i < STIMULATION_INDICATOR_SMOOTHNESS; ++i) {
            m_Circles.push_back(std::make_pair(STIMULATION_INDICATOR_RADIUS * cosf(float(i) / float(STIMULATION_INDICATOR_SMOOTHNESS - 1) * 2 * float(M_PI)),
                                               STIMULATION_INDICATOR_RADIUS * sinf(float(i) / float(STIMULATION_INDICATOR_SMOOTHNESS - 1) * 2 * float(M_PI))));
        }
    }

    bool Render(const CRendererContext& ctx) override
    {
        bool res = true;

        if (TPreRender) {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            res = T::Render(ctx);
            glPopAttrib();
        }

        bool ok = true;
        ok &= (IRenderer::GetSampleCount() != 0);
        ok &= (IRenderer::GetHistoryCount() != 0);
        ok &= (IRenderer::GetHistoryIndex() != 0);

        if (ok) {
            glPushAttrib(GL_ALL_ATTRIB_BITS);

            const size_t nSample          = IRenderer::GetSampleCount();
            const size_t historyIndex     = IRenderer::GetHistoryIndex();
            const uint64_t sampleDuration = ctx.GetSampleDuration();

            glDisable(GL_TEXTURE_1D);
            glDisable(GL_BLEND);
            glDisable(GL_LINE_SMOOTH);

            const size_t leftIndex = historyIndex - historyIndex % nSample;
            const size_t midIndex  = historyIndex;
            const double startTime = ((leftIndex * sampleDuration) >> 16) / 65536.;
            const double midTime   = ((midIndex * sampleDuration) >> 16) / 65536.;
            const double duration  = ((nSample * sampleDuration) >> 16) / 65536.;

            m_Stimulations.clear();
            for (const auto& stim : IRenderer::m_stimulationHistory) {
                if (m_Stimulations.count(stim.second) == 0) {
                    // we store the "position" of the indicator for each new encountered stimulation
                    // if there are too many of them, we loop over to the beginning.
                    m_Stimulations[stim.second] = m_Stimulations.size() % int(1.0F / STIMULATION_INDICATOR_SPACING);
                }

                if (midTime - duration < stim.first && stim.first < midTime) {
                    float progress;
                    if (stim.first > startTime) { progress = float((stim.first - startTime) / duration); }
                    else { progress = float((stim.first + duration - startTime) / duration); }

                    /*
                    ::glLineWidth(3);
                    ::glColor3f(0, 0, 0);
                    ::glBegin(GL_LINES);
                        ::glVertex2f(progress, 0);
                        ::glVertex2f(progress, 1);
                    ::glEnd();
                    */

                    // draw a vertical line representing the stimulation
                    glLineWidth(1);
                    glColor3fv(getMarkerColor(stim.second));
                    glBegin(GL_LINES);
                    glVertex2f(progress, 0);
                    glVertex2f(progress, 1);
                    glEnd();


                    // draw a (ugly) disc representing a stimulation
                    glBegin(GL_TRIANGLE_FAN);
                    for (const auto& circle : m_Circles) {
                        glVertex2f(float(circle.first / ctx.GetAspect() + progress),
                                   float(circle.second + 0.95F - float(m_Stimulations[stim.second]) * STIMULATION_INDICATOR_SPACING));
                    }
                    glEnd();


                    // now draw
                    glLineWidth(2);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glEnable(GL_LINE_SMOOTH);
                    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
                    glBegin(GL_LINE_LOOP);
                    for (const auto& circle : m_Circles) {
                        glVertex2f(float(circle.first / ctx.GetAspect() + progress),
                                   float(circle.second + 0.95F - float(m_Stimulations[stim.second]) * STIMULATION_INDICATOR_SPACING));
                    }
                    glEnd();
                    glDisable(GL_LINE_SMOOTH);

                    glDisable(GL_BLEND);
                }
            }

            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_BLEND);
            glEnable(GL_TEXTURE_1D);

            glPopAttrib();
        }

        if (!TPreRender) {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            res = T::Render(ctx);
            glPopAttrib();
        }

        return res && ok;
    }

    float* getMarkerColor(const uint64_t id)
    {
        static std::array<float, 4> color;
        const float alpha       = Reverse<>(uint8_t(id & 255)) * 3.0F / 255.0F;
        const auto alphai       = int(alpha);
        color[(alphai + 0) % 3] = 1 - alpha / 3.0F;
        color[(alphai + 1) % 3] = alpha / 3.0F;
        color[(alphai + 2) % 3] = 0;
        color[3]                = 0.75F;
        return color.data();
    }

    template <typename TVector>
    TVector Reverse(TVector v)
    {
        TVector res = 0;
        for (TVector i = 0; i < sizeof(TVector) * 8; ++i) {
            res <<= 1;
            res |= ((v & (1 << i)) ? 1 : 0);
        }
        return res;
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
