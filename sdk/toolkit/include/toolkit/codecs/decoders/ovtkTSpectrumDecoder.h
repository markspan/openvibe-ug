#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTStreamedMatrixDecoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TSpectrumDecoderLocal : public T
{
protected:

    Kernel::TParameterHandler<CMatrix*> m_frequencyAbscissa;
    Kernel::TParameterHandler<uint64_t> m_sampling;


    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;
    using T::m_oMatrix;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumDecoder));
        m_codec->initialize();
        m_oMatrix.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_Matrix));
        m_frequencyAbscissa.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_FrequencyAbscissa));
        m_sampling.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_Sampling));
        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_SpectrumDecoder_InputParameterId_MemoryBufferToDecode));

        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_oMatrix.uninitialize();
        m_frequencyAbscissa.uninitialize();
        m_sampling.uninitialize();
        m_iBuffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<uint64_t>& getOutputSamplingRate() { return m_sampling; }
    Kernel::TParameterHandler<CMatrix*>& getOutputFrequencyAbscissa() { return m_frequencyAbscissa; }

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_SpectrumDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_SpectrumDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_SpectrumDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TSpectrumDecoder : public TSpectrumDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>
{
    using TSpectrumDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::m_boxAlgorithm;
public:
    using TSpectrumDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::uninitialize;

    TSpectrumDecoder() { }

    TSpectrumDecoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TSpectrumDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
