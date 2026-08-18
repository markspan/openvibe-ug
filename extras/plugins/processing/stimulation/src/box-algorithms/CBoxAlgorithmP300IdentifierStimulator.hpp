///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmP300IdentifierStimulator.hpp
/// \author Baptiste Payan (Inria).
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

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {
class CBoxAlgorithmP300IdentifierStimulator final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return 128LL << 32; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;
    bool Reset();

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_P300IdentifierStimulator)

protected:
    enum class EStates { None, Flash, NoFlash, RepetitionRest, TrialRest, ExperimentStop };

    uint64_t m_startStimulation = 0;
    uint64_t m_stimulationBase  = 0;

    size_t m_nImages = 0;

    double m_percentRepetitionTarget = 0;

    size_t m_repetitionCountInTrial    = 0;
    size_t m_nTrial                    = 0;
    uint64_t m_flashDuration           = 0;
    uint64_t m_noFlashDuration         = 0;
    uint64_t m_interRepetitionDuration = 0;
    uint64_t m_interTrialDuration      = 0;

    // bool m_avoidNeighborFlashing = false;

    Kernel::IAlgorithmProxy* m_encoder       = nullptr;
    Kernel::IAlgorithmProxy* m_decoder       = nullptr;
    Kernel::IAlgorithmProxy* m_targetDecoder = nullptr;
    uint64_t m_lastTime                      = 0;
    bool m_headerSent                        = false;
    bool m_startReceived                     = false;

    EStates m_lastState       = EStates::None;
    uint64_t m_trialStartTime = 0;

    size_t m_nFlashInRepet                = 0;
    size_t m_nFlashInRepetWithoutTarget   = 0;
    uint64_t m_repetDuration              = 0;
    uint64_t m_repetDurationWithoutTarget = 0;
    uint64_t m_trialDuration              = 0;
    size_t m_trialIdx                     = 0;

    bool m_repetWithoutTarget = false; //true if the repetition doesn't contains the target

    uint64_t m_repetIdx = 0;
    int64_t m_targetNum = 0;

    uint64_t* m_repetTarget = nullptr; //for every Repetition of one trial indicate the number of target should see for the begin of the trial
    std::vector<size_t> m_images;

    void generateSequence();
    void generateTrialVars();
    int64_t getCurrentTimeInRepetition(uint64_t timeInTrial);
};

class CBoxAlgorithmP300IdentifierStimulatorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "P300 Identifier Stimulator"; }
    CString getAuthorName() const override { return "Baptiste Payan"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Generates a P300 stimulation sequence"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-select-font"; }

    CIdentifier getCreatedClass() const override { return Box_P300IdentifierStimulator; }
    IPluginObject* create() override { return new CBoxAlgorithmP300IdentifierStimulator; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Incoming stimulations", OV_TypeId_Stimulations);
        prototype.addOutput("Produced stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Target Stimulations", OV_TypeId_Stimulations);

        prototype.addSetting("Start stimulation", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Stimulation base", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");

        prototype.addSetting("Number of identifiable objects", OV_TypeId_Integer, "6");

        prototype.addSetting("Percent of repetitions containing the target", OV_TypeId_Float, "100");

        prototype.addSetting("Number of repetitions", OV_TypeId_Integer, "5");
        prototype.addSetting("Number of trials", OV_TypeId_Integer, "5");
        prototype.addSetting("Flash duration (in sec)", OV_TypeId_Float, "0.075");
        prototype.addSetting("No flash duration (in sec)", OV_TypeId_Float, "0.125");
        prototype.addSetting("Inter-repetition delay (in sec)", OV_TypeId_Float, "2");
        prototype.addSetting("Inter-trial delay (in sec)", OV_TypeId_Float, "5");

        // prototype.addSetting("Avoid neighbor flashing", OV_TypeId_Boolean, "false");

        prototype.addFlag(Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_P300IdentifierStimulatorDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
