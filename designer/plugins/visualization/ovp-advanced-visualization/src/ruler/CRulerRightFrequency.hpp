///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerRightFrequency.hpp
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
class CRulerRightFrequency final : public IRuler
{
public:
    void renderRight(GtkWidget* widget) override
    {
        const auto scale = double(m_rendererCtx->GetSpectrumFrequencyRange());
        if (std::fabs(m_lastScale - scale) > DBL_EPSILON) {
            m_range     = splitRange(0, scale);
            m_lastScale = scale;
        }

        gint w, h;
        gint lw, lh;

        const size_t nChannel = m_rendererCtx->GetSelectedCount();
        for (size_t i = 0; i < nChannel; ++i) {
            gdk_drawable_get_size(widget->window, &w, &h);
            GdkGC* drawGC = gdk_gc_new(widget->window);
            for (const auto& r : m_range) {
                const gint y        = gint((double(i) + r / scale) * (double(h) / double(nChannel)));
                PangoLayout* layout = gtk_widget_create_pango_layout(widget, getLabel(r).c_str());
                pango_layout_get_size(layout, &lw, &lh);
                lw /= PANGO_SCALE;
                lh /= PANGO_SCALE;
                gdk_draw_layout(widget->window, drawGC, 8, h - y - lh / 2, layout);
                gdk_draw_line(widget->window, drawGC, 0, h - y, 3, h - y);
                g_object_unref(layout);
            }
            g_object_unref(drawGC);
        }
    }

protected:
    double m_lastScale = 1;
    std::vector<double> m_range;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
