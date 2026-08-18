///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
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

#include "defines.hpp"

#include "box-algorithms/CBoxAlgorithmStreamedMatrixSwitch.hpp"
#include "box-algorithms/CBoxInputStreamSwitch.hpp"

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Streaming {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, Box_FlagIsUnstable.toString(), Box_FlagIsUnstable.id());
    OVP_Declare_New(CBoxAlgorithmStreamedMatrixSwitchDesc)
    OVP_Declare_New(CBoxInputStreamSwitchDesc)
OVP_Declare_End()

}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
