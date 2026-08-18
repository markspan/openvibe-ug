///-------------------------------------------------------------------------------------------------
/// 
/// \file CCrashingBox.cpp
/// \author Yann Renard (Inria)
/// \version 1.0.
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

#include "CCrashingBox.hpp"
#include <cmath>    // For unix system

namespace OpenViBE {
namespace Plugins {
namespace Tests {

bool CCrashingBox::initialize(Kernel::IBoxAlgorithmContext& /*context*/) { throw 0; }

bool CCrashingBox::uninitialize(Kernel::IBoxAlgorithmContext& /*context*/)
{
    const int one  = int(1.0);
    const int zero = int(sin(0.0));
    const int div  = one / zero;
    return div ? true : false;
}

bool CCrashingBox::processInput(Kernel::IBoxAlgorithmContext& context, const size_t /*index*/)
{
    context.markAlgorithmAsReadyToProcess();
    return true;
}

bool CCrashingBox::process(Kernel::IBoxAlgorithmContext& /*context*/)
{
    *static_cast<int*>(nullptr) = 0;
    return true;
}
}  // namespace Tests
}  // namespace Plugins
}  // namespace OpenViBE
