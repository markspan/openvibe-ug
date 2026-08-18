#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "ovtkTStreamedMatrixDecoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TChannelLocalisationDecoderLocal : public T
{
protected:

    Kernel::TParameterHandler<bool> m_oDynamic;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;
    using T::m_oMatrix;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_ChannelLocalisationDecoder));
        m_codec->initialize();
        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ChannelLocalisationDecoder_InputParameterId_MemoryBufferToDecode));
        m_oMatrix.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputParameterId_Matrix));
        m_oDynamic.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputParameterId_Dynamic));

        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_oDynamic.uninitialize();
        m_oMatrix.uninitialize();
        m_iBuffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<bool>& getOutputDynamic() { return m_oDynamic; }

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TChannelLocalisationDecoder : public TChannelLocalisationDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>
{
    using TChannelLocalisationDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::m_boxAlgorithm;
public:
    using TChannelLocalisationDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::uninitialize;

    TChannelLocalisationDecoder() { }

    TChannelLocalisationDecoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TChannelLocalisationDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
