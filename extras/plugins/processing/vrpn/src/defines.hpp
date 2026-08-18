///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \brief Defines of VRPN related identifiers.
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
#define Box_VRPNAnalogServer                            OpenViBE::CIdentifier(0x0DDC3A7E, 0x6F6E6401)
#define Box_VRPNAnalogServerDesc                        OpenViBE::CIdentifier(0xF54B8E03, 0xAAFF15C6)
#define Box_VRPNButtonServer                            OpenViBE::CIdentifier(0x0E382E6F, 0x5BE1F00C)
#define Box_VRPNButtonServerDesc                        OpenViBE::CIdentifier(0xBC86F256, 0x002495EF)
#define Box_VRPNAnalogClient                            OpenViBE::CIdentifier(0x7CF4A95E, 0x7270D07B)
#define Box_VRPNAnalogClientDesc                        OpenViBE::CIdentifier(0x77B2AE79, 0xFDC31871)
#define Box_VRPNButtonClient                            OpenViBE::CIdentifier(0x40714327, 0x458877D2)
#define Box_VRPNButtonClientDesc                        OpenViBE::CIdentifier(0x16FB6283, 0x45EC313F)


// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
