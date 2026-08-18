///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSignalMerger.cpp
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

#include "CBoxAlgorithmSignalMerger.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Streaming {

bool CBoxAlgorithmSignalMerger::initialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();
    for (size_t i = 0; i < nInput; ++i) { m_decoders.push_back(new Toolkit::TSignalDecoder<CBoxAlgorithmSignalMerger>(*this, i)); }
    m_encoder = new Toolkit::TSignalEncoder<CBoxAlgorithmSignalMerger>(*this, 0);
    return true;
}

bool CBoxAlgorithmSignalMerger::uninitialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();

    m_encoder->uninitialize();
    delete m_encoder;

    for (size_t i = 0; i < nInput; ++i) {
        m_decoders[i]->uninitialize();
        delete m_decoders[i];
    }
    m_decoders.clear();

    return true;
}

bool CBoxAlgorithmSignalMerger::processInput(const size_t index)
{
    const IDynamicBoxContext& boxCtx = this->getDynamicBoxContext();
    const size_t nInput              = this->getStaticBoxContext().getInputCount();

    if (boxCtx.getInputChunkCount(0) == 0) { return true; }

    const uint64_t tStart = boxCtx.getInputChunkStartTime(0, 0);
    const uint64_t tEnd   = boxCtx.getInputChunkEndTime(0, 0);
    for (size_t i = 1; i < nInput; ++i) {
        if (boxCtx.getInputChunkCount(i) == 0) { return true; }

        OV_ERROR_UNLESS_KRF(tStart == boxCtx.getInputChunkStartTime(i, 0),
                            "Invalid start time [" << boxCtx.getInputChunkStartTime(i, 0) << "] on input [" << i
                            << "] (expected value must match start time on input 0 [" << tStart << "])",
                            Kernel::ErrorType::BadInput);

        OV_ERROR_UNLESS_KRF(tEnd == boxCtx.getInputChunkEndTime(i, 0),
                            "Invalid end time [" << boxCtx.getInputChunkEndTime(i, 0) << "] on input [" << i
                            << "] (expected value must match end time on input 0 [" << tEnd << "])",
                            Kernel::ErrorType::BadInput);
    }

    if (index == nInput - 1) {
        for (size_t i = 1; i < nInput; ++i) {
            OV_ERROR_UNLESS_KRF(boxCtx.getInputChunkCount(0) >= boxCtx.getInputChunkCount(i),
                                "Invalid input chunk count [" << boxCtx.getInputChunkCount(i) << "] on input [" << i
                                << "] (expected value must be <= to chunk count on input 0 [" << boxCtx.getInputChunkCount(0) << "])",
                                Kernel::ErrorType::BadInput);
        }
    }

    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmSignalMerger::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();

    size_t nChunk = boxContext.getInputChunkCount(0);

    for (size_t input = 1; input < nInput; ++input) { if (boxContext.getInputChunkCount(input) < nChunk) { nChunk = boxContext.getInputChunkCount(input); } }

    for (size_t c = 0; c < nChunk; ++c) {
        size_t nSamplePerBlock = 0;
        size_t nChannel        = 0;
        size_t nHeader         = 0;
        size_t nBuffer         = 0;
        size_t nEnd            = 0;

        for (size_t i = 0; i < nInput; ++i) {
            m_decoders[i]->decode(c);

            const CMatrix* oMatrix = m_decoders[i]->getOutputMatrix();
            if (m_decoders[i]->isHeaderReceived()) {
                nHeader++;
                if (i == 0) {
                    nSamplePerBlock = oMatrix->getDimensionSize(1);
                    nChannel        = oMatrix->getDimensionSize(0);
                }
                else {
                    // Check that properties agree
                    OV_ERROR_UNLESS_KRF(nSamplePerBlock == oMatrix->getDimensionSize(1),
                                        "Output matrix dimension [" << oMatrix->getDimensionSize(1) << "] on input [" << i
                                        << "] must match sample count per block [" << nSamplePerBlock << "]",
                                        Kernel::ErrorType::BadInput);

                    OV_ERROR_UNLESS_KRF(m_decoders[0]->getOutputSamplingRate() == m_decoders[i]->getOutputSamplingRate(),
                                        "Output sampling rate [" << m_decoders[i]->getOutputSamplingRate() << "] on input [" << i
                                        << "] must match the sampling rate on input 0 [" << m_decoders[0]->getOutputSamplingRate() << "]",
                                        Kernel::ErrorType::BadInput);

                    nChannel += oMatrix->getDimensionSize(0);
                }
            }
            if (m_decoders[i]->isBufferReceived()) { nBuffer++; }
            if (m_decoders[i]->isEndReceived()) { nEnd++; }
        }

        OV_ERROR_UNLESS_KRF(!nHeader || nHeader == nInput, "Received [" << nHeader << "] headers for [" << nInput << "] declared inputs", Kernel::ErrorType::BadInput);
        OV_ERROR_UNLESS_KRF(!nBuffer || nBuffer == nInput, "Received [" << nBuffer << "] buffers for [" << nInput << "] declared inputs", Kernel::ErrorType::BadInput);
        OV_ERROR_UNLESS_KRF(!nEnd || nEnd == nInput, "Received [" << nEnd << "] ends for [" << nInput << "] declared inputs", Kernel::ErrorType::BadInput);

        if (nHeader) {
            // We have received headers from all inputs
            CMatrix* iMatrix = m_encoder->getInputMatrix();

            iMatrix->resize(nChannel, nSamplePerBlock);
            for (size_t i = 0, k = 0; i < nInput; ++i) {
                const CMatrix* oMatrix = m_decoders[i]->getOutputMatrix();
                for (size_t j = 0; j < oMatrix->getDimensionSize(0); j++, k++) { iMatrix->setDimensionLabel(0, k, oMatrix->getDimensionLabel(0, j)); }
            }
            const uint64_t sampling           = m_decoders[0]->getOutputSamplingRate();
            m_encoder->getInputSamplingRate() = sampling;

            this->getLogManager() << Kernel::LogLevel_Debug << "Setting sampling rate to " << sampling << "\n";

            m_encoder->encodeHeader();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, c), boxContext.getInputChunkEndTime(0, c));
        }

        if (nBuffer) {
            // We have received one buffer from each input
            CMatrix* iMatrix = m_encoder->getInputMatrix();

            nSamplePerBlock = iMatrix->getDimensionSize(1);

            for (size_t i = 0, k = 0; i < nInput; ++i) {
                CMatrix* oMatrix = m_decoders[i]->getOutputMatrix();
                for (size_t j = 0; j < oMatrix->getDimensionSize(0); j++, k++) {
                    memcpy(iMatrix->getBuffer() + k * nSamplePerBlock, oMatrix->getBuffer() + j * nSamplePerBlock, nSamplePerBlock * sizeof(double));
                }
            }
            m_encoder->encodeBuffer();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, c), boxContext.getInputChunkEndTime(0, c));
        }

        if (nEnd) {
            // We have received one end from each input
            m_encoder->encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, c), boxContext.getInputChunkEndTime(0, c));
        }
    }

    return true;
}
}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
