///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmErpPlot.hpp
/// \brief Classes for the Box ERP plot.
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

#pragma once

#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include "CGraphWindow.h"

#include <cfloat>
#include <iostream>
#include <list>
#include <string>
#include <utility>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/*
class Graph
{
public:
    //should be a list of colors;
    Graph(std::vector<std::string>& lineColor, std::vector<CString>& lineText, const int rowIndex, const int colIndex, const int curveSize)
        : m_LineColor(lineColor), m_LineText(lineText)
    {
        m_Curves.resize(lineColor.size());
        m_Variance.resize(lineColor.size());
        for (size_t i = 0; i < m_Curves.size(); ++i) {
            m_Curves[i].resize(curveSize, 0);
            m_Variance[i].resize(curveSize, 0);
        }
        this->m_RowIdx    = rowIndex;
        this->m_ColIdx    = colIndex;
        this->m_CurveSize = curveSize;
        m_PointCounter    = new int[lineColor.size()];
        m_Maximum         = - DBL_MAX;
        m_Minimum         = DBL_MAX;
        for (size_t i = 0; i < lineColor.size(); ++i) { m_PointCounter[i] = 0; }
    }

    ~Graph()
    {
        delete [] m_PointCounter;
        m_Curves.clear();
        m_Variance.clear();
    }

    void ResizeAxis(gint width, gint height, size_t nrOfGraphs);
    void Draw(const GtkWidget* widget) const;
    void DrawAxis(cairo_t* ctx) const;
    void DrawLine(cairo_t* ctx, double* xo, double* yo, double* xe, double* ye) const;
    void DrawAxisLabels(cairo_t* ctx) const;
    void DrawCurves(cairo_t* ctx) const;
    void DrawLegend(cairo_t* ctx) const;
    void DrawVar(cairo_t* ctx) const;
    void SnapCoords(cairo_t* ctx, double* x, double* y) const;

    double AdjustValueToScale(double value) const;
    void UpdateCurves(const double* curve, size_t howMany, size_t curveIndex);

    std::vector<std::vector<double>> m_Curves; //private
    std::vector<std::vector<double>> m_Variance;

    std::vector<std::string>& m_LineColor; //private
    std::vector<CString>& m_LineText; //private
    double m_Maximum = 1;
    double m_Minimum = -1;
    std::pair<int, int> m_ArgMaximum;
    std::pair<int, int> m_ArgMinimum;
    double m_GraphWidth   = 0;
    double m_GraphHeight  = 0;
    double m_GraphOriginX = 0;
    double m_GraphOriginY = 0;
    int m_RowIdx          = 0; //private
    int m_ColIdx          = 0; //private 
    int m_CurveSize       = 0; //private
    int* m_PointCounter   = nullptr;
    uint64_t m_StartTime  = 0;
    uint64_t m_EndTime    = 0;
    double m_FontSize     = 1.0;
};
*/

///<summary> The class CBoxAlgorithmErpPlot describes the box ERP plot. </summary>
class CBoxAlgorithmErpPlot final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;
    bool Save();
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ErpPlot)

protected:
    CString m_figureFileName;
    std::vector<std::string> m_legendColors;
    std::vector<CString> m_legend;
    CGraphWindow *m_drawWindow = nullptr;
    std::list<std::shared_ptr<Graph> > m_graphList;
    bool m_firstHeaderReceived     = false;
    std::vector<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmErpPlot>*> m_decoders;
    std::vector<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmErpPlot>*> m_varianceDecoders;
    Toolkit::TStimulationDecoder<CBoxAlgorithmErpPlot>* m_stimulationDecoder = nullptr;
    uint64_t m_triggerToSave                                                 = 0;
    bool m_xStartsAt0                                                        = false;

};

///<summary> Listener of the box ERP plot. </summary>
class CBoxAlgorithmErpPlotListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputType(index, OV_TypeId_StreamedMatrix);
        const size_t c        = index / 2 + 1;
        const std::string idx = std::to_string(c), iLabel = "ERP ", colorLabel = "Line color ", textLabel = "Line label ", varianceLabel = "Variance ";

        box.setInputName(index, (iLabel + idx).c_str());
        box.addSetting((colorLabel + idx).c_str(),OV_TypeId_Color, "0,0,0");
        box.addSetting((textLabel + idx).c_str(),OV_TypeId_String, "curve");
        //add the corresponding variance input
        box.addInput((varianceLabel + idx).c_str(), OV_TypeId_StreamedMatrix);

        return true;
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(index * 2 + 2);
        box.removeSetting(index * 2 + 1);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};


///<summary> Descriptor of the box ERP plot. </summary>
class CBoxAlgorithmErpPlotDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "ERP plot"; }
    CString getAuthorName() const override { return "Dieter Devlaminck"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Plots event-related potentials"; }
    CString getDetailedDescription() const override { return "plots target ERP versus non-target ERP"; }
    CString getCategory() const override { return "Visualization/Presentation"; }
    CString getVersion() const override { return "1.1"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return Box_ErpPlot; }
    IPluginObject* create() override { return new CBoxAlgorithmErpPlot; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmErpPlotListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Trigger",OV_TypeId_Stimulations);
        prototype.addInput("ERP1",OV_TypeId_StreamedMatrix);
        prototype.addInput("Variance1",OV_TypeId_StreamedMatrix);

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        //prototype.addInputSupport(OV_TypeId_StreamedMatrix);

        prototype.addSetting("Filename final figure",OV_TypeId_Filename, "");
        prototype.addSetting("Trigger to save figure",OV_TypeId_Stimulation, "OVTK_StimulationId_ExperimentStop");
        prototype.addSetting("X starts at 0",OV_TypeId_Boolean, "true");
        prototype.addSetting("Line color 1",OV_TypeId_Color, "0,0,0");
        prototype.addSetting("Line label 1",OV_TypeId_String, "curve 1");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ErpPlotDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
