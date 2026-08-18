#include "ovpCDetectingMinMaxBoxAlgorithm.h"

#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CDetectingMinMaxBoxAlgorithm::initialize()
{
    CIdentifier inputTypeID;
    getStaticBoxContext().getInputType(0, inputTypeID);
    if (inputTypeID == OV_TypeId_Signal)
    {
        m_decoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
        m_encoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
    }
    else { return false; }
    m_decoder->initialize();
    m_encoder->initialize();

    // Detects MinMax of signal input buffer
    m_detectingMinMax = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_DetectingMinMax));
    m_detectingMinMax->initialize();

    // compute filter coefs settings
    const CString minMax = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_maxFlag            = false;
    m_minFlag            = false;

    if (this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_MinMax, minMax) == size_t(EMinMax::Min)) { m_minFlag = true; }
    if (this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_MinMax, minMax) == size_t(EMinMax::Max)) { m_maxFlag = true; }

    const double start = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    const double end   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    // DetectingMinMax settings
    m_detectingMinMax->getInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_SignalMatrix)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));

    m_detectingMinMax->getInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_detectingMinMax->getInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_TimeWindowStart)->setValue(&start);
    m_detectingMinMax->getInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_TimeWindowEnd)->setValue(&end);

    // encoder settings
    m_encoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_detectingMinMax->getOutputParameter(OVP_Algorithm_DetectingMinMax_OutputParameterId_SignalMatrix));

    m_lastStartTime = 0;
    m_lastEndTime   = 0;
    return true;
}

bool CDetectingMinMaxBoxAlgorithm::uninitialize()
{
    m_encoder->uninitialize();
    m_decoder->uninitialize();
    m_detectingMinMax->uninitialize();
    getAlgorithmManager().releaseAlgorithm(*m_encoder);
    getAlgorithmManager().releaseAlgorithm(*m_decoder);
    getAlgorithmManager().releaseAlgorithm(*m_detectingMinMax);

    return true;
}

bool CDetectingMinMaxBoxAlgorithm::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CDetectingMinMaxBoxAlgorithm::process()
{
    Kernel::IBoxIO& boxContext = getDynamicBoxContext();
    const size_t nInput        = getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i)
    {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j)
        {
            Kernel::TParameterHandler<const CMemoryBuffer*> iBufferHandle(
                m_decoder->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));
            Kernel::TParameterHandler<CMemoryBuffer*> oBufferHandle(
                m_encoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));
            iBufferHandle       = boxContext.getInputChunk(i, j);
            oBufferHandle       = boxContext.getOutputChunk(i);
            const uint64_t tEnd = m_lastStartTime + boxContext.getInputChunkEndTime(i, j) - boxContext.getInputChunkStartTime(i, j);

            m_decoder->process();
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader))
            {
                m_detectingMinMax->process(OVP_Algorithm_DetectingMinMax_InputTriggerId_Initialize);
                m_encoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(i, m_lastStartTime, tEnd);
            }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer))
            {
                if (m_minFlag) { m_detectingMinMax->process(OVP_Algorithm_DetectingMinMax_InputTriggerId_DetectsMin); }
                if (m_maxFlag) { m_detectingMinMax->process(OVP_Algorithm_DetectingMinMax_InputTriggerId_DetectsMax); }

                m_encoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer);
                boxContext.markOutputAsReadyToSend(i, m_lastStartTime, tEnd);
            }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedEnd))
            {
                m_encoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd);
                boxContext.markOutputAsReadyToSend(i, m_lastStartTime, tEnd);
            }

            m_lastStartTime = boxContext.getInputChunkStartTime(i, j);
            m_lastEndTime   = boxContext.getInputChunkEndTime(i, j);
            boxContext.markInputAsDeprecated(i, j);
        }
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
