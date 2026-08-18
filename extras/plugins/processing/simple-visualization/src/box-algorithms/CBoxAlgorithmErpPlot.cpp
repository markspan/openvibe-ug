///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmErpPlot.cpp
/// \brief Classes implementation for the Box ERP plot.
/// \author Dieter Devlaminck (INRIA).
/// \version 1.1.
/// \date 16/11/2012
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

#include "CBoxAlgorithmErpPlot.h"
#include <visualization-toolkit/ovviz_all.h>
#include <cmath>
//#include <boost/lexical_cast.hpp>
//#include "../utils.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/*
static void EventHandler(const GtkWidget* widget, const gint width, const gint height, gpointer data)
{
    auto* graphs = reinterpret_cast<std::list<Graph*>*>(data);
    for (auto it = graphs->begin(); it != graphs->end(); ++it) {
        (*it)->ResizeAxis(width, height, graphs->size());
        (*it)->Draw(widget);
    }
}

static void OnConfigureEvent(GtkWidget* widget, const GdkEventConfigure* event, gpointer data)
{
    //std::cout << "OnConfigureEvent"<<event->width<<" "<< event->height<<"on widget "<<widget->allocation.width<<" "<<widget->allocation.height << "\n";
    gtk_widget_queue_draw_area(widget, 0, 0, event->width, event->height);
    EventHandler(widget, event->width, event->height, data);
}

static gboolean OnExposeEvent(const GtkWidget* widget, const GdkEventExpose* event, gpointer data)
{
    //std::cout << "OnExposeEvent" <<event->area.width<<" "<< event->area.height<<"on widget "<<widget->allocation.width<<" "<<widget->allocation.height<< "\n";
    //gtk_widget_queue_draw_area(widget,0, 0,event->area.width, event->area.height );
    EventHandler(widget, event->area.width, event->area.height, data);

    return FALSE;
}

static void RenderText(cairo_t* cr, const char* text, const double x, const double y)
{
    cairo_identity_matrix(cr);
    cairo_translate(cr, x, y);                              // set the origin of cairo instance 'cr' to (10,20) (i.e. this is where drawing will start from).
    PangoLayout* layout = pango_cairo_create_layout(cr);    // init pango layout ready for use
    pango_layout_set_text(layout, text, -1);                // sets the text to be associated with the layout (final arg is length, -1 to calculate automatically when passing a nul-terminated string)
    PangoFontDescription* desc = pango_font_description_from_string("Sans Bold 10");    // specify the font that would be ideal for your particular use
    pango_layout_set_font_description(layout, desc);        // assign the previous font description to the layout
    pango_font_description_free(desc);                      // free the description

    pango_cairo_update_layout(cr, layout);                  // if the target surface or transformation properties of the cairo instance have changed, update the pango layout to reflect this
    pango_cairo_show_layout(cr, layout);                    // draw the pango layout onto the cairo surface // mandatory

    g_object_unref(layout);                                 // free the layout
}

void Graph::ResizeAxis(const gint width, const gint height, const size_t nrOfGraphs)
{
    size_t nrOfRows          = size_t(ceil(sqrt(double(nrOfGraphs))));
    const size_t nrOfColumns = nrOfRows;
    if (nrOfGraphs <= (nrOfRows - 1) * nrOfRows) { nrOfRows--; }

    this->m_GraphWidth   = double(width) / double(nrOfColumns);
    this->m_GraphHeight  = double(height) / double(nrOfRows);
    this->m_GraphOriginX = this->m_GraphWidth * double(this->m_ColIdx);
    this->m_GraphOriginY = this->m_GraphHeight * double(this->m_RowIdx);

    //std::cout << "resizeAxis: origin x: " << m_GraphOriginX << ", origin y: " << m_GraphOriginY << ", width: " << m_GraphWidth << ", height: " << m_GraphHeight << "\n";
}

void Graph::Draw(const GtkWidget* widget) const //cairo_t * cairoContext)
{
    cairo_t* cairoContext = gdk_cairo_create(widget->window);

    cairo_set_line_width(cairoContext, 1);
    cairo_translate(cairoContext, m_GraphOriginX + 20, m_GraphOriginY + 20);
    cairo_scale(cairoContext, m_GraphWidth - 40, m_GraphHeight - 40);

    cairo_save(cairoContext);
    DrawAxis(cairoContext);
    cairo_restore(cairoContext);

    cairo_save(cairoContext);
    DrawLegend(cairoContext);
    cairo_restore(cairoContext);

    cairo_save(cairoContext);
    DrawVar(cairoContext);
    cairo_restore(cairoContext);

    cairo_save(cairoContext);
    DrawCurves(cairoContext);
    cairo_restore(cairoContext);

    cairo_save(cairoContext);
    DrawAxisLabels(cairoContext);
    cairo_restore(cairoContext);

    cairo_destroy(cairoContext);
}

void Graph::DrawAxis(cairo_t* ctx) const
{
    //make background white by drawing white rectangle
    cairo_set_source_rgb(ctx, 1.0, 1.0, 1.0);
    cairo_rectangle(ctx, 0, 0, 1, 1);
    cairo_fill(ctx);

    double ux = 1, uy = 1;
    cairo_device_to_user_distance(ctx, &ux, &uy);
    if (ux < uy) { ux = uy; }
    cairo_set_line_width(ctx, ux);

    cairo_set_source_rgb(ctx, 0, 0, 0);

    //cairo_save(cairoContext);
    //draw the horizontal line at zero if its inside the plotting region
    const double zeroLevel = AdjustValueToScale(0);
    double xo              = 0, yo   = zeroLevel,
           xe              = 1.0, ye = zeroLevel;
    if (std::fabs(zeroLevel) <= 1) { DrawLine(ctx, &xo, &yo, &xe, &ye); }

    // Draw y axis
    double dXo = 0, dYo = 0, dXe = 0, dYe = 1.0;
    DrawLine(ctx, &dXo, &dYo, &dXe, &dYe);
}

void Graph::DrawLine(cairo_t* ctx, double* xo, double* yo, double* xe, double* ye) const
{
    cairo_save(ctx);

    SnapCoords(ctx, xo, yo);
    SnapCoords(ctx, xe, ye);

    cairo_identity_matrix(ctx);
    cairo_set_line_width(ctx, 1.0);
    cairo_move_to(ctx, *xo, *yo);
    cairo_line_to(ctx, *xe, *ye);
    cairo_stroke(ctx);
    cairo_restore(ctx);
}

void Graph::SnapCoords(cairo_t* ctx, double* x, double* y) const
{
    cairo_user_to_device(ctx, x, y);
    *x = ceil(*x) + 0.5;
    *y = ceil(*y) + 0.5;
}

void Graph::DrawAxisLabels(cairo_t* ctx) const
{
    cairo_set_source_rgb(ctx, 0, 0, 0);

    // If we haven't received any data yet, bail out
    if (!(m_Minimum < DBL_MAX && m_Maximum > -DBL_MAX)) {
        cairo_move_to(ctx, 0, 1);

        double cx, cy;
        cairo_get_current_point(ctx, &cx, &cy);
        cairo_user_to_device(ctx, &cx, &cy);

        cairo_save(ctx);
        RenderText(ctx, "No data", cx, cy);
        cairo_restore(ctx);
        return;
    }

    // Note the scaling here should be compatible with adjustValueToScale.
    const double graphMin = m_Minimum - m_Variance[m_ArgMinimum.first][m_ArgMinimum.second],
                 graphMax = m_Maximum + m_Variance[m_ArgMaximum.first][m_ArgMaximum.second];


    // Including headroom may not be necessary as we are using adjustValueToScale to query where the Cairo drawing locations are, it'll take it into account
    // GraphMin = GraphMin-0.10f*std::fabs(GraphMin);
    // GraphMax = GraphMax+0.10f*std::fabs(GraphMax);

    const size_t numSteps  = 10;
    const double dataRange = graphMax - graphMin,
                 stepSize  = dataRange / numSteps;

    // Find a starting point in y so that stepping will pass through 0
    const double startY = floor(graphMin / stepSize) * stepSize;

    for (size_t i = 0; i <= numSteps; ++i) {
        const double valueAtTick = startY + double(i) * stepSize,
                     y           = AdjustValueToScale(valueAtTick);

        cairo_move_to(ctx, 0, y);

        double cx, cy;
        cairo_get_current_point(ctx, &cx, &cy);
        //std::cout<<"current point "<<cx<<" "<<cy<<"\n";
        cairo_user_to_device(ctx, &cx, &cy);
        //std::cout<<"device current point "<<cx<<" "<<cy<<"\n";

        std::stringstream ss;
        ss.precision(2);
        ss << valueAtTick;

        cairo_save(ctx);
        RenderText(ctx, ss.str().c_str(), cx, cy);
        cairo_restore(ctx);
    }

    const uint64_t xBegin = this->m_StartTime, xEnd = this->m_EndTime;


    for (double x = 0; x <= 1; x += 0.2) {
        cairo_move_to(ctx, x, 1);

        double cx, cy;
        cairo_get_current_point(ctx, &cx, &cy);
        //std::cout<<"current point "<<cx<<" "<<cy<<"\n";
        cairo_user_to_device(ctx, &cx, &cy);


        //X value to print range from XBegin to XEnd
        const double dataLengthSecs = CTime(xEnd - xBegin).toSeconds(),
                     dataStart      = CTime(xBegin).toSeconds();

        std::stringstream ss;
        ss.precision(2);
        ss << (dataLengthSecs * x + dataStart);

        cairo_save(ctx);
        RenderText(ctx, ss.str().c_str(), cx, cy);
        cairo_restore(ctx);
    }
}

void Graph::DrawCurves(cairo_t* ctx) const
{
    double ux = 1, uy = 1;
    cairo_device_to_user_distance(ctx, &ux, &uy);
    if (ux < uy) { ux = uy; }
    cairo_set_line_width(ctx, ux);

    for (size_t gi = 0; gi < m_Curves.size(); ++gi) {
        cairo_set_source_rgb(ctx, double(m_LineColor[gi].red) / 65535.0, double(m_LineColor[gi].green) / 65535.0, double(m_LineColor[gi].blue) / 65535.0);
        const std::vector<double>& curve = m_Curves[gi];

        //center
        double y = AdjustValueToScale(curve[0]),
               x = 0.0;
        cairo_move_to(ctx, x, y);

        for (int si = 1; si < m_CurveSize; ++si) {
            y = AdjustValueToScale(curve[si]);
            x = (double(si)) / (double(m_CurveSize));
            cairo_line_to(ctx, x, y);
        }

        cairo_save(ctx);
        cairo_identity_matrix(ctx);
        cairo_set_line_width(ctx, 1.0);
        cairo_stroke(ctx);
        cairo_restore(ctx);
    }
}

void Graph::DrawVar(cairo_t* ctx) const
{
    double ux = 1, uy = 1;
    cairo_device_to_user_distance(ctx, &ux, &uy);
    if (ux < uy) { ux = uy; }
    cairo_set_line_width(ctx, ux);

    for (size_t gi = 0; gi < m_Curves.size(); ++gi) {
        cairo_set_source_rgba(ctx, double(m_LineColor[gi].red) / 65535.0, double(m_LineColor[gi].green) / 65535.0, double(m_LineColor[gi].blue) / 65535.0, 0.5);
        const std::vector<double>& curves    = m_Curves[gi];
        const std::vector<double>& variances = m_Variance[gi];
        // Test first if we have any variance at all, if not, don't bother drawing as cairo slows down with tiny apertures
        if (std::none_of(variances.begin(), variances.end(), [](const double a) { return a > 0; })) { continue; }

        double var = variances[0];

        double y = AdjustValueToScale(curves[0] - var);
        double x = 0.0;
        cairo_move_to(ctx, x, y);

        // Draw variance below the data points
        for (int si = 1; si < m_CurveSize; ++si) {
            var = variances[si];

            y = AdjustValueToScale(curves[si] - var);
            x = (double(si)) / (double(m_CurveSize));
            cairo_line_to(ctx, x, y);
        }

        // Draw the last point separately
        cairo_line_to(ctx, x, y + 2 * var);

        // Draw variance above the points, including the first point
        for (int si = m_CurveSize - 1; si >= 0; si--) {
            var = variances[si];
            y   = AdjustValueToScale(curves[si] + var);
            x   = (double(si)) / (double(m_CurveSize));
            cairo_line_to(ctx, x, y);
        }

        // Fill the surrounded region?
        cairo_fill(ctx);
    }
}

void Graph::DrawLegend(cairo_t* ctx) const
{
    double ux = 1, uy = 1;
    cairo_device_to_user_distance(ctx, &ux, &uy);
    cairo_select_font_face(ctx, "Sans Bold 10", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    cairo_set_font_size(ctx, 0.02);

    double yTotal = 0;
    for (size_t gi = 0; gi < m_Curves.size(); ++gi) {
        cairo_set_source_rgb(ctx, double(m_LineColor[gi].red) / 65535.0, double(m_LineColor[gi].green) / 65535.0, double(m_LineColor[gi].blue) / 65535.0);

        cairo_text_extents_t extents;
        cairo_text_extents(ctx, m_LineText[gi].toASCIIString(), &extents);

        yTotal += extents.height + 0.02;

        // size_t len = m_LineText[gi].length();
        cairo_move_to(ctx, (1.0 - extents.width - 0.01), yTotal);

        double cx, cy;
        cairo_get_current_point(ctx, &cx, &cy);
        //std::cout<<"current point "<<cx<<" "<<cy<<"\n";
        cairo_user_to_device(ctx, &cx, &cy);
        //std::cout<<m_LineText[gi]<<"\n";
        cairo_save(ctx);
        cairo_show_text(ctx, m_LineText[gi].toASCIIString());
        cairo_restore(ctx);
    }
}



double Graph::AdjustValueToScale(const double value) const
{
    //std::cout<<m_pVariance[m_ArgMinimum.first][m_ArgMinimum.second] <<" "<<m_pVariance[m_ArgMaximum.first][m_ArgMaximum.second]<<"\n";
    double graphMin = m_Minimum - m_Variance[m_ArgMinimum.first][m_ArgMinimum.second];
    double graphMax = m_Maximum + m_Variance[m_ArgMaximum.first][m_ArgMaximum.second];

    graphMin = graphMin - 0.10 * std::fabs(graphMin);
    graphMax = graphMax + 0.10 * std::fabs(graphMax);
    return (graphMin - value) / (graphMax - graphMin) + 1.0;
}

void Graph::UpdateCurves(const double* curve, const size_t howMany, const size_t curveIndex)
{
    m_Curves[curveIndex].assign(curve, curve + howMany);

    m_Maximum = -DBL_MAX;
    m_Minimum = DBL_MAX;
    for (int j = 0; j < int(m_Curves.size()); ++j) {
        for (int i = 0; i < m_CurveSize; ++i) {
            //m_Maximum = m_lCurves[j][i]>m_Maximum ? m_lCurves[j][i]:m_Maximum;
            if (m_Curves[j][i] > m_Maximum) {
                m_Maximum           = m_Curves[j][i];
                m_ArgMaximum.first  = j;
                m_ArgMaximum.second = i;
            }
            //m_Minimum = m_lCurves[j][i]<m_Minimum ? m_lCurves[j][i]:m_Minimum;
            if (m_Curves[j][i] < m_Minimum) {
                m_Minimum           = m_Curves[j][i];
                m_ArgMinimum.first  = j;
                m_ArgMinimum.second = i;
            }
        }
        //m_Maximum = m_Maximum+0.01f*std::fabs(m_Maximum);
        //m_Minimum = m_Minimum-0.01f*std::fabs(m_Minimum);
    }
}
*/

bool CBoxAlgorithmErpPlot::initialize()
{
    // If you need to retrieve setting values, use the FSettingValueAutoCast function.
    const Kernel::IBox& boxContext = this->getStaticBoxContext();
    m_figureFileName               = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_triggerToSave                = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_xStartsAt0                   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    //should be a Graph per channel/electrode not per input (should be done when first header is received)
    for (size_t i = 1; i < boxContext.getInputCount(); ++i) {
        if ((i % 2) == 1) {
            const size_t inputParamsStartAt = 3;
            const size_t c                  = i / 2;
            CString color;
            
            boxContext.getSettingValue(inputParamsStartAt + 2 * c, color);
            color = this->getConfigurationManager().expand(color);
            std::stringstream ss(color.toASCIIString());
            int r = 0, g = 0, b = 0;
            char z;
            ss >> r >> z >> g >> z >> b;
            std::string std_color = std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b);
            m_legendColors.push_back(std_color);
            
            m_legend.push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), inputParamsStartAt + 2 * c + 1));
            m_decoders.push_back(new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmErpPlot>(*this, i));
        }
        else { 
            m_varianceDecoders.push_back(new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmErpPlot>(*this, i));
        }
    }

    m_stimulationDecoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmErpPlot>(*this, 0);

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_drawWindow = new CGraphWindow(this->getLogManager(), visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine());
    
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_drawWindow->visualItem());
    } else {
        getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
    }

    m_firstHeaderReceived = false;

    return true;
}

bool CBoxAlgorithmErpPlot::uninitialize()
{
    for (size_t i = 0; i < m_decoders.size(); ++i) {
        m_decoders[i]->uninitialize();
        m_varianceDecoders[i]->uninitialize();
    }

    m_stimulationDecoder->uninitialize();

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }
    
    if (m_drawWindow) {
        delete m_drawWindow;
        m_drawWindow = nullptr;
    }
    m_graphList.clear();

    return true;
}

bool CBoxAlgorithmErpPlot::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

//saving the graph in png images
bool CBoxAlgorithmErpPlot::Save()
{
    /*
    cairo_t* cairoContext = gdk_cairo_create(m_drawWindow->window);

    //the main surface
    cairo_surface_t* surface = cairo_get_target(cairoContext);

    //building filename
    const std::string extension = ".png";


    for (auto it = m_graphList->begin(); it != m_graphList->end(); ++it) {
        //cutting this graph
        cairo_surface_t* subsurface = cairo_surface_create_for_rectangle(surface, (*it)->m_GraphOriginX, (*it)->m_GraphOriginY, (*it)->m_GraphWidth,
                                                                         (*it)->m_GraphHeight);

        std::stringstream filename;
        //creating filename
        filename << m_figureFileName << ((*it)->m_RowIdx) << "_" << ((*it)->m_ColIdx) << extension;
        this->getLogManager() << Kernel::LogLevel_Info << "Saving [" << filename.str() << "] \n";
        cairo_surface_write_to_png(subsurface, filename.str().c_str());
    }
*/
    return true;
}

bool CBoxAlgorithmErpPlot::process()
{
    Kernel::IBoxIO& dynamicBoxContext    = this->getDynamicBoxContext();
    const Kernel::IBox& staticBoxContext = this->getStaticBoxContext();

    //listen for stimulation input
    for (size_t i = 0; i < dynamicBoxContext.getInputChunkCount(0); ++i) {
        m_stimulationDecoder->decode(i);
        if (m_stimulationDecoder->isBufferReceived()) {
            const CStimulationSet* stimSet = m_stimulationDecoder->getOutputStimulationSet();
            for (size_t j = 0; j < stimSet->size(); ++j) {
                if (stimSet->getId(j) == m_triggerToSave) {
                    this->getLogManager() << Kernel::LogLevel_Trace << "Saving\n";
                    Save();
                }
            }
        }
    }

    for (size_t inputi = 1; inputi < staticBoxContext.getInputCount(); ++inputi) {
        for (size_t i = 0; i < dynamicBoxContext.getInputChunkCount(inputi); ++i) {
            if ((inputi) % 2 == 1) {
                m_decoders[inputi / 2]->decode(i);

                if (m_decoders[inputi / 2]->isHeaderReceived() && !m_firstHeaderReceived) {
                    const size_t nElectrodes = m_decoders[inputi / 2]->getOutputMatrix()->getDimensionSize(0);
                    const auto nCols         = size_t(ceil(sqrt(double(nElectrodes))));

                    //create list of graph subplots
                    for (size_t dimi = 0; dimi < nElectrodes; ++dimi) {
                        auto new_graph = m_drawWindow->createGraph(uint(floor(float(dimi) / float(nCols))), uint(dimi % nCols));
                        std::vector<std::string> labels;
                        for(auto& it : m_legend) {
                            labels.push_back(it.toASCIIString());
                        }
                        new_graph->initCurves(m_legendColors, labels);
                        m_graphList.push_back(new_graph);
                    }

                    m_firstHeaderReceived = true;
                }
                if (m_decoders[inputi / 2]->isBufferReceived()) {
                    const uint64_t startTime = dynamicBoxContext.getInputChunkStartTime(inputi, i), endTime = dynamicBoxContext.getInputChunkEndTime(inputi, i);

                    //redraw all
                    CMatrix* matrix          = m_decoders[inputi / 2]->getOutputMatrix();
                    const size_t nElectrodes = matrix->getDimensionSize(0), nSamples = matrix->getDimensionSize(1);

                    auto it = m_graphList.begin();
                    for (size_t dimi = 0; dimi < nElectrodes; dimi++, ++it) {
                        const double* ptr = matrix->getBuffer() + dimi * nSamples;
                        uint64_t new_startTime = (m_xStartsAt0 ? 0 : startTime);
                        uint64_t new_endTime   = (m_xStartsAt0 ? (endTime - startTime) : endTime);
                        (*it)->updateTimes(new_startTime, new_endTime);
                        (*it)->updateCurves(ptr, nSamples, inputi / 2);
                    }

                    dynamicBoxContext.markInputAsDeprecated(inputi, i);
                }
            }
            else {
                //std::cout<<" variance input"<<(inputi/2-1)<<"\n";
                m_varianceDecoders[inputi / 2 - 1]->decode(i);


                if (m_varianceDecoders[inputi / 2 - 1]->isBufferReceived()) {
                    CMatrix* matrix = m_varianceDecoders[inputi / 2 - 1]->getOutputMatrix();

                    const size_t nSamples = matrix->getDimensionSize(1), nElectrodes = matrix->getDimensionSize(0);
                    auto it = m_graphList.begin();
                    for (size_t dimi = 0; dimi < nElectrodes; dimi++, ++it) {
                        const double* ptr = matrix->getBuffer() + dimi * nSamples;
                        (*it)->updateVariance(ptr, nSamples, inputi / 2 - 1);
                    }

                    dynamicBoxContext.markInputAsDeprecated(inputi, i);
                }
            }
        }
    }

    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
