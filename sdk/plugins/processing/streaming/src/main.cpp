///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \brief Main file for Streaming plugin, registering the boxes to OpenViBE
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

#include "box-algorithms/CBoxAlgorithmStreamedMatrixMultiplexer.hpp"
#include "box-algorithms/CBoxAlgorithmSignalMerger.hpp"

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Streaming {

OVP_Declare_Begin()
    OVP_Declare_New(CBoxAlgorithmStreamedMatrixMultiplexerDesc)
    OVP_Declare_New(CBoxAlgorithmSignalMergerDesc)
OVP_Declare_End()

}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
