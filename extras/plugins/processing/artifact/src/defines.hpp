///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \brief Defines list for Setting, Shortcut Macro and const.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 08/12/2020.
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
#define Box_Artifact_Amplitude                          OpenViBE::CIdentifier(0x41727469, 0xb68095e4)
#define Box_Artifact_Amplitude_Desc                     OpenViBE::CIdentifier(0x41727469, 0x83596875)
#define Box_ASR_Processor                               OpenViBE::CIdentifier(0x41727469, 0x17f1c6e2)
#define Box_ASR_Processor_Desc                          OpenViBE::CIdentifier(0x41727469, 0x1de22c87)
#define Box_ASR_Trainer                                 OpenViBE::CIdentifier(0x41727469, 0xc05f38ff)
#define Box_ASR_Trainer_Desc                            OpenViBE::CIdentifier(0x41727469, 0x966737cb)

#ifndef TypeId_Metric
#define TypeId_Metric                                   OpenViBE::CIdentifier(0x5261636B, 0x4D455452)
#endif

#ifndef TypeId_ArtifactAction
#define TypeId_ArtifactAction                           OpenViBE::CIdentifier(0x1a007224, 0xb1cfbb1d)
#endif
