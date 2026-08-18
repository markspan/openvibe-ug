///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmEpochAverage.cpp
/// \brief Classes implementation for the Box Epoch average.
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

#include "CBoxAlgorithmEpochAverage.hpp"

#include "../../algorithms/basic/CAlgorithmMatrixAverage.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmEpochAverage::initialize()
{
    CIdentifier inputTypeId;
    getStaticBoxContext().getInputType(0, inputTypeId);
    if (inputTypeId == OV_TypeId_StreamedMatrix || inputTypeId == OV_TypeId_TimeFrequency) {
        m_decoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixDecoder));
        m_encoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
    }
    else if (inputTypeId == OV_TypeId_FeatureVector) {
        m_decoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorDecoder));
        m_encoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorEncoder));
    }
    else if (inputTypeId == OV_TypeId_Signal) {
        m_decoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
        m_encoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    }
    else if (inputTypeId == OV_TypeId_Spectrum) {
        m_decoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumDecoder));
        m_encoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
    }
    else { return false; }
    m_decoder->initialize();
    m_encoder->initialize();

    m_matrixAverage = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(Algorithm_MatrixAverage));
    m_matrixAverage->initialize();

    if (inputTypeId == OV_TypeId_StreamedMatrix) { }
    else if (inputTypeId == OV_TypeId_FeatureVector) { }
    else if (inputTypeId == OV_TypeId_Signal) {
        m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setReferenceTarget(
            m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    }
    else if (inputTypeId == OV_TypeId_Spectrum) {
        m_encoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_FrequencyAbscissa)->setReferenceTarget(
            m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_FrequencyAbscissa));
        m_encoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_Sampling)->setReferenceTarget(
            m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_Sampling));
    }

    ip_averagingMethod.initialize(m_matrixAverage->getInputParameter(MatrixAverage_InputParameterId_AveragingMethod));
    ip_matrixCount.initialize(m_matrixAverage->getInputParameter(MatrixAverage_InputParameterId_MatrixCount));

    ip_averagingMethod = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
    ip_matrixCount     = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));

    m_matrixAverage->getInputParameter(MatrixAverage_InputParameterId_Matrix)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));
    m_encoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_matrixAverage->getOutputParameter(MatrixAverage_OutputParameterId_AveragedMatrix));

    OV_ERROR_UNLESS_KRF(ip_matrixCount > 0, "Invalid number of epochs (expected value > 0)", Kernel::ErrorType::BadSetting);

    return true;
}

bool CBoxAlgorithmEpochAverage::uninitialize()
{
    CIdentifier inputTypeID;
    getStaticBoxContext().getInputType(0, inputTypeID);
    if (inputTypeID == OV_TypeId_StreamedMatrix || inputTypeID == OV_TypeId_FeatureVector
        || inputTypeID == OV_TypeId_Signal || inputTypeID == OV_TypeId_Spectrum) {
        ip_averagingMethod.uninitialize();
        ip_matrixCount.uninitialize();

        m_matrixAverage->uninitialize();
        m_encoder->uninitialize();
        m_decoder->uninitialize();

        getAlgorithmManager().releaseAlgorithm(*m_matrixAverage);
        getAlgorithmManager().releaseAlgorithm(*m_encoder);
        getAlgorithmManager().releaseAlgorithm(*m_decoder);
    }

    return true;
}

bool CBoxAlgorithmEpochAverage::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmEpochAverage::process()
{
    Kernel::IBoxIO& boxContext = getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i) {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            Kernel::TParameterHandler<const CMemoryBuffer*> iMemoryBufferHandle(
                m_decoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_InputParameterId_MemoryBufferToDecode));
            Kernel::TParameterHandler<CMemoryBuffer*> oMemoryBufferHandle(
                m_encoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));
            iMemoryBufferHandle = boxContext.getInputChunk(i, j);
            oMemoryBufferHandle = boxContext.getOutputChunk(i);

            m_decoder->process();
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedHeader)) {
                m_matrixAverage->process(MatrixAverage_InputTriggerId_Reset);
                m_encoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(i, boxContext.getInputChunkStartTime(i, j), boxContext.getInputChunkEndTime(i, j));
            }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedBuffer)) {
                m_matrixAverage->process(MatrixAverage_InputTriggerId_FeedMatrix);
                if (m_matrixAverage->isOutputTriggerActive(MatrixAverage_OutputTriggerId_AveragePerformed)) {
                    m_encoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer);
                    boxContext.markOutputAsReadyToSend(i, boxContext.getInputChunkStartTime(i, j), boxContext.getInputChunkEndTime(i, j));
                }
            }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedEnd)) {
                m_encoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd);
                boxContext.markOutputAsReadyToSend(i, boxContext.getInputChunkStartTime(i, j), boxContext.getInputChunkEndTime(i, j));
            }

            boxContext.markInputAsDeprecated(i, j);
        }
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
