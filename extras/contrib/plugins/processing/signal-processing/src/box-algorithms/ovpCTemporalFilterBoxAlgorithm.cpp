#include "ovpCTemporalFilterBoxAlgorithm.h"
#include <cstdlib>
#include <cerrno>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CTemporalFilterBoxAlgorithm::initialize()
{
    m_decoder = new Toolkit::TSignalDecoder<CTemporalFilterBoxAlgorithm>(*this, 0);
    m_encoder = new Toolkit::TSignalEncoder<CTemporalFilterBoxAlgorithm>(*this, 0);

    // Compute filter coeff algorithm
    m_computeTemporalFilterCoefs = &getAlgorithmManager().getAlgorithm(
        getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ComputeTemporalFilterCoefs));
    m_computeTemporalFilterCoefs->initialize();

    // Apply filter to signal input buffer
    m_applyTemporalFilter = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ApplyTemporalFilter));
    m_applyTemporalFilter->initialize();

    m_lastEndTime = 0;

    // compute filter coefs settings
    const CString filter           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const CString kindFilter       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    const CString order            = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    const CString lowPassBandEdge  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    const CString highPassBandEdge = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    const CString passBandRipple   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

    bool initError = false;
    char* endPtr   = nullptr;


    uint64_t uiParameter = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterMethod, filter);
    if (uiParameter == CIdentifier::undefined().id())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Unrecognized filter method " << filter << ".\n";
        initError = true;
    }
    Kernel::TParameterHandler<uint64_t> ip_nameFilter(
        m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterMethod));
    ip_nameFilter = uiParameter;


    uiParameter = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterType, kindFilter);
    if (uiParameter == CIdentifier::undefined().id())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Unrecognized filter type " << kindFilter << ".\n";
        initError = true;
    }
    Kernel::TParameterHandler<uint64_t> ip_kindFilter(
        m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterType));
    ip_kindFilter = uiParameter;

    errno                      = 0;
    const int64_t intParameter = strtol(order, &endPtr, 10);
    if (intParameter <= 0 || (errno != 0 && intParameter == 0) || *endPtr != '\0' || errno == ERANGE)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Wrong filter order (" << order << "). Should be one or more.\n";
        initError = true;
    }
    Kernel::TParameterHandler<uint64_t> ip_filterOrder(
        m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterOrder));
    ip_filterOrder = intParameter;

    errno             = 0;
    double dParameter = strtod(lowPassBandEdge, &endPtr);
    if (dParameter < 0 || (errno != 0 && dParameter == 0) || *endPtr != '\0' || errno == ERANGE)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Wrong low cut frequency (" << lowPassBandEdge << " Hz). Should be positive.\n";
        initError = true;
    }
    Kernel::TParameterHandler<double> ip_lowCutFrequency(
        m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_LowCutFrequency));
    ip_lowCutFrequency = dParameter;

    errno      = 0;
    dParameter = strtod(highPassBandEdge, &endPtr);
    if (dParameter < 0 || (errno != 0 && dParameter == 0) || *endPtr != '\0' || errno == ERANGE)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Wrong high cut frequency (" << highPassBandEdge << " Hz). Should be positive.\n";
        initError = true;
    }
    else if (dParameter < double(ip_lowCutFrequency))
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Wrong high cut frequency (" << highPassBandEdge << " Hz). Should be over the low cut frequency "
                << lowPassBandEdge << " Hz.\n";
        initError = true;
    }
    Kernel::TParameterHandler<double> ip_highCutFrequency(
        m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_HighCutFrequency));
    ip_highCutFrequency = dParameter;

    errno      = 0;
    dParameter = strtod(passBandRipple, &endPtr);
    if ((errno != 0 && dParameter == 0) || *endPtr != '\0' || errno == ERANGE)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Wrong pass band ripple (" << passBandRipple << " dB).\n";
        initError = true;
    }
    Kernel::TParameterHandler<double> ip_passBandRipple(
        m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_BandPassRipple));
    ip_passBandRipple = dParameter;


    Kernel::TParameterHandler<uint64_t>
            ip_sampling(m_computeTemporalFilterCoefs->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_Sampling));
    ip_sampling.setReferenceTarget(m_decoder->getOutputSamplingRate());

    // apply filter settings
    m_applyTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_FilterCoefsMatrix)->setReferenceTarget(
        m_computeTemporalFilterCoefs->getOutputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_OutputParameterId_Matrix));

    m_encoder->getInputMatrix().setReferenceTarget(
        m_applyTemporalFilter->getOutputParameter(OVP_Algorithm_ApplyTemporalFilter_OutputParameterId_FilteredSignalMatrix));
    m_encoder->getInputSamplingRate().setReferenceTarget(m_decoder->getOutputSamplingRate());

    if (initError)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Something went wrong during the intialization. Desactivation of the box.\n";
        return false;
    }

    return true;
}

bool CTemporalFilterBoxAlgorithm::uninitialize()
{
    m_applyTemporalFilter->uninitialize();
    getAlgorithmManager().releaseAlgorithm(*m_applyTemporalFilter);
    m_computeTemporalFilterCoefs->uninitialize();
    getAlgorithmManager().releaseAlgorithm(*m_computeTemporalFilterCoefs);

    //codecs
    m_encoder->uninitialize();
    delete m_encoder;
    m_decoder->uninitialize();
    delete m_decoder;
    return true;
}

bool CTemporalFilterBoxAlgorithm::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CTemporalFilterBoxAlgorithm::process()
{
    Kernel::IBoxIO& boxContext = getDynamicBoxContext();
    const size_t nInput        = getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i)
    {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j)
        {
            const uint64_t tStart = boxContext.getInputChunkStartTime(i, j);
            const uint64_t tEnd   = boxContext.getInputChunkEndTime(i, j);

            if (!m_decoder->decode(j)) { return false; }

            //this has to be done here as it does not work if done once in initialize()
            CMatrix* iMatrix                                   = m_decoder->getOutputMatrix();
            Kernel::TParameterHandler<CMatrix*> matrixToFilter = m_applyTemporalFilter->getInputParameter(
                OVP_Algorithm_ApplyTemporalFilter_InputParameterId_SignalMatrix);
            matrixToFilter.setReferenceTarget(iMatrix);

            if (m_decoder->isHeaderReceived())
            {
                if (!m_computeTemporalFilterCoefs->process(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_Initialize)) { return false; }
                if (!m_computeTemporalFilterCoefs->process(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_ComputeCoefs)) { return false; }
                if (!m_applyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_Initialize)) { return false; }
                if (!m_encoder->encodeHeader()) { return false; }

                boxContext.markOutputAsReadyToSend(i, tStart, tEnd);
            }
            if (m_decoder->isBufferReceived())
            {
                if (m_lastEndTime == tStart)
                {
                    if (!m_applyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilterWithHistoric)) { return false; }
                }
                else { if (!m_applyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilter)) { return false; } }
                if (!m_encoder->encodeBuffer()) { return false; }
                boxContext.markOutputAsReadyToSend(i, tStart, tEnd);
            }
            if (m_decoder->isEndReceived())
            {
                if (!m_encoder->encodeEnd()) { return false; }
                boxContext.markOutputAsReadyToSend(i, tStart, tEnd);
            }

            // m_lastStartTime=tStart;
            m_lastEndTime = tEnd;
            boxContext.markInputAsDeprecated(i, j);
        }
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
