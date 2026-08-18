#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TSignalEncoderLocal : public T
{
protected:
    //The signal stream is a streamed matrix plus a sampling rate
    Kernel::TParameterHandler<uint64_t> m_iSampling;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_buffer;
    using T::m_iMatrix;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
        m_codec->initialize();
        m_iMatrix.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Matrix));
        m_iSampling.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling));
        m_buffer.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));

        return true;
    }

public:
    //again... we propagate initialize from upperclass.
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_iMatrix.uninitialize();
        m_iSampling.uninitialize();
        m_buffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<uint64_t>& getInputSamplingRate() { return m_iSampling; }

protected:

    /*
    The methods specific to the Signal encoder (overriding the TStreamedMatrixEncoderLocal implementations):
    */
    bool encodeHeaderImpl() { return m_codec->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader); }
    bool encodeBufferImpl() { return m_codec->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer); }
    bool encodeEndImpl() { return m_codec->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeEnd); }
};

/*
The Signal encoder can be instanciated easily through this class.
You just need one template class : the box (T).
*/
template <class T>
class TSignalEncoder : public TSignalEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>
{
    using TSignalEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>::m_boxAlgorithm;
public:
    using TSignalEncoderLocal<TStreamedMatrixEncoderLocal<TEncoder<T>>>::uninitialize;

    TSignalEncoder() { }

    TSignalEncoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TSignalEncoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
