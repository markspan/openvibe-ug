///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \brief Main file for VRPN plugin, registering the boxes to OpenViBE
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

#include <openvibe/ov_all.h>

#include "box-algorithms/CVRPNAnalogServer.hpp"
#include "box-algorithms/CVRPNButtonServer.hpp"
#include "box-algorithms/CBoxAlgorithmVRPNAnalogClient.hpp"
#include "box-algorithms/CBoxAlgorithmVRPNButtonClient.hpp"

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace VRPN {

OVP_Declare_Begin()
    OVP_Declare_New(CVRPNAnalogServerDesc)
    OVP_Declare_New(CVRPNButtonServerDesc)
    OVP_Declare_New(CBoxAlgorithmVRPNAnalogClientDesc)
    OVP_Declare_New(CBoxAlgorithmVRPNButtonClientDesc)
OVP_Declare_End()

}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
