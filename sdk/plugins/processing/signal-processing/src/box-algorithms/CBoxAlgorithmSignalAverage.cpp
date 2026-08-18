///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSignalAverage.cpp
/// \brief Classes implementation for the Box Signal average.
/// \author Bruno Renier (Inria).
/// \version 0.5.
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

#include "CBoxAlgorithmSignalAverage.hpp"

#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

void CBoxAlgorithmSignalAverage::computeAverage()
{
    const double* input = m_decoder.getOutputMatrix()->getBuffer();
    double* output      = m_encoder.getInputMatrix()->getBuffer();

    const size_t nChannel = m_decoder.getOutputMatrix()->getDimensionSize(0);
    const size_t nSample  = m_decoder.getOutputMatrix()->getDimensionSize(1);

    //computes and stores the average for each channel
    for (size_t c = 0; c < nChannel; ++c) {
        double sum = 0;
        for (size_t i = 0; i < nSample; ++i) { sum += input[(c * nSample) + i]; }
        output[c] = sum / double(nSample);
    }
}


bool CBoxAlgorithmSignalAverage::initialize()
{
    m_decoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);
    return true;
}

bool CBoxAlgorithmSignalAverage::uninitialize()
{
    m_encoder.uninitialize();
    m_decoder.uninitialize();
    return true;
}

bool CBoxAlgorithmSignalAverage::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmSignalAverage::process()
{
    const IDynamicBoxContext* boxCtx = getBoxAlgorithmContext()->getDynamicBoxContext();

    // Process input data
    for (size_t i = 0; i < boxCtx->getInputChunkCount(0); ++i) {
        m_decoder.decode(i);

        if (m_decoder.isHeaderReceived()) {
            // Construct the properties of the output stream
            const CMatrix* iMatrix = m_decoder.getOutputMatrix();
            CMatrix* oMatrix       = m_encoder.getInputMatrix();

            // Sampling rate will be decimated in the output
            const uint64_t iSampling   = m_decoder.getOutputSamplingRate();
            const size_t iSampleCount  = iMatrix->getDimensionSize(1);
            const uint64_t newSampling = uint64_t(ceil(double(iSampling) / double(iSampleCount)));

            m_encoder.getInputSamplingRate() = newSampling;

            // We keep the number of channels, but the output chunk size will be 1
            oMatrix->resize(iMatrix->getDimensionSize(0), 1);

            for (size_t j = 0; j < oMatrix->getDimensionSize(0); ++j) { oMatrix->setDimensionLabel(0, j, iMatrix->getDimensionLabel(0, j)); }

            m_encoder.encodeHeader();
            getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);
        }

        if (m_decoder.isBufferReceived()) {
            const uint64_t tStart = boxCtx->getInputChunkStartTime(0, i);
            const uint64_t tEnd   = boxCtx->getInputChunkEndTime(0, i);

            computeAverage();

            m_encoder.encodeBuffer();
            getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, tStart, tEnd);
        }
        // if (m_decoder.isEndReceived()) { }   // NOP
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
