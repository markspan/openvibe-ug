#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTStreamedMatrixDecoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TFeatureVectorDecoderLocal : public T
{
protected:

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;
    using T::m_oMatrix;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorDecoder));
        m_codec->initialize();
        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_FeatureVectorDecoder_InputParameterId_MemoryBufferToDecode));
        m_oMatrix.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_FeatureVectorDecoder_OutputParameterId_Matrix));

        return true;
    }

public:
    using T::initialize;
    using T::uninitialize;

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_FeatureVectorDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_FeatureVectorDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_FeatureVectorDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TFeatureVectorDecoder : public TFeatureVectorDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>
{
    using TFeatureVectorDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::m_boxAlgorithm;
public:
    using TFeatureVectorDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::uninitialize;

    TFeatureVectorDecoder() { }

    TFeatureVectorDecoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TFeatureVectorDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
