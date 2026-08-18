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
#define Box_LatencyEvaluation                           OpenViBE::CIdentifier(0x0AD11EC1, 0x7EF3690B)
#define Box_LatencyEvaluationDesc                       OpenViBE::CIdentifier(0x5DB56A54, 0x5380262B)
#define Box_MouseTracking                               OpenViBE::CIdentifier(0x1E386EE5, 0x203E13C6)
#define Box_MouseTrackingDesc                           OpenViBE::CIdentifier(0x7A31C11B, 0xF522262E)
#define Box_KeypressEmulator                            OpenViBE::CIdentifier(0x38503532, 0x19494145)
#define Box_KeypressEmulatorDesc                        OpenViBE::CIdentifier(0x59286224, 0x99423852)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define Box_FlagIsUnstable                              OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)

#define TypeId_Keypress_Key                             OpenViBE::CIdentifier(0x23E525F9, 0x005F187D)
#define TypeId_Keypress_Modifier                        OpenViBE::CIdentifier(0x0C4F4585, 0x78B246AC)
#define TypeId_Keypress_Modifier_None                   OpenViBE::CIdentifier(0x5A70497E, 0x52F57199)
#define TypeId_Keypress_Modifier_Shift                  OpenViBE::CIdentifier(0x47534AA6, 0x138A1130)
#define TypeId_Keypress_Modifier_Control                OpenViBE::CIdentifier(0x67C6455F, 0x08E6134D)
#define TypeId_Keypress_Modifier_Alt                    OpenViBE::CIdentifier(0x77577AEA, 0x74CD7E3E)
