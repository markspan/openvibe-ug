#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TStreamStructureDecoderLocal : public T
{
protected:

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamStructureDecoder));
        m_codec->initialize();
        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_StreamStructureDecoder_InputParameterId_MemoryBufferToDecode));

        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_iBuffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StreamStructureDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StreamStructureDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StreamStructureDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TStreamStructureDecoder : public TStreamStructureDecoderLocal<TDecoder<T>>
{
    using TStreamStructureDecoderLocal<TDecoder<T>>::m_boxAlgorithm;
public:
    using TStreamStructureDecoderLocal<TDecoder<T>>::uninitialize;

    TStreamStructureDecoder() { }

    TStreamStructureDecoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TStreamStructureDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
