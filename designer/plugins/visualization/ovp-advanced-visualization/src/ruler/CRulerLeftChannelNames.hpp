///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerLeftChannelNames.hpp
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

#include <string>

namespace OpenViBE {
namespace AdvancedVisualization {
class CRulerLeftChannelNames final : public IRuler
{
public:
    void renderLeft(GtkWidget* widget) override
    {
        gint w, h, lw, lh;
        gdk_drawable_get_size(widget->window, &w, &h);
        GdkGC* drawGC = gdk_gc_new(widget->window);
        for (size_t i = 0; i < m_rendererCtx->GetSelectedCount(); ++i) {
            const size_t idx        = m_rendererCtx->GetSelected(i);
            const std::string label = (m_rendererCtx->GetChannelName(idx) + " (" + std::to_string(idx + 1) + ")");
            PangoLayout* layout     = gtk_widget_create_pango_layout(widget, label.c_str());
            pango_layout_get_size(layout, &lw, &lh);
            lw /= PANGO_SCALE;
            lh /= PANGO_SCALE;
            gdk_draw_layout(widget->window, drawGC, w - lw, gint(((double(i) + 0.5) * h) / double(m_rendererCtx->GetSelectedCount()) - double(lh) / 2.0), layout);
            g_object_unref(layout);
        }
        g_object_unref(drawGC);
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
