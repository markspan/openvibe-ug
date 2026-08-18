///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \brief Defines of Straming related identifiers.
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
#define Box_SignalMerger                                OpenViBE::CIdentifier(0x4BF9326F, 0x75603102)
#define Box_SignalMergerDesc                            OpenViBE::CIdentifier(0x7A684C44, 0x23BA70A5)
#define Box_StreamedMatrixMultiplexer                   OpenViBE::CIdentifier(0x7A12298B, 0x785F4D42)
#define Box_StreamedMatrixMultiplexerDesc               OpenViBE::CIdentifier(0x0B420425, 0x3F602DE7)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
