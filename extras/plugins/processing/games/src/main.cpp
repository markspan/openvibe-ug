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

#include <openvibe/ov_all.h>

// Boxes Includes
#include "boxes/CBoxHelloWorldGame.hpp"
#include "boxes/CBoxHelloSenderGame.hpp"
#include "boxes/CBoxHelloBidirectionnalGame.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Games {

OVP_Declare_Begin()
#ifdef TARGET_HAS_ThirdPartyLSL
    // Register boxes
    OVP_Declare_New(CBoxHelloWorldGameDesc)
    OVP_Declare_New(CBoxHelloSenderGameDesc)
    OVP_Declare_New(CBoxHelloBidirectionnalGameDesc)
#endif // TARGET_HAS_ThirdPartyLSL
OVP_Declare_End()

}  // namespace Games
}  // namespace Plugins
}  // namespace OpenViBE
