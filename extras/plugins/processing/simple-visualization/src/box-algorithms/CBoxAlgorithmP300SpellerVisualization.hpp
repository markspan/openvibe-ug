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

#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

namespace TCPTagging {
class IStimulusSender; // fwd declare
}  // namespace TCPTagging

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class CP300SpellerState;
class CP300SpellerView;

class CBoxAlgorithmP300SpellerVisualization final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmP300SpellerVisualization();
    ~CBoxAlgorithmP300SpellerVisualization() override;

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    uint64_t getClockFrequency() override { return 128LL << 32; }   // To send stimuli soon after their frame
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_P300SpellerVisualization)

protected:
    struct SHeldStimulus
    {
        uint64_t stimulation;
        uint64_t update;    ///< Sent once this update of the view is on screen
        uint64_t queuedAt;
    };

    static constexpr double MAX_FRAME_WAIT_SECONDS = 1.0;

    void sendTag(uint64_t stimulation, uint64_t time);

    CString m_interfaceFilename;
    uint64_t m_rowStimulationBase    = 0;
    uint64_t m_columnStimulationBase = 0;

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

    std::unique_ptr<CP300SpellerState> m_state;
    std::unique_ptr<CP300SpellerView> m_view;
    uint64_t m_nRow                      = 0;
    uint64_t m_nCol                      = 0;

    int m_lastTargetRow = 0;
    int m_lastTargetCol = 0;
    int m_targetRow     = 0;
    int m_targetCol     = 0;
    int m_selectedRow   = 0;
    int m_selectedCol   = 0;

    // TCP Tagging: the stimuli of this round, then those waiting for their update to reach the screen
    std::vector<uint64_t> m_stimuliQueue;
    std::deque<SHeldStimulus> m_heldStimuli;
    uint64_t m_lastUpdate                         = 0;
    uint64_t m_lastTagTime                        = 0;
    bool m_warnedNoFrame                          = false;
    TCPTagging::IStimulusSender* m_stimulusSender = nullptr;
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
