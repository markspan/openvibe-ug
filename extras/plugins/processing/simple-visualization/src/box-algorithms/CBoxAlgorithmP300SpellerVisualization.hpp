///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmP300SpellerVisualization.hpp
/// \brief Classes for the Box P300 Speller Visualization.
/// \author Yann Renard (INRIA).
/// \version 1.0.
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
#include <visualization-toolkit/ovviz_all.h>

#include <gtk/gtk.h>
#include <list>
#include <map>
#include "../utils.hpp"

namespace TCPTagging {
class IStimulusSender; // fwd declare
}  // namespace TCPTagging

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class CBoxAlgorithmP300SpellerVisualization final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;
    void FlushQueue();                  // Sends all accumulated stimuli to the TCP Tagging

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_P300SpellerVisualization)

protected:
    using widget_style_t = struct
    {
        GtkWidget* widget;
        GtkWidget* childWidget;
        GdkColor bgColor;
        GdkColor fgColor;
        PangoFontDescription* fontDesc;
    };

    typedef void (CBoxAlgorithmP300SpellerVisualization::*cache_callback)(widget_style_t& style, void* data);

    void cacheBuildFromTable(GtkTable* table);
    void cacheForEach(cache_callback callback, void* data);
    void cacheForEachIf(int iLine, int iColumn, cache_callback ifCB, cache_callback elseCB, void* ifUserData, void* elseUserData);
    void cacheChangeNullCB(widget_style_t& style, void* data);
    void cacheChangeBackgroundCB(widget_style_t& style, void* data);
    void cacheChangeForegroundCB(widget_style_t& style, void* data);
    void cacheChangeFontCB(widget_style_t& style, void* data);
    void cacheCollectWidgetCB(widget_style_t& style, void* data);
    void cacheCollectChildWidgetCB(widget_style_t& style, void* data);

    CString m_interfaceFilename;
    uint64_t m_rowStimulationBase    = 0;
    uint64_t m_columnStimulationBase = 0;

    GdkColor m_flashBgColor                  = InitGDKColor(0, 6554, 6554, 6554);
    GdkColor m_flashFgColor                  = InitGDKColor(0, 65535, 65535, 65535);
    uint64_t m_flashFontSize                 = 100;
    PangoFontDescription* m_flashFontDesc    = nullptr;
    GdkColor m_noFlashBgColor                = InitGDKColor(0, 0, 0, 0);
    GdkColor m_noFlashFgColor                = InitGDKColor(0, 32768, 32768, 32768);
    uint64_t m_noFlashFontSize               = 75;
    PangoFontDescription* m_noFlashFontDesc  = nullptr;
    GdkColor m_targetBgColor                 = InitGDKColor(0, 6554, 26214, 6554);
    GdkColor m_targetFgColor                 = InitGDKColor(0, 39321, 65535, 39321);
    uint64_t m_targetFontSize                = 100;
    PangoFontDescription* m_targetFontDesc   = nullptr;
    GdkColor m_selectedBgColor               = InitGDKColor(0, 45875, 13107, 13107);
    GdkColor m_selectedFgColor               = InitGDKColor(0, 19661, 6554, 6554);
    uint64_t m_selectedFontSize              = 100;
    PangoFontDescription* m_selectedFontDesc = nullptr;

    Kernel::IAlgorithmProxy* m_sequenceStimulationDecoder        = nullptr;
    Kernel::IAlgorithmProxy* m_targetStimulationDecoder          = nullptr;
    Kernel::IAlgorithmProxy* m_targetFlaggingStimulationEncoder  = nullptr;
    Kernel::IAlgorithmProxy* m_rowSelectionStimulationDecoder    = nullptr;
    Kernel::IAlgorithmProxy* m_columnSelectionStimulationDecoder = nullptr;
    Kernel::TParameterHandler<const CMemoryBuffer*> m_sequenceMemoryBuffer;
    Kernel::TParameterHandler<const CMemoryBuffer*> m_targetMemoryBuffer;
    Kernel::TParameterHandler<const CStimulationSet*> m_targetFlaggingStimulationSet;
    Kernel::TParameterHandler<CStimulationSet*> m_sequenceStimulationSet;
    Kernel::TParameterHandler<CStimulationSet*> m_targetStimulationSet;
    Kernel::TParameterHandler<CMemoryBuffer*> m_targetFlaggingMemoryBuffer;
    uint64_t m_lastTime = 0;

    GtkBuilder* m_mainWidgetInterface    = nullptr;
    GtkBuilder* m_toolbarWidgetInterface = nullptr;
    GtkWidget* m_mainWindow              = nullptr;
    GtkWidget* m_toolbarWidget           = nullptr;
    GtkTable* m_table                    = nullptr;
    GtkLabel* m_result                   = nullptr;
    GtkLabel* m_target                   = nullptr;
    uint64_t m_nRow                      = 0;
    uint64_t m_nCol                      = 0;

    int m_lastTargetRow = 0;
    int m_lastTargetCol = 0;
    int m_targetRow     = 0;
    int m_targetCol     = 0;
    int m_selectedRow   = 0;
    int m_selectedCol   = 0;

    bool m_tableInitialized = false;

    // @todo refactor to std::pair<long,long> ?
    std::map<size_t, std::map<size_t, widget_style_t>> m_cache;
    std::list<std::pair<int, int>> m_targetHistory;

    // TCP Tagging
    std::vector<uint64_t> m_stimuliQueue;
    guint m_idleFuncTag                           = 0;
    TCPTagging::IStimulusSender* m_stimulusSender = nullptr;

    VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;
};

class CBoxAlgorithmP300SpellerVisualizationDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "P300 Speller Visualization"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Visualizes the alphabet for P300 spellers"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Visualization/Presentation"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-select-font"; }

    CIdentifier getCreatedClass() const override { return Box_P300SpellerVisualization; }
    IPluginObject* create() override { return new CBoxAlgorithmP300SpellerVisualization; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Sequence stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Target stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Row selection stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Column selection stimulations", OV_TypeId_Stimulations);

        prototype.addOutput("Target / Non target flagging (deprecated)", OV_TypeId_Stimulations);

        prototype.addSetting("Interface filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-speller.ui");
        prototype.addSetting("Row stimulation base", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addSetting("Column stimulation base", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_07");

        prototype.addSetting("Flash background color", OV_TypeId_Color, "10,10,10");
        prototype.addSetting("Flash foreground color", OV_TypeId_Color, "100,100,100");
        prototype.addSetting("Flash font size", OV_TypeId_Integer, "100");

        prototype.addSetting("No flash background color", OV_TypeId_Color, "0,0,0");
        prototype.addSetting("No flash foreground color", OV_TypeId_Color, "50,50,50");
        prototype.addSetting("No flash font size", OV_TypeId_Integer, "75");

        prototype.addSetting("Target background color", OV_TypeId_Color, "10,40,10");
        prototype.addSetting("Target foreground color", OV_TypeId_Color, "60,100,60");
        prototype.addSetting("Target font size", OV_TypeId_Integer, "100");

        prototype.addSetting("Selected background color", OV_TypeId_Color, "70,20,20");
        prototype.addSetting("Selected foreground color", OV_TypeId_Color, "30,10,10");
        prototype.addSetting("Selected font size", OV_TypeId_Integer, "100");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_P300SpellerVisualizationDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
