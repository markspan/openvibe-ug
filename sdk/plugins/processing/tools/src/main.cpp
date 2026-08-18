///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \brief Main file for Tools plugin, registering the boxes to OpenViBE
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

#include "box-algorithms/CBoxAlgorithmEBMLStreamSpy.hpp"
#include "box-algorithms/CBoxAlgorithmStimulationListener.hpp"
#include "box-algorithms/CBoxAlgorithmMatrixValidityChecker.hpp"
#include "box-algorithms/CBoxAlgorithmExternalProcessing.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Tools {

OVP_Declare_Begin()
    // ValidityCheckerType: this type registration enables the choice between different action to do on an invalid stream
    context.getTypeManager().registerEnumerationType(TypeId_ValidityCheckerType, "Action to do");
    context.getTypeManager().registerEnumerationEntry(TypeId_ValidityCheckerType, "Log warning", TypeId_ValidityCheckerType_LogWarning.id());
    context.getTypeManager().registerEnumerationEntry(TypeId_ValidityCheckerType, "Stop player", TypeId_ValidityCheckerType_StopPlayer.id());
    context.getTypeManager().registerEnumerationEntry(TypeId_ValidityCheckerType, "Interpolate", TypeId_ValidityCheckerType_Interpolate.id());

    OVP_Declare_New(CBoxAlgorithmStimulationListenerDesc)
    OVP_Declare_New(CBoxAlgorithmEBMLStreamSpyDesc)
    OVP_Declare_New(CBoxAlgorithmMatrixValidityCheckerDesc)
    OVP_Declare_New(CBoxAlgorithmExternalProcessingDesc)
OVP_Declare_End()

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
