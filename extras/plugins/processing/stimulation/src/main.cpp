///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
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

#include "defines.hpp"

#if defined TARGET_HAS_ThirdPartyLua
#include "box-algorithms/CBoxAlgorithmLuaStimulator.hpp"
#endif // TARGET_HAS_ThirdPartyLua
#if defined TARGET_HAS_ThirdPartyOpenAL
#include "box-algorithms/CBoxAlgorithmOpenALSoundPlayer.hpp"
#endif // TARGET_HAS_ThirdPartyOpenAL

#include "box-algorithms/adaptation/CBoxAlgorithmStimulationFilter.hpp"
#include "box-algorithms/CBoxAlgorithmP300IdentifierStimulator.hpp"
#include "box-algorithms/CBoxAlgorithmP300SpellerStimulator.hpp"
#include "box-algorithms/CBoxAlgorithmRunCommand.hpp"
#include "box-algorithms/CBoxAlgorithmThresholdCrossingDetector.hpp"
#include "box-algorithms/CKeyboardStimulator.hpp"
#include "box-algorithms/CBoxAlgorithmStimulationValidator.hpp"
#include "box-algorithms/CBoxStimulationConverter.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, Box_FlagIsUnstable.toString(), Box_FlagIsUnstable.id());

    context.getTypeManager().registerEnumerationType(TypeId_StimulationFilterAction, "Stimulation Filter Action");
    context.getTypeManager().registerEnumerationEntry(TypeId_StimulationFilterAction, "Select", TypeId_StimulationFilterAction_Select.id());
    context.getTypeManager().registerEnumerationEntry(TypeId_StimulationFilterAction, "Reject", TypeId_StimulationFilterAction_Reject.id());

    context.getTypeManager().registerEnumerationType(TypeId_Voting_ClearVotes, "Clear votes");
    context.getTypeManager().registerEnumerationEntry(TypeId_Voting_ClearVotes, "When expires", TypeId_Voting_ClearVotes_WhenExpires.id());
    context.getTypeManager().registerEnumerationEntry(TypeId_Voting_ClearVotes, "After output", TypeId_Voting_ClearVotes_AfterOutput.id());

    context.getTypeManager().registerEnumerationType(TypeId_Voting_OutputTime, "Output time");
    context.getTypeManager().registerEnumerationEntry(TypeId_Voting_OutputTime, "Time of voting", TypeId_Voting_OutputTime_Vote.id());
    context.getTypeManager().registerEnumerationEntry(TypeId_Voting_OutputTime, "Time of last winning stimulus", TypeId_Voting_OutputTime_Winner.id());
    context.getTypeManager().registerEnumerationEntry(TypeId_Voting_OutputTime, "Time of last voting stimulus", TypeId_Voting_OutputTime_Last.id());

    context.getTypeManager().registerEnumerationType(TypeId_Voting_RejectClass_CanWin, "Reject can win");
    context.getTypeManager().registerEnumerationEntry(TypeId_Voting_RejectClass_CanWin, "Yes", TypeId_Voting_RejectClass_CanWin_Yes.id());
    context.getTypeManager().registerEnumerationEntry(TypeId_Voting_RejectClass_CanWin, "No", TypeId_Voting_RejectClass_CanWin_No.id());

#if defined(TARGET_HAS_ThirdPartyGTK)
    OVP_Declare_New(CKeyboardStimulatorDesc)
#endif // TARGET_HAS_ThirdPartyGTK

#if defined TARGET_HAS_ThirdPartyLua
    OVP_Declare_New(CBoxAlgorithmLuaStimulatorDesc)
#endif // TARGET_HAS_ThirdPartyLua

#if defined TARGET_HAS_ThirdPartyOpenAL
    OVP_Declare_New(CBoxAlgorithmOpenALSoundPlayerDesc)
#endif // TARGET_HAS_ThirdPartyOpenAL

    OVP_Declare_New(CBoxAlgorithmThresholdCrossingDetectorDesc)
    OVP_Declare_New(CBoxAlgorithmRunCommandDesc)
    OVP_Declare_New(CBoxAlgorithmStimulationFilterDesc)
    OVP_Declare_New(CBoxAlgorithmP300SpellerStimulatorDesc)
    OVP_Declare_New(CBoxAlgorithmP300IdentifierStimulatorDesc)
    OVP_Declare_New(CBoxAlgorithmStimulationValidatorDesc)
    OVP_Declare_New(CBoxStimulationConverterDesc)

OVP_Declare_End()
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
