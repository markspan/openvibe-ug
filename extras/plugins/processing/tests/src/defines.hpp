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
#define Box_AlgorithmAddition                           OpenViBE::CIdentifier(0x75FCE50E, 0x8302FA91)
#define Box_AlgorithmAdditionDesc                       OpenViBE::CIdentifier(0x842E0B85, 0xA59FABC1)
#define Box_BoxAlgorithmAdditionTest                    OpenViBE::CIdentifier(0x534EB140, 0x15F41496)
#define Box_BoxAlgorithmAdditionTestDesc                OpenViBE::CIdentifier(0xB33EC315, 0xF63BC0C5)
#define Box_CrashingBox                                 OpenViBE::CIdentifier(0x00DAFD60, 0x39A58819)
#define Box_CrashingBoxDesc                             OpenViBE::CIdentifier(0x009F54B9, 0x2B6A4922)
#define Box_TestCodecToolkit                            OpenViBE::CIdentifier(0x330E3A87, 0x31565BA6)
#define Box_TestCodecToolkitDesc                        OpenViBE::CIdentifier(0x376A4712, 0x1AA65567)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
