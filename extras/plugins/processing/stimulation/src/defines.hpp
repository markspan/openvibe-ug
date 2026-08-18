///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
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

// Boxes
//---------------------------------------------------------------------------------------------------
#define Box_StimulationMultiplexer                      OpenViBE::CIdentifier(0x07DB4EFA, 0x472B0938)
#define Box_StimulationMultiplexerDesc                  OpenViBE::CIdentifier(0x79EF4E4D, 0x178F09E6)
#define Box_SoundPlayer                                 OpenViBE::CIdentifier(0x18D06E9F, 0x68D43C23)
#define Box_SoundPlayerDesc                             OpenViBE::CIdentifier(0x246E5EC4, 0x127D21AA)
#define Box_StimulationVoter                            OpenViBE::CIdentifier(0x2BBD61FC, 0x041A4EDB)
#define Box_StimulationVoterDesc                        OpenViBE::CIdentifier(0x1C36287C, 0x6F143FBF)
#define Box_KeyboardStimulator                          OpenViBE::CIdentifier(0x00D317B9, 0x6324C3FF)
#define Box_KeyboardStimulatorDesc                      OpenViBE::CIdentifier(0x00E51ACD, 0x284CA2CF)
#define Box_P300IdentifierStimulator                    OpenViBE::CIdentifier(0x00F27FDB, 0x8203D1A5)
#define Box_P300IdentifierStimulatorDesc                OpenViBE::CIdentifier(0x000F20CA, 0x2A4EA9C3)
#define Box_ThresholdCrossingDetector                   OpenViBE::CIdentifier(0x04FA78CD, 0xAFE45DE7)
#define Box_ThresholdCrossingDetectorDesc               OpenViBE::CIdentifier(0x798ACD86, 0xEF1287A4)
#define Box_StimulationFilter                           OpenViBE::CIdentifier(0x02F96101, 0x5E647CB8)
#define Box_StimulationFilterDesc                       OpenViBE::CIdentifier(0x4D2A23FC, 0x28191E18)
#define Box_LuaStimulator                               OpenViBE::CIdentifier(0x0B5A2787, 0x02750621)
#define Box_LuaStimulatorDesc                           OpenViBE::CIdentifier(0x67AF36F3, 0x2B424F46)
#define Box_OpenALSoundPlayer                           OpenViBE::CIdentifier(0x7AC2396F, 0x7EE52EFE)
#define Box_OpenALSoundPlayerDesc                       OpenViBE::CIdentifier(0x6FD040EF, 0x7E2F1284)
#define Box_P300SpellerStimulator                       OpenViBE::CIdentifier(0x88857F9A, 0xF560D3EB)
#define Box_P300SpellerStimulatorDesc                   OpenViBE::CIdentifier(0xCEAFBB05, 0x5DA19DCB)
#define Box_RunCommand                                  OpenViBE::CIdentifier(0x48843891, 0x7BFC57F4)
#define Box_RunCommandDesc                              OpenViBE::CIdentifier(0x29D449AE, 0x2CA94942)
#define Box_StimulationValidator                        OpenViBE::CIdentifier(0x393d15e9, 0x5b6f63b9)
#define Box_StimulationValidatorDesc                    OpenViBE::CIdentifier(0x553bdcca, 0xe66d09bf)
#define Box_StimulationConverter                        OpenViBE::CIdentifier(0x5261636b, 0x53744376)
#define Box_StimulationConverterDesc                    OpenViBE::CIdentifier(0x5261636b, 0x53744364)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define Box_FlagIsUnstable                              OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)

#define TypeId_Voting_ClearVotes                        OpenViBE::CIdentifier(0x17AE30F8, 0x40B57661)
#define TypeId_Voting_ClearVotes_AfterOutput            OpenViBE::CIdentifier(0x7FA81A20, 0x484023F9)
#define TypeId_Voting_ClearVotes_WhenExpires            OpenViBE::CIdentifier(0x02766639, 0x00B155B4)
#define TypeId_Voting_OutputTime                        OpenViBE::CIdentifier(0x48583E8F, 0x47F22462)
#define TypeId_Voting_OutputTime_Vote                   OpenViBE::CIdentifier(0x2F37507F, 0x00C06761)
#define TypeId_Voting_OutputTime_Winner                 OpenViBE::CIdentifier(0x72416689, 0x17673658)
#define TypeId_Voting_OutputTime_Last                   OpenViBE::CIdentifier(0x4F2830DB, 0x716C2930)
#define TypeId_Voting_RejectClass_CanWin                OpenViBE::CIdentifier(0x442F2F14, 0x7A17336C)
#define TypeId_Voting_RejectClass_CanWin_Yes            OpenViBE::CIdentifier(0x40011974, 0x54BB3C71)
#define TypeId_Voting_RejectClass_CanWin_No             OpenViBE::CIdentifier(0x275B746A, 0x480B302C)


#define TypeId_StimulationFilterAction                  OpenViBE::CIdentifier(0x09E59E57, 0x8D4A553A)
#define TypeId_StimulationFilterAction_Select           OpenViBE::CIdentifier(0xBDBBA98D, 0xC0477399)
#define TypeId_StimulationFilterAction_Reject           OpenViBE::CIdentifier(0xB7C594D2, 0x32474226)
