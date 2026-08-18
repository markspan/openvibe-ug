///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \brief Defines of Tools related identifiers.
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
#define Box_EBMLStreamSpy                               OpenViBE::CIdentifier(0x0ED76695, 0x01A69CC3)
#define Box_EBMLStreamSpyDesc                           OpenViBE::CIdentifier(0x354A6864, 0x06BC570C)
#define Box_ExternalProcessing                          OpenViBE::CIdentifier(0x15422959, 0x16304449)
#define Box_ExternalProcessingDesc                      OpenViBE::CIdentifier(0x63386942, 0x61D42502)
#define Box_MatrixValidityChecker                       OpenViBE::CIdentifier(0x60210579, 0x6F7519B6)
#define Box_MatrixValidityCheckerDesc                   OpenViBE::CIdentifier(0x6AFC2671, 0x1D8C493C)
#define Box_StimulationListener                         OpenViBE::CIdentifier(0x65731E1D, 0x47DE5276)
#define Box_StimulationListenerDesc                     OpenViBE::CIdentifier(0x0EC013FD, 0x5DD23E44)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define TypeId_ValidityCheckerType                      OpenViBE::CIdentifier(0x32EA493A, 0x11E56D82)
#define TypeId_ValidityCheckerType_LogWarning           OpenViBE::CIdentifier(0x747A0F84, 0x1097253A)
#define TypeId_ValidityCheckerType_StopPlayer           OpenViBE::CIdentifier(0x4EC06D50, 0x5B131CE2)
#define TypeId_ValidityCheckerType_Interpolate          OpenViBE::CIdentifier(0x1DE96E02, 0x53767550)
