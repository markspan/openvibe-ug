///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCommonAverageReference.cpp
/// \brief Classes implementation for the Box Common Average Reference.
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

#include "CBoxAlgorithmCommonAverageReference.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmCommonAverageReference::initialize()
{
    m_decoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
    m_decoder->initialize();

    ip_buffer.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));
    op_matrix.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Matrix));
    op_sampling.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));

    m_encoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_encoder->initialize();

    ip_matrix.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Matrix));
    ip_sampling.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling));
    op_buffer.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));

    op_matrix = &m_oMatrix;
    ip_matrix = &m_oMatrix;
    ip_sampling.setReferenceTarget(op_sampling);

    return true;
}

bool CBoxAlgorithmCommonAverageReference::uninitialize()
{
    op_buffer.uninitialize();
    ip_sampling.uninitialize();
    ip_matrix.uninitialize();
    m_encoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_encoder);

    op_sampling.uninitialize();
    op_matrix.uninitialize();
    ip_buffer.uninitialize();
    m_decoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_decoder);

    return true;
}

bool CBoxAlgorithmCommonAverageReference::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmCommonAverageReference::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        ip_buffer = boxContext.getInputChunk(0, i);
        op_buffer = boxContext.getOutputChunk(0);

        m_decoder->process();
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader)) {
            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader);
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer)) {
            const size_t nChannel = m_oMatrix.getDimensionSize(0), nSample = m_oMatrix.getDimensionSize(1);
            for (size_t j = 0; j < nSample; ++j) {
                double* buffer = m_oMatrix.getBuffer() + j;
                double sum     = 0;
                for (size_t c = nChannel; c != 0; c--) {
                    sum += *buffer;
                    buffer += nSample;
                }
                const double mean = sum / double(nChannel);
                for (size_t c = nChannel; c != 0; c--) {
                    buffer -= nSample;
                    *buffer -= mean;
                }
            }

            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer);
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedEnd)) {
            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeEnd);
        }

        boxContext.markInputAsDeprecated(0, i);
        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
