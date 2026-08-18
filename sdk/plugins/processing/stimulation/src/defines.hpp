///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \brief Defines of Stimulations related identifiers.
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
#define Box_ClockStimulator                             OpenViBE::CIdentifier(0x4F756D3F, 0x29FF0B96)
#define Box_ClockStimulatorDesc                         OpenViBE::CIdentifier(0x4FD067E9, 0x740D2AF0)
#define Box_PlayerController                            OpenViBE::CIdentifier(0x5F426DCE, 0x08456E13)
#define Box_PlayerControllerDesc                        OpenViBE::CIdentifier(0x2E4142D7, 0x3F957E25)
#define Box_StimulationMultiplexer                      OpenViBE::CIdentifier(0x07DB4EFA, 0x472B0938)
#define Box_StimulationMultiplexerDesc                  OpenViBE::CIdentifier(0x79EF4E4D, 0x178F09E6)
#define Box_StimulationVoter                            OpenViBE::CIdentifier(0x2BBD61FC, 0x041A4EDB)
#define Box_StimulationVoterDesc                        OpenViBE::CIdentifier(0x1C36287C, 0x6F143FBF)
#define Box_StreamEndDetector                           OpenViBE::CIdentifier(0x44F2725A, 0x8E922233)
#define Box_StreamEndDetectorDesc                       OpenViBE::CIdentifier(0x6DD8B6EA, 0xC581B3FC)
#define Box_Timeout                                     OpenViBE::CIdentifier(0x24FCD292, 0x5C8F6AA8)
#define Box_TimeoutDesc                                 OpenViBE::CIdentifier(0xB3D90844, 0x79A4CF6D)

// Type definitions
//---------------------------------------------------------------------------------------------------
#define Voting_ClearVotes                               OpenViBE::CIdentifier(0x17AE30F8, 0x40B57661)
#define Voting_ClearVotes_AfterOutput                   OpenViBE::CIdentifier(0x7FA81A20, 0x484023F9)
#define Voting_ClearVotes_WhenExpires                   OpenViBE::CIdentifier(0x02766639, 0x00B155B4)
#define Voting_OutputTime                               OpenViBE::CIdentifier(0x48583E8F, 0x47F22462)
#define Voting_OutputTime_Vote                          OpenViBE::CIdentifier(0x2F37507F, 0x00C06761)
#define Voting_OutputTime_Winner                        OpenViBE::CIdentifier(0x72416689, 0x17673658)
#define Voting_OutputTime_Last                          OpenViBE::CIdentifier(0x4F2830DB, 0x716C2930)
#define Voting_RejectClass_CanWin                       OpenViBE::CIdentifier(0x442F2F14, 0x7A17336C)
#define Voting_RejectClass_CanWin_Yes                   OpenViBE::CIdentifier(0x40011974, 0x54BB3C71)
#define Voting_RejectClass_CanWin_No                    OpenViBE::CIdentifier(0x275B746A, 0x480B302C)

#define PlayerAction                                    OpenViBE::CIdentifier(0xCC14D8D6, 0xF27ECB73)
#define PlayerAction_Play                               OpenViBE::CIdentifier(0x6BD7E1B3, 0x85E75066)
#define PlayerAction_Stop                               OpenViBE::CIdentifier(0x8A72885E, 0xD537A9A4)
#define PlayerAction_Pause                              OpenViBE::CIdentifier(0xD2FA4573, 0xE7F1D11D)
#define PlayerAction_Forward                            OpenViBE::CIdentifier(0xE43E9888, 0x33A361ED)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
