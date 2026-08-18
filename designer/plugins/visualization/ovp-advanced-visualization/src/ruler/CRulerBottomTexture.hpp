///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerBottomTexture.hpp
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

#include "CRulerTexture.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
class CRulerBottomTexture final : public CRulerTexture
{
public:
    void render() override

    {
        this->preRender();

        glColor4f(0, 0, 0, m_blackAlpha);
        glBegin(GL_QUADS);
        glTexCoord1f(0);
        glVertex2f(0, 0.00F);
        glVertex2f(0, 0.05F);
        glTexCoord1f(1);
        glVertex2f(1, 0.05F);
        glVertex2f(1, 0.00F);
        glEnd();

        glColor4f(1, 1, 1, m_whiteAlpha);
        glBegin(GL_QUADS);
        glTexCoord1f(0);
        glVertex2f(0, 0.00F);
        glVertex2f(0, 0.04F);
        glTexCoord1f(1);
        glVertex2f(1, 0.04F);
        glVertex2f(1, 0.00F);
        glEnd();

        this->postRender();
    }

    void renderBottom(GtkWidget* widget) override
    {
        const double scale = 1.0 / double(m_rendererCtx->GetScale());
        if (std::fabs(m_lastScale - scale) > DBL_EPSILON) {
            m_range     = splitRange(-scale * 0.5, scale * 0.5);
            m_lastScale = scale;
        }

        gint w, h;
        gint lw, lh;

        gdk_drawable_get_size(widget->window, &w, &h);
        GdkGC* drawGC = gdk_gc_new(widget->window);
        for (const auto& i : m_range) {
            PangoLayout* layout = gtk_widget_create_pango_layout(widget, getLabel(i).c_str());
            pango_layout_get_size(layout, &lw, &lh);
            lw /= PANGO_SCALE;
            lh /= PANGO_SCALE;
            gdk_draw_layout(widget->window, drawGC, gint((0.5 + i / scale) * w - lw * 0.5), 0, layout);
            g_object_unref(layout);
        }
        g_object_unref(drawGC);
    }

protected:
    double m_lastScale = 0;
    std::vector<double> m_range;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
