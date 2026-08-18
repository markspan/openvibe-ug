#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TFeatureVectorEncoderLocal : public T
{
protected:
    // the feature vector stream is just a streamed matrix with some constraint (dimension = 2).
    // no specific parameter.

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_buffer;
    using T::m_iMatrix;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorEncoder));
        m_codec->initialize();
        m_iMatrix.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_FeatureVectorEncoder_InputParameterId_Matrix));
        m_buffer.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_FeatureVectorEncoder_OutputParameterId_EncodedMemoryBuffer));

        return true;
    }

    bool encodeHeaderImpl() { return m_codec->process(OVP_GD_Algorithm_FeatureVectorEncoder_InputTriggerId_EncodeHeader); }
    bool encodeBufferImpl() { return m_codec->process(OVP_GD_Algorithm_FeatureVectorEncoder_InputTriggerId_EncodeBuffer); }
    bool encodeEndImpl() { return m_codec->process(OVP_GD_Algorithm_FeatureVectorEncoder_InputTriggerId_EncodeEnd); }

public:
    using T::initialize;
    using T::uninitialize;
};

template <class T>
class TFeatureVectorEncoder : public TFeatureVectorEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>
{
    using TFeatureVectorEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>::m_boxAlgorithm;
public:
    using TFeatureVectorEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>::uninitialize;

    TFeatureVectorEncoder() { }

    TFeatureVectorEncoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TFeatureVectorEncoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
