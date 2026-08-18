#include "ovpCDownsamplingBoxAlgorithm.h"

#include <cstdlib>
#include <climits>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {


bool CDownsamplingBoxAlgorithm::initialize()
{
    CIdentifier inputTypeID;
    getStaticBoxContext().getInputType(0, inputTypeID);
    if (inputTypeID == OV_TypeId_Signal)
    {
        CIdentifier algorithmID = getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder);
        if (algorithmID == CIdentifier::undefined())
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Unable to find algorithm " << OVP_GD_ClassId_Algorithm_SignalDecoder << "\n";
            return false;
        }
        m_decoder = &getAlgorithmManager().getAlgorithm(algorithmID);

        algorithmID = getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder);
        if (algorithmID == CIdentifier::undefined())
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Unable to find algorithm " << OVP_GD_ClassId_Algorithm_SignalEncoder << "\n";
            return false;
        }
        m_encoder = &getAlgorithmManager().getAlgorithm(algorithmID);
    }
    else
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Only 'signal' input type is supported\n";
        return false;
    }
    m_decoder->initialize();
    m_encoder->initialize();

    ip_bufferToDecode.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));
    op_encodedBuffer.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));

    // Compute filter coeff algorithm
    m_computeTemporalFilterCoefs = &getAlgorithmManager().getAlgorithm(
        getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ComputeTemporalFilterCoefs));
    m_computeTemporalFilterCoefs->initialize();

    // Apply filter to signal input buffer
    m_applyTemporalFilter = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ApplyTemporalFilter));
    m_applyTemporalFilter->initialize();

    // Compute Downsampling of signal input buffer
    m_downsampling = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_Downsampling));
    m_downsampling->initialize();

    // Compute filter coefs settings
    m_newSampling                = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const CString ratio          = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    const CString filter         = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    const CString filterOrder    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    const CString passBandRipple = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

    double ratioValue = 1.0 / 4;
    if (this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FrequencyCutOffRatio, ratio) == size_t(EFrequencyCutOffRatio::R14))
    {
        ratioValue = 1.0 / 4;
    }
    if (this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FrequencyCutOffRatio, ratio) == size_t(EFrequencyCutOffRatio::R13))
    {
        ratioValue = 1.0 / 3;
    }
    if (this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FrequencyCutOffRatio, ratio) == size_t(EFrequencyCutOffRatio::R12))
    {
        ratioValue = 1.0 / 2;
    }

    uint64_t filterValue       = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterMethod, filter);
    uint64_t kindFilter        = uint64_t(EFilterType::LowPass); //Low Pass
    uint64_t order             = atoi(filterOrder);
    double lowCutFrequency     = 0;
    double highCutFrequency    = double(m_newSampling) * ratioValue;
    double passBandRippleValue = atof(passBandRipple);

    // Compute filter settings
    m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterMethod)->setValue(&filterValue);
    m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterType)->setValue(&kindFilter);
    m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterOrder)->setValue(&order);
    m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_LowCutFrequency)->setValue(&lowCutFrequency);
    m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_HighCutFrequency)->setValue(&highCutFrequency);
    m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_BandPassRipple)->setValue(&passBandRippleValue);

    // Apply filter settings
    m_applyTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_SignalMatrix)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Matrix));
    m_applyTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_FilterCoefsMatrix)->setReferenceTarget(
        m_computeTemporalFilterCoefs->getOutputParameter(
            OVP_Algorithm_ComputeTemporalFilterCoefs_OutputParameterId_Matrix));

    // Downsampling settings
    m_downsampling->getInputParameter(OVP_Algorithm_Downsampling_InputParameterId_SignalMatrix)->setReferenceTarget(
        m_applyTemporalFilter->getOutputParameter(OVP_Algorithm_ApplyTemporalFilter_OutputParameterId_FilteredSignalMatrix));
    m_downsampling->getInputParameter(OVP_Algorithm_Downsampling_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    m_downsampling->getInputParameter(OVP_Algorithm_Downsampling_InputParameterId_NewSampling)->setValue(&m_newSampling);

    // Encoder settings
    m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setValue(&m_newSampling);

    m_iSignal.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Matrix));
    m_oSignal.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Matrix));
    m_samplingRate.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));

    m_signalDesc = new CMatrix();

    m_lastEndTime       = uint64_t(-1);
    m_flagFirstTime     = true;
    m_warned            = false;
    m_lastBufferSize    = 0;
    m_currentBufferSize = 0;

    return true;
}

bool CDownsamplingBoxAlgorithm::uninitialize()
{
    delete m_signalDesc;
    m_signalDesc = nullptr;

    m_applyTemporalFilter->uninitialize();
    m_computeTemporalFilterCoefs->uninitialize();
    m_encoder->uninitialize();
    m_decoder->uninitialize();
    m_downsampling->uninitialize();

    getAlgorithmManager().releaseAlgorithm(*m_applyTemporalFilter);
    getAlgorithmManager().releaseAlgorithm(*m_computeTemporalFilterCoefs);
    getAlgorithmManager().releaseAlgorithm(*m_encoder);
    getAlgorithmManager().releaseAlgorithm(*m_decoder);
    getAlgorithmManager().releaseAlgorithm(*m_downsampling);

    return true;
}

bool CDownsamplingBoxAlgorithm::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CDownsamplingBoxAlgorithm::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t j = 0; j < boxContext.getInputChunkCount(0); ++j)
    {
        ip_bufferToDecode = boxContext.getInputChunk(0, j);
        op_encodedBuffer  = boxContext.getOutputChunk(0);

        const uint64_t tStart = boxContext.getInputChunkStartTime(0, j);
        const uint64_t tEnd   = boxContext.getInputChunkEndTime(0, j);

        m_decoder->process();
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader))
        {
            m_computeTemporalFilterCoefs->process(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_Initialize);
            m_computeTemporalFilterCoefs->process(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_ComputeCoefs);
            m_applyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_Initialize);
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer))
        {
            bool success = true;
            if (m_lastEndTime == tStart)
            {
                success &= m_applyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilterWithHistoric);
                success &= m_downsampling->process(OVP_Algorithm_Downsampling_InputTriggerId_ResampleWithHistoric);
            }
            else
            {
                success &= m_applyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilter);
                success &= m_downsampling->process(OVP_Algorithm_Downsampling_InputTriggerId_Resample);
            }

            if (!success)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Subalgorithm failed, returning\n";
                return false;
            }

            Kernel::TParameterHandler<CMatrix*> signal(m_downsampling->getOutputParameter(OVP_Algorithm_Downsampling_OutputParameterId_SignalMatrix));
            m_currentBufferSize = signal->getDimensionSize(1);

            if ((m_flagFirstTime) || (m_currentBufferSize != m_lastBufferSize))
            {
                if (!m_flagFirstTime && !m_warned)
                {
                    // this->getLogManager() << Kernel::LogLevel_Warning << "This box is flagged as unstable !\n";
                    this->getLogManager() << Kernel::LogLevel_Warning <<
                            "The input sampling frequency is not an integer multiple of the output sampling frequency, or the input epoch size is unsuitable. This results in creation of size varying output chunks. This may cause crash in downstream boxes.\n";
                    this->getLogManager() << Kernel::LogLevel_Debug << "(current block size is " << m_currentBufferSize << ", new block size is " <<
                            m_lastBufferSize << ")\n";
                    m_warned = true;
                }

                m_signalDesc->resize(m_iSignal->getDimensionSize(0), m_currentBufferSize);
                for (size_t k = 0; k < m_iSignal->getDimensionSize(0); ++k) { m_signalDesc->setDimensionLabel(0, k, m_iSignal->getDimensionLabel(0, k)); }
                m_oSignal.setReferenceTarget(m_signalDesc);
                m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader);
                boxContext.markOutputAsReadyToSend(0, tStart, tStart);
                m_lastBufferSize = m_currentBufferSize;

                m_flagFirstTime = false;
            }
            m_oSignal.setReferenceTarget(m_downsampling->getOutputParameter(OVP_Algorithm_Downsampling_OutputParameterId_SignalMatrix));
            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer);
            boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
            m_lastBufferSize = m_currentBufferSize;
        }

        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedEnd))
        {
            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeEnd);
            boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
        }

        m_lastEndTime = tEnd;

        boxContext.markInputAsDeprecated(0, j);
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
