///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSignalDecimation.cpp
/// \brief Classes implementation for the Box Signal Decimation.
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmSignalDecimation.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmSignalDecimation::initialize()
{
    m_decoder = nullptr;
    m_encoder = nullptr;

    m_decimationFactor = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    OV_ERROR_UNLESS_KRF(m_decimationFactor > 1, "this box's setting \"Decimation factor\" is " << m_decimationFactor << ", while it must be above 1. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    m_decoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
    m_decoder->initialize();

    ip_buffer.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));
    op_pMatrix.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Matrix));
    op_sampling.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));

    m_encoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_encoder->initialize();

    ip_sampling.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling));
    ip_pMatrix.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Matrix));
    op_buffer.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));

    m_nChannel         = 0;
    m_iSampleIdx       = 0;
    m_iNSamplePerBlock = 0;
    m_oSampling        = 0;
    m_oSampleIdx       = 0;
    m_oNSamplePerBlock = 0;

    m_nTotalSample  = 0;
    m_startTimeBase = 0;
    m_lastStartTime = 0;
    m_lastEndTime   = 0;

    return true;
}

bool CBoxAlgorithmSignalDecimation::uninitialize()
{
    op_buffer.uninitialize();
    ip_pMatrix.uninitialize();
    ip_sampling.uninitialize();

    if (m_encoder) {
        m_encoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_encoder);
        m_encoder = nullptr;
    }

    op_sampling.uninitialize();
    op_pMatrix.uninitialize();
    ip_buffer.uninitialize();

    if (m_decoder) {
        m_decoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_decoder);
        m_decoder = nullptr;
    }

    return true;
}

bool CBoxAlgorithmSignalDecimation::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmSignalDecimation::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        ip_buffer = boxContext.getInputChunk(0, i);
        op_buffer = boxContext.getOutputChunk(0);

        const uint64_t tStart = boxContext.getInputChunkStartTime(0, i);
        const uint64_t tEnd   = boxContext.getInputChunkEndTime(0, i);

        if (tStart != m_lastEndTime) {
            m_startTimeBase = tStart;
            m_iSampleIdx    = 0;
            m_oSampleIdx    = 0;
            m_nTotalSample  = 0;
        }

        m_lastStartTime = tStart;
        m_lastEndTime   = tEnd;

        m_decoder->process();
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader)) {
            m_iSampleIdx       = 0;
            m_iNSamplePerBlock = op_pMatrix->getDimensionSize(1);
            m_iSampling        = op_sampling;

            OV_ERROR_UNLESS_KRF(m_iSampling%m_decimationFactor == 0,
                                "the signal arrives at " << m_iSampling << " Hz, which cannot be divided evenly by this box's setting \"Decimation factor\", which is " << m_decimationFactor << ". Might we humbly suggest another factor, or the Signal Resampling box? Thank you most kindly.", Kernel::ErrorType::BadSetting);

            m_oSampleIdx       = 0;
            m_oNSamplePerBlock = size_t(m_iNSamplePerBlock / m_decimationFactor);
            m_oNSamplePerBlock = (m_oNSamplePerBlock ? m_oNSamplePerBlock : 1);
            m_oSampling        = op_sampling / m_decimationFactor;

            OV_ERROR_UNLESS_KRF(m_oSampling != 0, "Failed to decimate: output sampling frequency is 0", Kernel::ErrorType::BadOutput);

            m_nChannel     = op_pMatrix->getDimensionSize(0);
            m_nTotalSample = 0;

            ip_pMatrix->copyDescription(*op_pMatrix);
            ip_pMatrix->setDimensionSize(1, m_oNSamplePerBlock);
            ip_sampling = m_oSampling;
            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader);
            ip_pMatrix->resetBuffer();

            boxContext.markOutputAsReadyToSend(0, tStart, tStart); // $$$ supposes we have one node per chunk
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer)) {
            double* iBuffer = op_pMatrix->getBuffer();
            double* oBuffer = ip_pMatrix->getBuffer() + m_oSampleIdx;

            for (size_t j = 0; j < m_iNSamplePerBlock; ++j) {
                double* iBufferTmp = iBuffer;
                double* oBufferTmp = oBuffer;
                for (size_t k = 0; k < m_nChannel; ++k) {
                    *oBufferTmp += *iBufferTmp;
                    oBufferTmp += m_oNSamplePerBlock;
                    iBufferTmp += m_iNSamplePerBlock;
                }

                m_iSampleIdx++;
                if (m_iSampleIdx == m_decimationFactor) {
                    m_iSampleIdx = 0;
                    oBufferTmp   = oBuffer;
                    for (size_t k = 0; k < m_nChannel; ++k) {
                        *oBufferTmp /= double(m_decimationFactor);
                        oBufferTmp += m_oNSamplePerBlock;
                    }

                    oBuffer++;
                    m_oSampleIdx++;
                    if (m_oSampleIdx == m_oNSamplePerBlock) {
                        oBuffer      = ip_pMatrix->getBuffer();
                        m_oSampleIdx = 0;
                        m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer);
                        const uint64_t tStartSample = m_startTimeBase + CTime(m_oSampling, m_nTotalSample).time();
                        const uint64_t tEndSample   = m_startTimeBase + CTime(m_oSampling, m_nTotalSample + m_oNSamplePerBlock).time();
                        boxContext.markOutputAsReadyToSend(0, tStartSample, tEndSample);
                        m_nTotalSample += m_oNSamplePerBlock;

                        ip_pMatrix->resetBuffer();
                    }
                }

                iBuffer++;
            }
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedEnd)) {
            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeEnd);
            boxContext.markOutputAsReadyToSend(0, tStart, tStart); // $$$ supposes we have one node per chunk
        }

        boxContext.markInputAsDeprecated(0, i);
    }
    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
