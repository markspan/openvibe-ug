#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTDecoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TStimulationDecoderLocal : public T
{
protected:

    Kernel::TParameterHandler<CStimulationSet*> m_oStimulationSet;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
        m_codec->initialize();
        m_oStimulationSet.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));
        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));

        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_oStimulationSet.uninitialize();
        m_iBuffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<CStimulationSet*>& getOutputStimulationSet() { return m_oStimulationSet; }

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TStimulationDecoder : public TStimulationDecoderLocal<TDecoder<T>>
{
    using TStimulationDecoderLocal<TDecoder<T>>::m_boxAlgorithm;
public:
    using TStimulationDecoderLocal<TDecoder<T>>::uninitialize;

    TStimulationDecoder() { }

    TStimulationDecoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TStimulationDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
