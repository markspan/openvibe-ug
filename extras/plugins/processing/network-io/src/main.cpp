///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \brief main file for box plugin.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/03/2020.
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

#include "defines.hpp"

#include "box-algorithms/CBoxTCPWriter.hpp"
#include "box-algorithms/CBoxSharedMemoryWriter.hpp"
#include "box-algorithms/CBoxLSLExport.hpp"
#include "box-algorithms/CBoxLSLCommunication.hpp"

namespace OpenViBE {
namespace Plugins {

OVP_Declare_Begin()
#ifdef TARGET_HAS_ThirdPartyLSL
    OVP_Declare_New(NetworkIO::CBoxLSLExportDesc)
    OVP_Declare_New(NetworkIO::CBoxLSLCommunicationDesc)
#endif

    OVP_Declare_New(FileReadingAndWriting::CBoxSharedMemoryWriterDesc)
    OVP_Declare_New(NetworkIO::CBoxTCPWriterDesc)

    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());

    context.getTypeManager().registerEnumerationType(TypeID_TCPWriter_OutputStyle, "Stimulus output");
    context.getTypeManager().registerEnumerationEntry(TypeID_TCPWriter_OutputStyle, "Raw", TCPWRITER_RAW);
    context.getTypeManager().registerEnumerationEntry(TypeID_TCPWriter_OutputStyle, "Hex", TCPWRITER_HEX);
    context.getTypeManager().registerEnumerationEntry(TypeID_TCPWriter_OutputStyle, "String", TCPWRITER_STRING);

    context.getTypeManager().registerEnumerationType(TypeID_TCPWriter_RawOutputStyle, "Raw output");
    context.getTypeManager().registerEnumerationEntry(TypeID_TCPWriter_RawOutputStyle, "Raw", TCPWRITER_RAW);

OVP_Declare_End()

}  // namespace Plugins
}  // namespace OpenViBE
