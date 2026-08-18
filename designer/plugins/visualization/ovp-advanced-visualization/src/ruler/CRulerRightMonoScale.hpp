///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerRightMonoScale.hpp
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

#include "../IRuler.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
class CRulerRightMonoScale final : public IRuler
{
public:
    CRulerRightMonoScale() : m_lastScale(-1) { }

    void renderRight(GtkWidget* widget) override
    {
        const double scale = 1.0 / double(m_rendererCtx->GetScale());
        if (std::fabs(m_lastScale - scale) > DBL_EPSILON) {
            if (m_rendererCtx->IsPositiveOnly()) { m_range = splitRange(0, scale, IRuler_SplitCount); }
            else { m_range = splitRange(-scale * 0.5, scale * 0.5, IRuler_SplitCount); }
            m_lastScale = scale;
        }

        const double offset = m_rendererCtx->IsPositiveOnly() ? 0 : 0.5;

        gint w, h, lw, lh;

        gdk_drawable_get_size(widget->window, &w, &h);
        GdkGC* drawGC = gdk_gc_new(widget->window);
        for (const auto& i : m_range) {
            PangoLayout* layout = gtk_widget_create_pango_layout(widget, getLabel(i).c_str());
            pango_layout_get_size(layout, &lw, &lh);
            lw /= PANGO_SCALE;
            lh /= PANGO_SCALE;
            const gint y = gint((1 - (offset + i / scale)) * h);
            gdk_draw_layout(widget->window, drawGC, 8, y - lh / 2, layout);
            gdk_draw_line(widget->window, drawGC, 0, y, 3, y);
            g_object_unref(layout);
        }
        g_object_unref(drawGC);
    }

protected:
    double m_lastScale = 1;
    std::vector<double> m_range;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
