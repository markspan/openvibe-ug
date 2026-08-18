#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"
#include "ovtkTStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TSpectrumEncoderLocal : public T
{
protected:

    Kernel::TParameterHandler<CMatrix*> m_iFrequencyAbscissa;
    Kernel::TParameterHandler<uint64_t> m_iSampling;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_buffer;
    using T::m_iMatrix;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
        m_codec->initialize();
        m_iMatrix.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_Matrix));
        m_iFrequencyAbscissa.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_FrequencyAbscissa));
        m_iSampling.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_Sampling));
        m_buffer.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_SpectrumEncoder_OutputParameterId_EncodedMemoryBuffer));


        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_iMatrix.uninitialize();
        m_iFrequencyAbscissa.uninitialize();
        m_iSampling.uninitialize();
        m_buffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<uint64_t>& getInputSamplingRate() { return m_iSampling; }
    Kernel::TParameterHandler<CMatrix*>& getInputFrequencyAbscissa() { return m_iFrequencyAbscissa; }

    size_t getInputFrequencyAbscissaCount() { return m_iFrequencyAbscissa->getDimensionSize(0); }


protected:
    bool encodeHeaderImpl() { return m_codec->process(OVP_GD_Algorithm_SpectrumEncoder_InputTriggerId_EncodeHeader); }
    bool encodeBufferImpl() { return m_codec->process(OVP_GD_Algorithm_SpectrumEncoder_InputTriggerId_EncodeBuffer); }
    bool encodeEndImpl() { return m_codec->process(OVP_GD_Algorithm_SpectrumEncoder_InputTriggerId_EncodeEnd); }
};

template <class T>
class TSpectrumEncoder : public TSpectrumEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>
{
    using TSpectrumEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>::m_boxAlgorithm;
public:
    using TSpectrumEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>::uninitialize;

    TSpectrumEncoder() { }

    TSpectrumEncoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TSpectrumEncoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
