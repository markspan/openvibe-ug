///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \author Laurent Bonnet (Inria)
/// \date 12/05/2011.
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
#define Box_StreamedMatrixSwitch                        OpenViBE::CIdentifier(0x556A2C32, 0x61DF49FC)
#define Box_StreamedMatrixSwitchDesc                    OpenViBE::CIdentifier(0x556A2C32, 0x61DF49FC)
#define Box_InputMatrixSwitch                           OpenViBE::CIdentifier(0x5261636b, 0x49534d53)
#define Box_InputMatrixSwitchDesc                       OpenViBE::CIdentifier(0x5261636b, 0x49534d44)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define Box_FlagIsUnstable                              OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)
