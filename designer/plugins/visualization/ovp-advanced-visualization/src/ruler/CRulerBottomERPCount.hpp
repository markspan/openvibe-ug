///-------------------------------------------------------------------------------------------------
/// 
/// \file CRulerBottomERPCount.hpp
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
class CRulerBottomERPCount final : public IRuler
{
public:
    void renderBottom(GtkWidget* widget) override
    {
        if (m_renderer == nullptr) { return; }
        if (m_renderer->GetSampleCount() == 0) { return; }
        if (m_renderer->GetHistoryCount() == 0) { return; }
        if (m_renderer->GetHistoryIndex() == 0) { return; }

        const double duration           = double(m_renderer->GetSampleCount());
        const std::vector<double> range = splitRange(0, duration, 10);

        gint w, h;

        gdk_drawable_get_size(widget->window, &w, &h);
        GdkGC* drawGC = gdk_gc_new(widget->window);
        for (const auto& i : range) {
            const gint x        = gint((i / duration) * w);
            PangoLayout* layout = gtk_widget_create_pango_layout(widget, getLabel(i).c_str());
            gdk_draw_layout(widget->window, drawGC, x, 5, layout);
            gdk_draw_line(widget->window, drawGC, x, 0, x, 3);
            g_object_unref(layout);
        }
        g_object_unref(drawGC);
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
