///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStreamedMatrixMultiplexer.cpp
/// \brief Classes implementation for the Box Signal Merger.
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

#include "CBoxAlgorithmStreamedMatrixMultiplexer.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Streaming {

bool CBoxAlgorithmStreamedMatrixMultiplexer::initialize()
{
    m_lastStartTime = 0;
    m_lastEndTime   = 0;
    m_headerSent    = false;

    return true;
}

bool CBoxAlgorithmStreamedMatrixMultiplexer::uninitialize() { return true; }

bool CBoxAlgorithmStreamedMatrixMultiplexer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmStreamedMatrixMultiplexer::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i) {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            const CMemoryBuffer* iBuffer = boxContext.getInputChunk(i, j);
            const uint64_t tStart        = boxContext.getInputChunkStartTime(i, j);
            const uint64_t tEnd          = boxContext.getInputChunkEndTime(i, j);

            if ((!m_headerSent && tStart == tEnd) || (m_headerSent && tStart != tEnd)) {
                CMemoryBuffer* oBuffer = boxContext.getOutputChunk(0);
                oBuffer->setSize(iBuffer->getSize(), true);

                memcpy(oBuffer->getDirectPointer(), iBuffer->getDirectPointer(), iBuffer->getSize());

                OV_ERROR_UNLESS_KRF(tStart >= m_lastStartTime && tEnd >= m_lastEndTime,
                                    "Invalid chunk times with start = [" << tStart << "] and end = [" << tEnd << "] while last chunk has start = ["
                                    << m_lastStartTime << "] and end = [" << m_lastEndTime << "]", Kernel::ErrorType::BadInput);

                m_lastStartTime = tStart;
                m_lastEndTime   = tEnd;

                boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
                m_headerSent = true;
            }

            boxContext.markInputAsDeprecated(i, j);
        }
    }

    return true;
}
}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
