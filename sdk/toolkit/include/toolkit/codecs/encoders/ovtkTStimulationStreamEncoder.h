#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTEncoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TStimulationEncoderLocal : public T
{
protected:

    Kernel::TParameterHandler<CStimulationSet*> m_iStimulationSet;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_buffer;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationEncoder));
        m_codec->initialize();
        m_iStimulationSet.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));
        m_buffer.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_StimulationEncoder_OutputParameterId_EncodedMemoryBuffer));

        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_iStimulationSet.uninitialize();
        m_buffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<CStimulationSet*>& getInputStimulationSet() { return m_iStimulationSet; }

protected:
    bool encodeHeaderImpl() { return m_codec->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeHeader); }
    bool encodeBufferImpl() { return m_codec->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeBuffer); }
    bool encodeEndImpl() { return m_codec->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeEnd); }
};

template <class T>
class TStimulationEncoder : public TStimulationEncoderLocal<TEncoder<T>>
{
    using TStimulationEncoderLocal<TEncoder<T>>::m_boxAlgorithm;
public:
    using TStimulationEncoderLocal<TEncoder<T>>::uninitialize;

    TStimulationEncoder() { }

    TStimulationEncoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TStimulationEncoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
