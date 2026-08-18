#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTStreamedMatrixDecoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TSignalDecoderLocal : public T
{
protected:

    Kernel::TParameterHandler<uint64_t> m_sampling;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;
    using T::m_oMatrix;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
        m_codec->initialize();
        m_sampling.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));
        m_oMatrix.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Matrix));
        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));

        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_sampling.uninitialize();
        m_oMatrix.uninitialize();
        m_iBuffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<uint64_t>& getOutputSamplingRate() { return m_sampling; }

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TSignalDecoder : public TSignalDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>
{
    using TSignalDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::m_boxAlgorithm;
public:
    using TSignalDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::uninitialize;

    TSignalDecoder() { }

    TSignalDecoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TSignalDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
