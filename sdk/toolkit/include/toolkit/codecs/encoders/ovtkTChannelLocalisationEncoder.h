#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "ovtkTStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TChannelLocalisationEncoderLocal : public T
{
protected:

    Kernel::TParameterHandler<bool> m_iDynamic;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_buffer;
    using T::m_iMatrix;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_ChannelLocalisationEncoder));
        m_codec->initialize();
        m_iMatrix.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ChannelLocalisationEncoder_InputParameterId_Matrix));
        m_iDynamic.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ChannelLocalisationEncoder_InputParameterId_Dynamic));
        m_buffer.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ChannelLocalisationEncoder_OutputParameterId_EncodedMemoryBuffer));

        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_iMatrix.uninitialize();
        m_iDynamic.uninitialize();
        m_buffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<bool>& getInputDynamic() { return m_iDynamic; }

protected:
    bool encodeHeaderImpl() { return m_codec->process(OVP_GD_Algorithm_ChannelLocalisationEncoder_InputTriggerId_EncodeHeader); }
    bool encodeBufferImpl() { return m_codec->process(OVP_GD_Algorithm_ChannelLocalisationEncoder_InputTriggerId_EncodeBuffer); }
    bool encodeEndImpl() { return m_codec->process(OVP_GD_Algorithm_ChannelLocalisationEncoder_InputTriggerId_EncodeEnd); }
};

template <class T>
class TChannelLocalisationEncoder : public TChannelLocalisationEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>
{
    using TChannelLocalisationEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>::m_boxAlgorithm;
public:
    using TChannelLocalisationEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>::uninitialize;

    TChannelLocalisationEncoder() { }

    TChannelLocalisationEncoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TChannelLocalisationEncoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
