///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \brief Defines list for Setting, Shortcut Macro and const.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 22/11/2021.
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
#define Box_LSLCommunication                OpenViBE::CIdentifier(0xc07934f5, 0x9390e102)
#define Box_LSLCommunication_Desc           OpenViBE::CIdentifier(0xc07934f5, 0x9390e100)
#define Box_LSLExport                       OpenViBE::CIdentifier(0x6F3467FF, 0x52794DA6)
#define Box_LSLExportDesc                   OpenViBE::CIdentifier(0x40C03C3F, 0x034A19C2)
#define Box_SharedMemoryWriter              OpenViBE::CIdentifier(0xACC272DD, 0xC1BDC1B1)
#define Box_SharedMemoryWriterDesc          OpenViBE::CIdentifier(0xACC727DD, 0xCACDC1B1)
#define Box_TCPWriter                       OpenViBE::CIdentifier(0x02F24947, 0x17FA0477)
#define Box_TCPWriterDesc                   OpenViBE::CIdentifier(0x3C32489D, 0x46F565D3)

//---------------------------------------------------------------------------------------------------
#define TypeID_TCPWriter_OutputStyle        OpenViBE::CIdentifier(0x6D7E53DD, 0x6A0A4753)
#define TypeID_TCPWriter_RawOutputStyle     OpenViBE::CIdentifier(0x77D3E238, 0xB954EC48)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#define OV_AttributeId_Box_FlagIsUnstable   OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)
