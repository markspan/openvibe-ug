///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmEntropyMeasure.cpp
/// \brief Classes of the Box Entropy Measures.
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Mon Jan 29 10:00:00 2024.
/// \Copyright (C) 2024 Inria
/// 
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
/// 
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------


#include <iomanip>
#include "CBoxAlgorithmEntropyMeasure.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmEntropyMeasure::initialize() {
    m_timeWindow = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_timeInterval = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_m = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_r = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_scale = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    m_isMultivariate = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

    m_inputDecoder.initialize(*this, 0);
    m_outputMatrices.resize(m_scale);
    m_outputEncoders.resize(m_scale);

    for(int scale = 0; scale < m_scale; scale++) {
        m_outputMatrices[scale] = new CMatrix();

        m_outputEncoders[scale].initialize(*this, scale);
        m_outputEncoders[scale].getInputMatrix().setReferenceTarget(m_outputMatrices[scale]);
        m_outputEncoders[scale].getInputSamplingRate().setReferenceTarget(m_inputDecoder.getOutputSamplingRate());
    }

    m_entropyCalculator = new SampleEntropyCalculator(m_m, m_r, m_scale);

    std::cout << std::fixed;
    std::cout << std::setprecision(16);


    return true;
}

bool CBoxAlgorithmEntropyMeasure::uninitialize()
{
    m_inputDecoder.uninitialize();
    for(int scale = 0; scale < m_scale; scale++) {
        m_outputEncoders[scale].uninitialize();
    }
    return true;
}

bool CBoxAlgorithmEntropyMeasure::processInput(const size_t index)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}



bool CBoxAlgorithmEntropyMeasure::process()
{
    IDynamicBoxContext& dynamicBoxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < dynamicBoxContext.getInputChunkCount(0); ++i) {

        m_inputDecoder.decode(i);

        const uint64_t tStart = dynamicBoxContext.getInputChunkStartTime(0, i);
        const uint64_t tEnd   = dynamicBoxContext.getInputChunkEndTime(0, i);

        CMatrix* iMatrix = m_inputDecoder.getOutputMatrix();

        const size_t nbChannels = iMatrix->getDimensionSize(0);
        const size_t nbSamplesPerChannel = iMatrix->getDimensionSize(1);


        if (m_inputDecoder.isHeaderReceived()) {
            
            m_samplingRate = m_inputDecoder.getOutputSamplingRate();
            size_t maxChanSize = floor(m_samplingRate * m_timeWindow);
            size_t timeIntervalSize = floor(m_samplingRate * m_timeInterval);

            // initialize entropy calculator
            m_entropyCalculator->initialize(maxChanSize, timeIntervalSize, nbChannels);
            
            // Pass the header to the next boxes, by encoding a header:
            for(int scale = 0; scale < m_scale; scale++) {
                if(!m_isMultivariate) {
                    m_outputMatrices[scale]->resize(nbChannels,nbSamplesPerChannel);
                } else {
                    m_outputMatrices[scale]->resize(1, nbSamplesPerChannel);
                }
                m_outputEncoders[scale].encodeHeader();
            }
        }

        if (m_inputDecoder.isBufferReceived()) {        

            double* inputBuffer = iMatrix->getBuffer();
                
            if(!m_isMultivariate) {
                m_entropyCalculator->calculateUnivariate(inputBuffer, nbSamplesPerChannel, nbChannels, m_outputMatrices);
            } else {
                m_entropyCalculator-> calculateMultivariate(inputBuffer, nbSamplesPerChannel, nbChannels, m_outputMatrices);
            }

            for(int scale = 0; scale < m_scale; scale++) {
                m_outputEncoders[scale].encodeBuffer();
            }

        }

        if (m_inputDecoder.isEndReceived()) {
            for(int scale = 0; scale < m_scale; scale++)
                m_outputEncoders[scale].encodeEnd();
        }

        for(int scale = 0; scale < m_scale; scale++) {
            dynamicBoxContext.markOutputAsReadyToSend(scale, tStart, tEnd);   
        }
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE