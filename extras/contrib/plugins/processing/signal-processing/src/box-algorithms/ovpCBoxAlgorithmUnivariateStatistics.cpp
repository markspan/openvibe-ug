#include "ovpCBoxAlgorithmUnivariateStatistics.h"
#include "../algorithms/ovpCAlgorithmUnivariateStatistics.h"
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxUnivariateStatistic::initialize()
{
    //initialise en/decoder function of the input type
    this->getStaticBoxContext().getInputType(0, m_inputTypeID);

#if 0 // this is not needed as you know you always habe signal
    if(m_inputTypeID==OV_TypeId_StreamedMatrix)
    {
        m_decoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixDecoder));
        m_meanEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
        m_varianceEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
        m_rangeEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
        m_medianEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
        m_iqrEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
        m_percentileEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
    }
    else if(m_inputTypeID==OV_TypeId_FeatureVector)
    {
        m_decoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorDecoder));
        m_meanEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorEncoder));
        m_varianceEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorEncoder));
        m_rangeEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorEncoder));
        m_medianEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorEncoder));
        m_iqrEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorEncoder));
        m_percentileEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorEncoder));
    }
    else if(m_inputTypeID==OV_TypeId_Signal)
    {
#endif
    m_decoder           = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
    m_meanEncoder       = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_varianceEncoder   = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_rangeEncoder      = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_medianEncoder     = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_iqrEncoder        = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_percentileEncoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
#if 0 // this is not needed as you know you always habe signal
    }
    else if(m_inputTypeID==OV_TypeId_Spectrum)
    {
        m_decoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumDecoder));
        m_meanEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
        m_varianceEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
        m_rangeEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
        m_medianEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
        m_iqrEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
        m_percentileEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
    }
    else
    {
        this->getLogManager() << Kernel::LogLevel_Debug << "Input type is not planned : no matrix base. This box can't work, so it is disabled\n";
        return false;
    }
#endif

    m_decoder->initialize();
    m_meanEncoder->initialize();
    m_varianceEncoder->initialize();
    m_rangeEncoder->initialize();
    m_medianEncoder->initialize();
    m_iqrEncoder->initialize();
    m_percentileEncoder->initialize();

    //initialize the real algorithm this box encapsulate
    m_matrixStatistic = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_AlgoUnivariateStatistic));
    m_matrixStatistic->initialize();

    //initialize all handlers
    m_matrixStatistic->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_Matrix)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));
    m_meanEncoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_matrixStatistic->getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Mean));
    m_varianceEncoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_matrixStatistic->getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Var));
    m_rangeEncoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_matrixStatistic->getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Range));
    m_medianEncoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_matrixStatistic->getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Med));
    m_iqrEncoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_matrixStatistic->getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_IQR));
    m_percentileEncoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_matrixStatistic->getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Percent));

#if 0 // this is not needed as you know you always habe signal

    /// specific connection for what is different of matrix base
    if(m_inputTypeID==OV_TypeId_Signal)
    {
#endif
    op_sampling.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_meanEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_varianceEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_rangeEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_medianEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_iqrEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_percentileEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
#if 0 // this is not needed as you know you always habe signal
    }
    else if(m_inputTypeID==OV_TypeId_Spectrum)
    {
        m_meanEncoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_MinMaxFrequencyBands)->setReferenceTarget(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_MinMaxFrequencyBands));
        m_varianceEncoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_MinMaxFrequencyBands)->setReferenceTarget(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_MinMaxFrequencyBands));
        m_rangeEncoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_MinMaxFrequencyBands)->setReferenceTarget(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_MinMaxFrequencyBands));
        m_medianEncoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_MinMaxFrequencyBands)->setReferenceTarget(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_MinMaxFrequencyBands));
        m_iqrEncoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_MinMaxFrequencyBands)->setReferenceTarget(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_MinMaxFrequencyBands));
        m_percentileEncoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_MinMaxFrequencyBands)->setReferenceTarget(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_MinMaxFrequencyBands));
    }
#endif

    op_compression.initialize(m_matrixStatistic->getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Compression));

    ip_isMeanActive.initialize(m_matrixStatistic->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_MeanActive));
    ip_isVarianceActive.initialize(m_matrixStatistic->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_VarActive));
    ip_isRangeActive.initialize(m_matrixStatistic->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_RangeActive));
    ip_isMedianActive.initialize(m_matrixStatistic->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_MedActive));
    ip_isIQRActive.initialize(m_matrixStatistic->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_IQRActive));
    ip_isPercentileActive.initialize(m_matrixStatistic->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_PercentActive));

    //get dis/enabled output wanted
    ip_isMeanActive       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    ip_isVarianceActive   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    ip_isRangeActive      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    ip_isMedianActive     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    ip_isIQRActive        = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    ip_isPercentileActive = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

    //the percentile value
    ip_parameterValue.initialize(m_matrixStatistic->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_PercentValue));
    ip_parameterValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);

    return true;
}

bool CBoxUnivariateStatistic::uninitialize()
{
#if 0
    if(m_inputTypeID==OV_TypeId_Signal)
    {
#endif
    op_sampling.uninitialize();
#if 0
    }
#endif

    ip_parameterValue.uninitialize();

    m_matrixStatistic->uninitialize();
    m_meanEncoder->uninitialize();
    m_varianceEncoder->uninitialize();
    m_rangeEncoder->uninitialize();
    m_medianEncoder->uninitialize();
    m_iqrEncoder->uninitialize();
    m_percentileEncoder->uninitialize();
    m_decoder->uninitialize();

    this->getAlgorithmManager().releaseAlgorithm(*m_matrixStatistic);
    this->getAlgorithmManager().releaseAlgorithm(*m_meanEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_varianceEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_rangeEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_medianEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_iqrEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_percentileEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_decoder);

    return true;
}

bool CBoxUnivariateStatistic::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxUnivariateStatistic::process()
{
    Kernel::IBoxIO& boxContext = getDynamicBoxContext();

    //for each input, calculate statistics and return the value
    for (size_t j = 0; j < boxContext.getInputChunkCount(0); ++j)
    {
        Kernel::TParameterHandler<const CMemoryBuffer*> iBufferHandle(
            m_decoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_InputParameterId_MemoryBufferToDecode));
        Kernel::TParameterHandler<CMemoryBuffer*> oBufferHandleMean(
            m_meanEncoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));
        Kernel::TParameterHandler<CMemoryBuffer*> oBufferHandleVar(
            m_varianceEncoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));
        Kernel::TParameterHandler<CMemoryBuffer*> oBufferHandleRange(
            m_rangeEncoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));
        Kernel::TParameterHandler<CMemoryBuffer*> oBufferHandleMedian(
            m_medianEncoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));
        Kernel::TParameterHandler<CMemoryBuffer*> oBufferHandleIqr(
            m_iqrEncoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));
        Kernel::TParameterHandler<CMemoryBuffer*> oBufferHandlePercent(
            m_percentileEncoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));
        iBufferHandle        = boxContext.getInputChunk(0, j);
        oBufferHandleMean    = boxContext.getOutputChunk(0);
        oBufferHandleVar     = boxContext.getOutputChunk(1);
        oBufferHandleRange   = boxContext.getOutputChunk(2);
        oBufferHandleMedian  = boxContext.getOutputChunk(3);
        oBufferHandleIqr     = boxContext.getOutputChunk(4);
        oBufferHandlePercent = boxContext.getOutputChunk(5);

        m_decoder->process();

        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedHeader))
        {
            m_matrixStatistic->process(OVP_Algorithm_UnivariateStatistic_InputTriggerId_Initialize);

#if 0 // this is not needed as you know you always habe signal
            if(m_inputTypeID==OV_TypeId_FeatureVector)
            {
            }
            if(m_inputTypeID==OV_TypeId_Signal)
            {
#endif
            this->getLogManager() << Kernel::LogLevel_Debug << "DownSampling information : " << op_sampling << "*" << op_compression << "=>" <<
                    op_sampling * op_compression << "\n";
            op_sampling = uint64_t(op_sampling * op_compression);
            if (op_sampling == 0) { this->getLogManager() << Kernel::LogLevel_Warning << "Output sampling Rate is null, it could produce problem in next boxes \n"; }
#if 0 // this is not needed as you know you always habe signal
            }
            else if(m_inputTypeID==OV_TypeId_Spectrum)
            {
            }
#endif

            if (ip_isMeanActive)
            {
                m_meanEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isVarianceActive)
            {
                m_varianceEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(1, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isRangeActive)
            {
                m_rangeEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(2, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isMedianActive)
            {
                m_medianEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(3, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isIQRActive)
            {
                m_iqrEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(4, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isPercentileActive)
            {
                m_percentileEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(5, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
        }//end header
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedBuffer))
        {
            m_matrixStatistic->process(OVP_Algorithm_UnivariateStatistic_InputTriggerId_Process);
            if (m_matrixStatistic->isOutputTriggerActive(OVP_Algorithm_UnivariateStatistic_OutputTriggerId_ProcessDone))
            {
#if 0
                if(m_inputTypeID == OV_TypeId_FeatureVector) { }
                if(m_inputTypeID == OV_TypeId_Signal) { }
                else if(m_inputTypeID == OV_TypeId_Spectrum) { }
#endif
                if (ip_isMeanActive)
                {
                    m_meanEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer);
                    boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
                }
                if (ip_isVarianceActive)
                {
                    m_varianceEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer);
                    boxContext.markOutputAsReadyToSend(1, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
                }
                if (ip_isRangeActive)
                {
                    m_rangeEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer);
                    boxContext.markOutputAsReadyToSend(2, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
                }
                if (ip_isMedianActive)
                {
                    m_medianEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer);
                    boxContext.markOutputAsReadyToSend(3, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
                }
                if (ip_isIQRActive)
                {
                    m_iqrEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer);
                    boxContext.markOutputAsReadyToSend(4, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
                }
                if (ip_isPercentileActive)
                {
                    m_percentileEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer);
                    boxContext.markOutputAsReadyToSend(5, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
                }
            }
            else { this->getLogManager() << Kernel::LogLevel_Debug << "Process not activated\n"; }
        }//end buffer
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedEnd))
        {
#if 0
            if(m_inputTypeID == OV_TypeId_FeatureVector) { }
            if(m_inputTypeID == OV_TypeId_Signal) { }
            else if(m_inputTypeID == OV_TypeId_Spectrum) { }
#endif
            if (ip_isMeanActive)
            {
                m_meanEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd);
                boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isVarianceActive)
            {
                m_varianceEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd);
                boxContext.markOutputAsReadyToSend(1, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isRangeActive)
            {
                m_rangeEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd);
                boxContext.markOutputAsReadyToSend(2, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isMedianActive)
            {
                m_medianEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd);
                boxContext.markOutputAsReadyToSend(3, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isIQRActive)
            {
                m_iqrEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd);
                boxContext.markOutputAsReadyToSend(4, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
            if (ip_isPercentileActive)
            {
                m_percentileEncoder->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd);
                boxContext.markOutputAsReadyToSend(5, boxContext.getInputChunkStartTime(0, j), boxContext.getInputChunkEndTime(0, j));
            }
        }//end ender

        boxContext.markInputAsDeprecated(0, j);
    }
    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
