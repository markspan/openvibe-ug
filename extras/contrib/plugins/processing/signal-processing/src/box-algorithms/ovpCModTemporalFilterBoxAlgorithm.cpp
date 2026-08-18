#include "ovpCModTemporalFilterBoxAlgorithm.h"
#include <cstdlib>
#include <cerrno>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CModTemporalFilterBoxAlgorithm::initialize()
{
    m_hasBeenInit = false;

    m_decoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
    m_encoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));

    m_decoder->initialize();
    m_encoder->initialize();

    ip_bufferToDecode.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));
    op_encodedBuffer.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));

    // Compute filter coeff algorithm
    m_computeModTemporalFilterCoefs = &getAlgorithmManager().getAlgorithm(
        getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ComputeTemporalFilterCoefs));
    m_computeModTemporalFilterCoefs->initialize();

    // Apply filter to signal input buffer
    m_applyModTemporalFilter = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ApplyTemporalFilter));
    m_applyModTemporalFilter->initialize();

    m_lastEndTime = 0;

    m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));

    m_filterMethod  = CString("");
    m_filterType    = CString("");
    m_filterOrder   = CString("");
    m_lowBand       = CString("");
    m_highBand      = CString("");
    m_passBandRiple = CString("");
    if (!updateSettings())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "The box cannot be initialized.\n";
        return false;
    }

    m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_Sampling)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));

    // apply filter settings
    m_applyModTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_SignalMatrix)->setReferenceTarget(
        m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Matrix));
    m_applyModTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_FilterCoefsMatrix)->setReferenceTarget(
        m_computeModTemporalFilterCoefs->getOutputParameter(
            OVP_Algorithm_ComputeTemporalFilterCoefs_OutputParameterId_Matrix));

    m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Matrix)->setReferenceTarget(
        m_applyModTemporalFilter->getOutputParameter(OVP_Algorithm_ApplyTemporalFilter_OutputParameterId_FilteredSignalMatrix));
    return true;
}

bool CModTemporalFilterBoxAlgorithm::updateSettings()
{
    bool retVal  = false;
    bool error   = false;
    char* endPtr = nullptr;
    //get the settings
    const CString filter           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const CString kindFilter       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    const CString filterOrder      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    const CString lowPassBandEdge  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    const CString highPassBandEdge = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    const CString passBandRipple   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

    if (m_filterMethod != filter)
    {
        const uint64_t parameter = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterMethod, filter);
        if (parameter == CIdentifier::undefined().id())
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Unrecognized filter method " << filter << ".\n";
            error = true;
        }
        else
        {
            Kernel::TParameterHandler<uint64_t> ip_nameFilter(
                m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterMethod));
            ip_nameFilter = parameter;
            retVal        = true;
        }
        m_filterMethod = filter; //We set up the new value to avoid to repeat the error log over and over again
    }

    if (m_filterType != kindFilter)
    {
        const uint64_t parameter = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterType, kindFilter);
        if (parameter == CIdentifier::undefined().id())
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Unrecognized filter type " << kindFilter << ".\n";
            error = true;
        }
        else
        {
            Kernel::TParameterHandler<uint64_t> ip_kindFilter(
                m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterType));
            ip_kindFilter = parameter;
            retVal        = true;
        }
        m_filterType = kindFilter; //We set up the new value to avoid to repeat the error log over and over again
    }

    if (m_filterOrder != filterOrder)
    {
        errno                   = 0;
        const int64_t parameter = strtol(filterOrder, &endPtr, 10);
        if (parameter <= 0 || (errno != 0 && parameter == 0) || *endPtr != '\0' || errno == ERANGE)
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Wrong filter order (" << filterOrder << "). Should be one or more.\n";
            error = true;
        }
        else
        {
            Kernel::TParameterHandler<uint64_t> ip_filterOrder(
                m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterOrder));
            ip_filterOrder = parameter;
            retVal         = true;
        }
        m_filterOrder = filterOrder; //We set up the new value to avoid to repeat the error log over and over again
    }

    if (m_lowBand != lowPassBandEdge)
    {
        Kernel::TParameterHandler<double> ip_highCutFrequency(
            m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_HighCutFrequency));

        errno                  = 0;
        const double parameter = strtod(lowPassBandEdge, &endPtr);
        if (parameter < 0 || (errno != 0 && parameter == 0) || *endPtr != '\0' || errno == ERANGE)
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Wrong low cut frequency (" << lowPassBandEdge << " Hz). Should be positive.\n";
            error = true;
        }
        else if (m_hasBeenInit && parameter > double(ip_highCutFrequency)
        )//If it's not the first init we need to check that we do not set a wrong frequency according to the high one
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Wrong low cut frequency (" << lowPassBandEdge << " Hz). Should be under the high cut frequency "
                    << double(ip_highCutFrequency) << " Hz.\n";
            error = true;
        }
        else
        {
            Kernel::TParameterHandler<double> ip_lowCutFrequency(
                m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_LowCutFrequency));
            ip_lowCutFrequency = parameter;
            retVal             = true;
        }
        m_lowBand = lowPassBandEdge;
    }

    if (m_highBand != highPassBandEdge)
    {
        Kernel::TParameterHandler<double> ip_lowCutFrequency(
            m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_LowCutFrequency));
        errno                  = 0;
        const double parameter = strtod(highPassBandEdge, &endPtr);
        if (parameter < 0 || (errno != 0 && parameter == 0) || *endPtr != '\0' || errno == ERANGE)
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Wrong high cut frequency (" << highPassBandEdge << " Hz). Should be positive.\n";
            error = true;
        }
        else if (parameter < double(ip_lowCutFrequency))
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Wrong high cut frequency (" << highPassBandEdge << " Hz). Should be over the low cut frequency "
                    << double(ip_lowCutFrequency) << " Hz.\n";
            error = true;
        }
        else
        {
            Kernel::TParameterHandler<double> ip_highCutFrequency(
                m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_HighCutFrequency));
            ip_highCutFrequency = parameter;
            retVal              = true;
        }
        m_highBand = highPassBandEdge;
    }

    if (m_passBandRiple != passBandRipple)
    {
        errno                  = 0;
        const double parameter = strtod(passBandRipple, &endPtr);
        if ((errno != 0 && parameter == 0) || *endPtr != '\0' || errno == ERANGE)
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Wrong pass band ripple (" << passBandRipple << " dB).\n";
            error = true;
        }
        else
        {
            Kernel::TParameterHandler<double> ip_passBandRipple(
                m_computeModTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_BandPassRipple));
            ip_passBandRipple = parameter;
            retVal            = true;
        }
        m_passBandRiple = passBandRipple;
    }

    //If it was the original init we return false to stop the init process
    if (!m_hasBeenInit && error) { return false; }
    m_hasBeenInit = true;
    return retVal;
}

bool CModTemporalFilterBoxAlgorithm::compute()
{
    //compute filter coeff
    if (!m_computeModTemporalFilterCoefs->process(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_Initialize)) { return false; }
    if (!m_computeModTemporalFilterCoefs->process(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_ComputeCoefs)) { return false; }
    if (!m_applyModTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_Initialize)) { return false; }

    return true;
}

bool CModTemporalFilterBoxAlgorithm::uninitialize()
{
    m_applyModTemporalFilter->uninitialize();
    m_computeModTemporalFilterCoefs->uninitialize();
    m_encoder->uninitialize();
    m_decoder->uninitialize();

    getAlgorithmManager().releaseAlgorithm(*m_applyModTemporalFilter);
    getAlgorithmManager().releaseAlgorithm(*m_computeModTemporalFilterCoefs);
    getAlgorithmManager().releaseAlgorithm(*m_encoder);
    getAlgorithmManager().releaseAlgorithm(*m_decoder);

    return true;
}

bool CModTemporalFilterBoxAlgorithm::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CModTemporalFilterBoxAlgorithm::process()
{
    Kernel::IBoxIO& boxContext = getDynamicBoxContext();
    const size_t nInput        = getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i)
    {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j)
        {
            //TParameterHandler < const CMemoryBuffer* > iBufferHandle(m_decoder->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));
            //TParameterHandler < CMemoryBuffer* > oBufferHandle(m_encoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));
            //iBufferHandle=boxContext.getInputChunk(i, j);
            //oBufferHandle=boxContext.getOutputChunk(i);
            ip_bufferToDecode    = boxContext.getInputChunk(i, j);
            op_encodedBuffer     = boxContext.getOutputChunk(i);
            const uint64_t start = boxContext.getInputChunkStartTime(i, j);
            const uint64_t end   = boxContext.getInputChunkEndTime(i, j);

            if (!m_decoder->process()) { return false; }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader))
            {
                compute();
                if (!m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader)) { return false; }

                boxContext.markOutputAsReadyToSend(i, start, end);
            }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer))
            {
                //recompute if the settings have changed only
                if (updateSettings() && !compute()) { this->getLogManager() << Kernel::LogLevel_Error << "error during computation\n"; }

                if (m_lastEndTime == start)
                {
                    if (!m_applyModTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilterWithHistoric)) { return false; }
                }
                else { if (!m_applyModTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilter)) { return false; } }
                if (!m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer)) { return false; }
                boxContext.markOutputAsReadyToSend(i, start, end);
            }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedEnd))
            {
                if (!m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeEnd)) { return false; }
                boxContext.markOutputAsReadyToSend(i, start, end);
            }

            m_lastEndTime = end;
            boxContext.markInputAsDeprecated(i, j);
        }
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
