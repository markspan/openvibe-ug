#include "defines.hpp"

#include "box-algorithms/CBoxAlgorithmClockStimulator.hpp"
#include "box-algorithms/CBoxAlgorithmPlayerController.hpp"
#include "box-algorithms/CBoxAlgorithmStimulationVoter.hpp"
#include "box-algorithms/CBoxAlgorithmStreamEndDetector.hpp"
#include "box-algorithms/CBoxAlgorithmTimeout.hpp"
#include "box-algorithms/CBoxStimulationMultiplexer.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationType(PlayerAction, "Player Action");
    context.getTypeManager().registerEnumerationEntry(PlayerAction, "Play", PlayerAction_Play.id());
    context.getTypeManager().registerEnumerationEntry(PlayerAction, "Stop", PlayerAction_Stop.id());
    context.getTypeManager().registerEnumerationEntry(PlayerAction, "Pause", PlayerAction_Pause.id());
    context.getTypeManager().registerEnumerationEntry(PlayerAction, "Forward", PlayerAction_Forward.id());

    context.getTypeManager().registerEnumerationType(Voting_ClearVotes, "Clear votes");
    context.getTypeManager().registerEnumerationEntry(Voting_ClearVotes, "When expires", Voting_ClearVotes_WhenExpires.id());
    context.getTypeManager().registerEnumerationEntry(Voting_ClearVotes, "After output", Voting_ClearVotes_AfterOutput.id());
    context.getTypeManager().registerEnumerationType(Voting_OutputTime, "Output time");
    context.getTypeManager().registerEnumerationEntry(Voting_OutputTime, "Time of voting", Voting_OutputTime_Vote.id());
    context.getTypeManager().registerEnumerationEntry(Voting_OutputTime, "Time of last winning stimulus", Voting_OutputTime_Winner.id());
    context.getTypeManager().registerEnumerationEntry(Voting_OutputTime, "Time of last voting stimulus", Voting_OutputTime_Last.id());
    context.getTypeManager().registerEnumerationType(Voting_RejectClass_CanWin, "Reject can win");
    context.getTypeManager().registerEnumerationEntry(Voting_RejectClass_CanWin, "Yes", Voting_RejectClass_CanWin_Yes.id());
    context.getTypeManager().registerEnumerationEntry(Voting_RejectClass_CanWin, "No", Voting_RejectClass_CanWin_No.id());

    OVP_Declare_New(CBoxAlgorithmClockStimulatorDesc)
    OVP_Declare_New(CBoxAlgorithmPlayerControllerDesc)
    OVP_Declare_New(CBoxStimulationMultiplexerDesc)
    OVP_Declare_New(CBoxAlgorithmStreamEndDetectorDesc)
    OVP_Declare_New(CBoxAlgorithmTimeoutDesc)
    OVP_Declare_New(CBoxAlgorithmStimulationVoterDesc)

OVP_Declare_End()

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
