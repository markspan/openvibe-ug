///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmP300MagicCardVisualization.hpp
/// \brief Classes for the Box P300 Magic Card Visualization.
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
#include <map>
#include "../utils.hpp"

namespace TCPTagging {
class IStimulusSender;  // fwd declare
}  // namespace TCPTagging

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class CBoxAlgorithmP300MagicCardVisualization final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    void FlushQueue();                  // Sends all accumulated stimuli to the TCP Tagging
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_P300MagicCardVisualization)

protected:
    using widget_style_t = struct
    {
        int index;
        GdkColor bgColor;
        GtkWidget* parent;
        GtkWidget* widget;
        GtkWidget* image;
    };

    typedef void (CBoxAlgorithmP300MagicCardVisualization::*cache_callback)(widget_style_t& style, void* data);

    void cacheBuildFromTable(GtkTable* table);
    void cacheForEach(cache_callback callback, void* data);
    void cacheForEachIf(int card, cache_callback ifCB, cache_callback elseCB, void* ifUserData, void* elseUserData);
    void cacheChangeNullCB(widget_style_t& style, void* data);
    void cacheChangeImageCB(widget_style_t& style, void* data);
    void cacheChangeBackgroundCB(widget_style_t& style, void* data);

    CString m_interfaceFilename;
    uint64_t m_cardStimulationBase = 0;

    Kernel::IAlgorithmProxy* m_sequenceStimulationDecoder       = nullptr;
    Kernel::IAlgorithmProxy* m_targetStimulationDecoder         = nullptr;
    Kernel::IAlgorithmProxy* m_targetFlaggingStimulationEncoder = nullptr;
    Kernel::IAlgorithmProxy* m_cardSelectionStimulationDecoder  = nullptr;
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
    //GtkLabel* m_target = nullptr;
    GdkColor m_bgColor         = InitGDKColor(0, 58982, 58982, 58982);
    GdkColor m_targetBgColor   = InitGDKColor(0, 6554, 26214, 6554);
    GdkColor m_selectedBgColor = InitGDKColor(0, 45875, 13107, 13107);
    uint64_t m_nTableRow       = 0;
    uint64_t m_nTableCol       = 0;
    uint64_t m_nCard           = 0;

    int m_targetCard = 0;

    std::vector<GtkWidget*> m_fgImage;
    std::vector<GtkWidget*> m_bgImage;

    bool m_tableInitialized = false;

    std::map<size_t, widget_style_t> m_caches;

    VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;

    // TCP Tagging
    std::vector<uint64_t> m_stimuliQueue;
    guint m_idleFuncTag                           = 0;
    TCPTagging::IStimulusSender* m_stimulusSender = nullptr;
};

class CBoxAlgorithmP300MagicCardVisualizationDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "P300 Magic Card Visualization"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Presents a matrix of images to the user in various ways"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Visualization/Presentation"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-select-font"; }

    CIdentifier getCreatedClass() const override { return Box_P300MagicCardVisualization; }
    IPluginObject* create() override { return new CBoxAlgorithmP300MagicCardVisualization; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Sequence stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Target stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Card selection stimulations", OV_TypeId_Stimulations);

        prototype.addOutput("Target / Non target flagging (deprecated)", OV_TypeId_Stimulations);

        prototype.addSetting("Interface filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card.ui");
        prototype.addSetting("Background color", OV_TypeId_Color, "90,90,90");
        prototype.addSetting("Target background color", OV_TypeId_Color, "10,40,10");
        prototype.addSetting("Selected background color", OV_TypeId_Color, "70,20,20");
        prototype.addSetting("Card stimulation base", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addSetting("TCP Tagging Host address", OV_TypeId_String, "localhost");
        prototype.addSetting("TCP Tagging Host port", OV_TypeId_Integer, "15361");
        prototype.addSetting("Default background filename", OV_TypeId_Filename,
                             "${Path_Data}/plugins/simple-visualization/p300-magic-card/openvibe-logo.png-offscreen");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/01.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/02.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/03.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/04.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/05.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/06.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/07.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/08.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/09.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/10.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/11.png");
        prototype.addSetting("Card filename", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/12.png");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_P300MagicCardVisualizationDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
