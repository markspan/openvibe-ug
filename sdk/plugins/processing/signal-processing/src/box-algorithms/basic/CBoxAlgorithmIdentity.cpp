///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmIdentity.cpp
/// \brief Classes implementation for the Box Identity.
/// \author Yann Renard (Inria).
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

#include "CBoxAlgorithmIdentity.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

void CBoxAlgorithmIdentity::release() { delete this; }

bool CBoxAlgorithmIdentity::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmIdentity::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();
    const size_t nInput        = getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount();
    uint64_t tStart            = 0;
    uint64_t tEnd              = 0;
    size_t size                = 0;
    const uint8_t* buffer      = nullptr;

    for (size_t i = 0; i < nInput; ++i) {
        for (size_t j = 0; j < boxContext->getInputChunkCount(i); ++j) {
            boxContext->getInputChunk(i, j, tStart, tEnd, size, buffer);
            boxContext->appendOutputChunkData(i, buffer, size);
            boxContext->markOutputAsReadyToSend(i, tStart, tEnd);
            boxContext->markInputAsDeprecated(i, j);
        }
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
