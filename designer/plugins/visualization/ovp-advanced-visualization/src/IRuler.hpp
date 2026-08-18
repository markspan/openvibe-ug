///-------------------------------------------------------------------------------------------------
/// 
/// \file IRuler.hpp
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

#include <mensia/advanced-visualization.hpp>

#include <gtk/gtk.h>

#include <cmath>
#include <vector>
#include <cstdlib>

#define IRuler_SplitCount 5

namespace OpenViBE {
namespace AdvancedVisualization {
class IRuler
{
public:
    IRuler() { }
    IRuler(const IRuler&) = delete;
    virtual ~IRuler()     = default;

    virtual void SetRendererContext(const CRendererContext* ctx) { m_rendererCtx = ctx; }
    virtual void SetRenderer(const IRenderer* renderer) { m_renderer = renderer; }

    virtual void DoRender() { this->render(); }
    virtual void DoRenderLeft(GtkWidget* widget) { if (m_rendererCtx->GetScaleVisibility()) { this->renderLeft(widget); } }
    virtual void DoRenderRight(GtkWidget* widget) { if (m_rendererCtx->GetScaleVisibility()) { this->renderRight(widget); } }
    virtual void DoRenderBottom(GtkWidget* widget) { if (m_rendererCtx->GetScaleVisibility()) { this->renderBottom(widget); } }

protected:
    virtual void render() { }
    virtual void renderLeft(GtkWidget* /*widget*/) { }
    virtual void renderRight(GtkWidget* /*widget*/) { }
    virtual void renderBottom(GtkWidget* /*widget*/) { }

    std::vector<double> splitRange(const double start, const double stop, const size_t count = 10) const
    {
        std::vector<double> res;
        const double range = stop - start;
        const double order = floor(log(range) / log(10.0) - 0.1);
        double step        = pow(10, order);
        double nStep       = trunc(range / step);

        while (nStep < double(count)) {
            nStep *= 2;
            step /= 2;
        }
        while (nStep > double(count)) {
            nStep /= 2;
            step *= 2;
        }

        double value = trunc(start / step) * step;
        while (value < start) { value += step; }
        while (value <= stop) {
            res.push_back(std::abs(value) < std::abs(range / 1000) ? 0 : value);
            value += step;
        }
        return res;
    }

    static std::string getLabel(const double v)
    {
        char label[1024];
        (fabs(v) < 1E-10) ? sprintf(label, "0") : sprintf(label, "%g", v);
        return label;
        //return (fabs(v) < 1E-10) ? "0" : std::to_string(v); // To string can cause problem, fstream method is a little long for g : https://stackoverflow.com/questions/35591647/how-to-let-ss-f-work-like-printfg-f
    }

    const CRendererContext* m_rendererCtx = nullptr;
    const IRenderer* m_renderer           = nullptr;
    float m_blackAlpha                    = 0.9F;
    float m_whiteAlpha                    = 1.0F;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
