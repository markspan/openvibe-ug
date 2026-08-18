///-------------------------------------------------------------------------------------------------
/// 
/// \file CBottomTimeRuler.cpp
/// \brief Implementation for the class CBottomTimeRuler.
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

#include "CBottomTimeRuler.hpp"
#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {


#define CONVERT_TIME(i) (double((i)>>32) + double(double((i)&0xFFFFFFFF) / double((uint64_t)1<<32)))

gboolean BottomRulerExposeEventCB(GtkWidget* /*widget*/, GdkEventExpose* /*event*/, gpointer data)
{
    //redraw the ruler
    auto* bottomRuler = reinterpret_cast<CBottomTimeRuler*>(data);
    bottomRuler->Draw();

    //don't propagate this signal to the children if any
    return TRUE;
}

gboolean ResizeBottomRulerCB(GtkWidget* /*widget*/, GtkAllocation* allocation, gpointer data)
{
    auto* bottomRuler = reinterpret_cast<CBottomTimeRuler*>(data);
    bottomRuler->OnResizeEventCB(allocation->width, allocation->height);
    return FALSE;
}

CBottomTimeRuler::CBottomTimeRuler(CBufferDatabase& database, const int width, const int height)
    : m_bottomRuler(gtk_drawing_area_new()), m_database(&database), m_height(height)
{
    gtk_widget_set_size_request(m_bottomRuler, width, height);
    g_signal_connect_after(G_OBJECT(m_bottomRuler), "expose-event", G_CALLBACK(BottomRulerExposeEventCB), this);
}

void CBottomTimeRuler::Draw()
{
    //if the widget is invisible, no need to redraw it
    if (!GTK_WIDGET_VISIBLE(m_bottomRuler)) { return; }

    //gets the number of buffers to display
    const uint64_t nBufferToDisplay = m_database->m_NBufferToDisplay;

    if (m_database->m_DimSizes[1] == 1 && nBufferToDisplay != 1) { /* nBufferToDisplay--;*/ }

    //gets the widget's size
    gint bottomRulerWidth;
    gint bottomRulerHeight;
    gdk_drawable_get_size(m_bottomRuler->window, &bottomRulerWidth, &bottomRulerHeight);

    //in ms
    const double intervalWidth = CONVERT_TIME(nBufferToDisplay * m_database->m_BufferDuration);
    //if(m_Database->areEpochsContiguous() == true){intervalWidth = CONVERT_TIME(nBufferToDisplay * m_Database->m_BufferDuration);}
    //else { intervalWidth = CONVERT_TIME(nBufferToDisplay * m_Database->m_BufferDuration); }

    //available width per buffer
    const double widthPerBuffer = double(bottomRulerWidth) / double(nBufferToDisplay);

    //computes the step of the values displayed on the ruler
    const auto nearestSmallerPowerOf10 = double(pow(10, floor(log10(intervalWidth))));
    const auto maxNumberOfLabels       = uint64_t(bottomRulerWidth / m_pixelsPerLabel);

    double valueStep = nearestSmallerPowerOf10;
    if (uint64_t(floor(intervalWidth / nearestSmallerPowerOf10)) > maxNumberOfLabels) { valueStep = 2 * nearestSmallerPowerOf10; }
    else if (uint64_t(floor(intervalWidth / nearestSmallerPowerOf10)) < maxNumberOfLabels / 2) { valueStep = nearestSmallerPowerOf10 / 2; }

    if (m_database->GetDisplayMode() == Scroll) {
        //compute start, end time and base value of the step
        double startTime = 0;
        if (!m_database->m_StartTime.empty()) { startTime = CONVERT_TIME(m_database->m_StartTime[0]); }

        const double endTime   = startTime + intervalWidth;
        const double baseValue = valueStep * floor(startTime / valueStep);

        //X position of the first label (if there are less buffers than needed)
        auto baseX = int64_t(floor(bottomRulerWidth - (double(m_database->m_SampleBuffers.size()) * widthPerBuffer)));
        if (baseX < 0) { baseX = 0; }

        //draw ruler base (horizontal line)
        gdk_draw_line(m_bottomRuler->window, m_bottomRuler->style->fg_gc[GTK_WIDGET_STATE(m_bottomRuler)], gint(baseX), 0, gint(bottomRulerWidth), 0);

        const int clipLeft  = 0;
        const int clipRight = bottomRulerWidth - 1;

        drawRuler(baseX, bottomRulerWidth, startTime, endTime, intervalWidth, baseValue, valueStep, clipLeft, clipRight);
    }
    else //scan mode
    {
        //draw ruler base (horizontal line)
        gdk_draw_line(m_bottomRuler->window, m_bottomRuler->style->fg_gc[GTK_WIDGET_STATE(m_bottomRuler)], 0, 0, gint(bottomRulerWidth), 0);

        //left part of the ruler (recent data)
        size_t leftmostBufferToDisplay = 0;
        m_database->GetIndexOfBufferStartingAtTime(m_leftmostDisplayedTime, leftmostBufferToDisplay);
        double startTime = 0;
        if (!m_database->m_StartTime.empty()) { startTime = CONVERT_TIME(m_leftmostDisplayedTime); }
        double endTime   = startTime + intervalWidth;
        double baseValue = valueStep * floor(startTime / valueStep);
        int clipLeft     = 0;
        int clipRight    = int(double(m_database->m_NBufferToDisplay - leftmostBufferToDisplay) * widthPerBuffer);

        drawRuler(0, bottomRulerWidth, startTime, endTime, intervalWidth, baseValue, valueStep, clipLeft, clipRight);

        //right part (older data)
        startTime -= intervalWidth;
        endTime   = startTime + intervalWidth;
        baseValue = valueStep * floor(startTime / valueStep);
        clipLeft  = clipRight + 1;
        clipRight = bottomRulerWidth - 1;

        drawRuler(0, bottomRulerWidth, startTime, endTime, intervalWidth, baseValue, valueStep, clipLeft, clipRight);
    }
}

void CBottomTimeRuler::OnResizeEventCB(const gint width, gint /*height*/) const { gtk_widget_set_size_request(m_bottomRuler, width, m_height); }

void CBottomTimeRuler::drawRuler(const int64_t baseX, const int rulerWidth, const double startTime, const double endTime, const double length,
                                 const double baseValue, const double valueStep, const int clipLeft, const int clipRight)
{
    for (double i = baseValue; i < double(0.5 + endTime); i += valueStep) {
        //compute the position of the label
        const gint textX = gint(double(baseX) + ((i - startTime) * ((double(rulerWidth)) / length)));
        //is text clipped?
        if (textX < clipLeft) { continue; }

        std::string timeLabel = std::to_string(i);
        PangoLayout* text     = gtk_widget_create_pango_layout(m_bottomRuler, timeLabel.c_str());

        int textWidth;
        pango_layout_get_pixel_size(text, &textWidth, nullptr);

        //is text beyond visible range?
        if (textX + textWidth > clipRight) {
            g_object_unref(text);
            break;
        }

        //if the width allocated per label becomes too small compared to the effective width of the label
        if (uint64_t(textWidth) >= m_pixelsPerLabel - 20) {
            //increases the allocated width per label
            m_pixelsPerLabel = textWidth + 30;
        }

        //display it
        gdk_draw_layout(m_bottomRuler->window, m_bottomRuler->style->fg_gc[GTK_WIDGET_STATE(m_bottomRuler)], textX, 4, text);

        //draw a small line above it
        gdk_draw_line(m_bottomRuler->window, m_bottomRuler->style->fg_gc[GTK_WIDGET_STATE(m_bottomRuler)], textX, 0, textX, 3);

        g_object_unref(text);
    }
}

void CBottomTimeRuler::LinkWidthToWidget(GtkWidget* widget)
{
    //adds a callback to the widget for the size-allocate signal
    g_signal_connect(G_OBJECT(widget), "size-allocate", G_CALLBACK(ResizeBottomRulerCB), this);
}
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
