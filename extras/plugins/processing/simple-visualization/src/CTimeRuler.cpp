///-------------------------------------------------------------------------------------------------
/// 
/// \file CTimeRuler.cpp
/// \brief Implementation for the class CTimeRuler.
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

#include "CTimeRuler.hpp"
#include <cmath>
#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {


#define CONVERT_TIME(i) (double((i)>>32) + double(double((i)&0xFFFFFFFF) / double((uint64_t)1<<32)))

//CALLBACKS
//! Callback to redraw the bottom ruler
gboolean TimeRulerExposeEventCB(GtkWidget* /*widget*/, GdkEventExpose* /*event*/, gpointer data)
{
    //redraw the ruler
    auto* timeRuler = reinterpret_cast<CTimeRuler*>(data);
    timeRuler->Draw();

    //don't propagate this signal to the children if any
    return TRUE;
}

//! Called when the widget whose width is associated with the ruler is resized.
gboolean TimeRulerResizeCB(GtkWidget* /*widget*/, GtkAllocation* allocation, gpointer data)
{
    const auto* timeRuler = reinterpret_cast<CTimeRuler*>(data);
    timeRuler->OnResizeEventCB(allocation->width, allocation->height);
    return FALSE;
}

CTimeRuler::CTimeRuler(IStreamDatabase& streamDatabase, const int width, const int height)
    : m_widget(gtk_drawing_area_new()), m_stream(streamDatabase), m_height(height)
{
    gtk_widget_set_size_request(m_widget, width, height);
    g_signal_connect_after(G_OBJECT(m_widget), "expose-event", G_CALLBACK(TimeRulerExposeEventCB), this);
}

void CTimeRuler::Draw()
{
    //if the widget is invisible, no need to redraw it
    if (!GTK_WIDGET_VISIBLE(m_widget)) { return; }

    //return if time between two consecutive buffers hasn't been computed yet
    if (!m_stream.IsBufferTimeStepComputed()) { return; }

    //get widget size
    gint bottomRulerW;
    gdk_drawable_get_size(m_widget->window, &bottomRulerW, nullptr);

    const double startTime = CONVERT_TIME(m_stream.GetStartTime(0));
    const double endTime   = CONVERT_TIME(m_stream.GetStartTime(0) + m_stream.GetMaxBufferCount() * m_stream.GetBufferTimeStep());
    const double intervalW = endTime - startTime;

    //compute step between two values displayed on the ruler
    const auto nearestSmallerPowerOf10 = double(pow(10, floor(log10(intervalW))));

    const auto maxNLabels = uint64_t(bottomRulerW / m_pixelsPerLabel);

    double valueStep = nearestSmallerPowerOf10;
    if (uint64_t(floor(intervalW / nearestSmallerPowerOf10)) > maxNLabels) { valueStep = 2 * nearestSmallerPowerOf10; }
    else if (uint64_t(floor(intervalW / nearestSmallerPowerOf10)) < maxNLabels / 2) { valueStep = nearestSmallerPowerOf10 / 2; }

    //recompute step base value
    const double baseValue = valueStep * floor(startTime / valueStep);

    //X position of the first label
    const double bufferW = double(bottomRulerW) / double(m_stream.GetMaxBufferCount());
    auto baseX           = int64_t(floor(bottomRulerW - (double(m_stream.GetCurrentBufferCount()) * bufferW)));
    if (baseX < 0) { baseX = 0; }

    //draw ruler base (horizontal line)
    gdk_draw_line(m_widget->window, m_widget->style->fg_gc[GTK_WIDGET_STATE(m_widget)], gint(baseX), 0, bottomRulerW, 0);

    //draw labels
    std::stringstream timeLabel;
    for (double i = baseValue; i < double(0.5 + endTime); i += valueStep) {
        //clear stringstream
        timeLabel.str("");

        //compute label position
        const gint textX = gint(double(baseX) + ((i - startTime) * ((double(bottomRulerW)) / intervalW)));

        if (textX >= bottomRulerW) { break; }

        timeLabel << i;

        PangoLayout* text = gtk_widget_create_pango_layout(m_widget, timeLabel.str().c_str());

        int textW;
        pango_layout_get_pixel_size(text, &textW, nullptr);

        //if the width allocated per label becomes too small compared to the effective width of the label
        if (uint64_t(textW) >= m_pixelsPerLabel - 20) {
            //increases the allocated width per label
            m_pixelsPerLabel = textW + 30;
        }

        //display it
        gdk_draw_layout(m_widget->window, m_widget->style->fg_gc[GTK_WIDGET_STATE(m_widget)], textX, 4, text);

        //draw a small line above it
        gdk_draw_line(m_widget->window, m_widget->style->fg_gc[GTK_WIDGET_STATE(m_widget)], textX, 0, textX, 3);
    }
}

void CTimeRuler::LinkWidthToWidget(GtkWidget* widget)
{
    //add a callback to the widget for the size-allocate signal
    g_signal_connect(G_OBJECT(widget), "size-allocate", G_CALLBACK(TimeRulerResizeCB), this);
}
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
