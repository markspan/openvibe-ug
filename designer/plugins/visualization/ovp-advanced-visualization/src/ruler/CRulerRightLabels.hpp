///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerRightLabels.hpp
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
template <size_t TDim>
class CRulerRightLabels final : public IRuler
{
public:
    void renderRight(GtkWidget* widget) override
    {
        gint w, h, lw, lh;

        const size_t nChannel = m_rendererCtx->GetSelectedCount();
        for (size_t channel = 0; channel < nChannel; ++channel) {
            gdk_drawable_get_size(widget->window, &w, &h);
            GdkGC* drawGC = gdk_gc_new(widget->window);

            const auto labelCount = double(m_rendererCtx->GetDimensionLabelCount(TDim));

            gint lastY = gint((double(channel) + (-1.0 + 0.5) / labelCount) * (double(h) / double(nChannel)));

            for (size_t label = 0; label < m_rendererCtx->GetDimensionLabelCount(TDim); ++label) {
                const gint y = gint((double(channel) + (double(label) + 0.5) / labelCount) * (double(h) / double(nChannel)));
                if (y >= lastY + 10) {
                    PangoLayout* layout = gtk_widget_create_pango_layout(widget, m_rendererCtx->GetDimensionLabel(TDim, label));
                    pango_layout_get_size(layout, &lw, &lh);
                    lw /= PANGO_SCALE;
                    lh /= PANGO_SCALE;
                    gdk_draw_layout(widget->window, drawGC, 8, h - y - lh / 2, layout);
                    gdk_draw_line(widget->window, drawGC, 0, h - y, 3, h - y);
                    g_object_unref(layout);
                    lastY = y;
                }
            }
            g_object_unref(drawGC);
        }
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
