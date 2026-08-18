///-------------------------------------------------------------------------------------------------
/// 
/// \file CROCCurveDraw.cpp
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

#if defined(TARGET_HAS_ThirdPartyGTK)

#include "CROCCurveDraw.hpp"

#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

static void SizeAllocateCB(GtkWidget* /*widget*/, GdkRectangle* rectangle, gpointer data) { static_cast<CROCCurveDraw*>(data)->ResizeEvent(rectangle); }
static void AreaExposeCB(GtkWidget* /*widget*/, GdkEventExpose* /*event*/, gpointer data) { static_cast<CROCCurveDraw*>(data)->ExposeEnvent(); }

CROCCurveDraw::CROCCurveDraw(GtkNotebook* notebook, const std::string& className)
    : m_drawableArea(gtk_drawing_area_new())
{
    gtk_widget_set_size_request(m_drawableArea, 700, 600);

    g_signal_connect(G_OBJECT(m_drawableArea), "expose-event", G_CALLBACK(AreaExposeCB), this);
    g_signal_connect(G_OBJECT(m_drawableArea), "size-allocate", G_CALLBACK(SizeAllocateCB), this);

    GtkWidget* label = gtk_label_new(className.c_str());
    gtk_notebook_append_page(notebook, m_drawableArea, label);


    //get left ruler widget's font description
    PangoContext* ctx                    = gtk_widget_get_pango_context(m_drawableArea);
    const PangoFontDescription* fontDesc = pango_context_get_font_description(ctx);

    //adapt the allocated height per label to the font's height (plus 4 pixel to add some spacing)
    if (pango_font_description_get_size_is_absolute(fontDesc)) { m_pixelsPerLeftRulerLabel = pango_font_description_get_size(fontDesc) + 4; }
    else { m_pixelsPerLeftRulerLabel = pango_font_description_get_size(fontDesc) / PANGO_SCALE + 4; }
}

void CROCCurveDraw::GenerateCurve()
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(m_drawableArea, &allocation);

    const size_t width  = allocation.width - 2 * m_margin;
    const size_t height = allocation.height - 2 * m_margin;

    m_pointList.clear();
    for (size_t i = 0; i < m_coordinateList.size(); ++i) {
        GdkPoint point;
        point.x = gint(m_coordinateList[i].first * double(width) + double(m_margin));
        point.y = gint((allocation.height - gint(m_margin)) - m_coordinateList[i].second * double(height));
        m_pointList.push_back(point);
    }
    m_hasBeenInit = true;
}

void CROCCurveDraw::ExposeEnvent()
{
    m_hasBeenExposed = true;
    redraw();
}

void CROCCurveDraw::ResizeEvent(GdkRectangle* /*rectangle*/)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation(m_drawableArea, &alloc);

    if (!m_hasBeenInit) { return; }

    GenerateCurve();
    redraw();
}

void CROCCurveDraw::redraw() const
{
    if (!m_hasBeenInit || !m_hasBeenExposed) { return; }

    GtkAllocation allocation;
    gtk_widget_get_allocation(m_drawableArea, &allocation);

    gdk_draw_rectangle(m_drawableArea->window, GTK_WIDGET(m_drawableArea)->style->white_gc, TRUE, 0, 0, allocation.width, allocation.height);


    const GdkColor lineColor = { 0, 35000, 35000, 35000 };
    GdkGC* gc                = gdk_gc_new((m_drawableArea)->window);
    gdk_gc_set_rgb_fg_color(gc, &lineColor);


    //Left ruler
    gdk_draw_line((m_drawableArea)->window, gc, gint(m_margin), gint(m_margin), gint(m_margin), gint(allocation.height - m_margin));
    drawLeftMark(m_margin, m_margin, "1");
    drawLeftMark(m_margin, allocation.height / 2, "0.5");
    drawLeftMark(m_margin, allocation.height - m_margin, "0");

    //*** Black magic section to rotate the text of the left ruler. The solution comes from the internet (gtk doc), it works so
    // don't touch it unless you are sure of what you are doing
    PangoContext* context   = gtk_widget_get_pango_context(m_drawableArea);
    GdkScreen* screen       = gdk_drawable_get_screen(m_drawableArea->window);
    PangoRenderer* renderer = gdk_pango_renderer_get_default(screen);
    gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER(renderer), m_drawableArea->window);
    GdkGC* rotationGc = gdk_gc_new(m_drawableArea->window);
    gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER(renderer), rotationGc);
    int width, height;
    PangoMatrix matrix = PANGO_MATRIX_INIT;
    pango_matrix_translate(&matrix, 0, double(allocation.height + 100) / 2);
    PangoLayout* layout = pango_layout_new(context);
    pango_layout_set_text(layout, "True Positive Rate", -1);
    const PangoFontDescription* desc = pango_context_get_font_description(context);
    pango_layout_set_font_description(layout, desc);
    const GdkColor color = { 0, 0, 0, 0 };
    gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, &color);

    pango_matrix_rotate(&matrix, 90);
    pango_context_set_matrix(context, &matrix);
    pango_layout_context_changed(layout);
    pango_layout_get_size(layout, &width, &height);
    pango_renderer_draw_layout(renderer, layout, 15, (allocation.height + height) / 2);

    gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, nullptr);
    gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER(renderer), nullptr);
    gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER(renderer), nullptr);

    pango_matrix_rotate(&matrix, -90);
    pango_context_set_matrix(context, &matrix);
    pango_layout_context_changed(layout);

    g_object_unref(layout);
    g_object_unref(context);
    g_object_unref(rotationGc);
    //** End of black magic section

    //Bottom ruler
    gdk_draw_line((m_drawableArea)->window, gc, gint(m_margin), gint(allocation.height - m_margin), gint(allocation.width - m_margin),
                  gint(allocation.height - m_margin));
    drawBottomMark(m_margin, allocation.height - m_margin, "0");
    drawBottomMark(allocation.width / 2, allocation.height - m_margin, "0.5");
    drawBottomMark(allocation.width - m_margin, allocation.height - m_margin, "1");

    int textW;
    int textH;
    PangoLayout* text = gtk_widget_create_pango_layout(m_drawableArea, "False positive rate");
    pango_layout_set_justify(text, PANGO_ALIGN_CENTER);
    pango_layout_get_pixel_size(text, &textW, &textH);
    gdk_draw_layout(m_drawableArea->window, GTK_WIDGET(m_drawableArea)->style->black_gc, allocation.width / 2 - textW / 2, allocation.height - 15, text);
    g_object_unref(text);


    if (m_pointList.empty()) { gdk_draw_lines((m_drawableArea)->window, GTK_WIDGET(m_drawableArea)->style->black_gc, &(m_pointList[0]), gint(m_pointList.size())); }

    gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
    gdk_draw_line((m_drawableArea)->window, gc, gint(m_margin), gint(allocation.height - m_margin), gint(allocation.width - m_margin), gint(m_margin));

    g_object_unref(gc);
}

void CROCCurveDraw::drawLeftMark(const size_t w, const size_t h, const char* label) const
{
    gint textW;
    gint textH;
    PangoLayout* text = gtk_widget_create_pango_layout(m_drawableArea, label);
    pango_layout_set_width(text, 28);
    pango_layout_set_justify(text, PANGO_ALIGN_LEFT);

    pango_layout_get_pixel_size(text, &textW, &textH);

    gdk_draw_layout(m_drawableArea->window, GTK_WIDGET(m_drawableArea)->style->black_gc, gint(w - 20) - (textW / 2), gint(h) - (textH / 2), text);

    const GdkColor lineColor = { 0, 35000, 35000, 35000 };
    GdkGC* gc                = gdk_gc_new((m_drawableArea)->window);
    gdk_gc_set_rgb_fg_color(gc, &lineColor);
    gdk_draw_line(m_drawableArea->window, gc, gint(w - 5), gint(h), gint(w), gint(h));

    g_object_unref(gc);
}

void CROCCurveDraw::drawBottomMark(const size_t w, const size_t h, const char* label) const
{
    int textW;
    int textH;
    PangoLayout* text = gtk_widget_create_pango_layout(m_drawableArea, label);
    pango_layout_set_width(text, 28);
    pango_layout_set_justify(text, PANGO_ALIGN_LEFT);

    pango_layout_get_pixel_size(text, &textW, &textH);

    gdk_draw_layout(m_drawableArea->window, GTK_WIDGET(m_drawableArea)->style->black_gc, gint(w) - (textW / 2), gint(h + 14), text);

    const GdkColor lineColor = { 0, 35000, 35000, 35000 };
    GdkGC* gc                = gdk_gc_new((m_drawableArea)->window);
    gdk_gc_set_rgb_fg_color(gc, &lineColor);
    gdk_draw_line(m_drawableArea->window, gc, gint(w), gint(h + 5), gint(w), gint(h));

    g_object_unref(gc);
}

}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGTK
