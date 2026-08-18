#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTEncoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TStreamedMatrixEncoderLocal : public T
{
protected:
    //specific attribute : a matrix handler
    Kernel::TParameterHandler<CMatrix*> m_iMatrix;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_buffer;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder));
        m_codec->initialize();
        m_iMatrix.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix));
        m_buffer.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_OutputParameterId_EncodedMemoryBuffer));

        return true;
    }

public:
    // we propagate the visiblity of TCodec::initialize
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_iMatrix.uninitialize();
        m_buffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<CMatrix*>& getInputMatrix() { return m_iMatrix; }

protected:

    /*
    The methods specific to the Streamed Matrix encoder :
    */
    bool encodeHeaderImpl() { return m_codec->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeHeader); }
    bool encodeBufferImpl() { return m_codec->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeBuffer); }
    bool encodeEndImpl() { return m_codec->process(OVP_GD_Algorithm_StreamedMatrixEncoder_InputTriggerId_EncodeEnd); }
};

/*
This class provides an access to the Local class.
It makes a lot easier the declaration of a Streamed Matrix encoder, as you don't have to specify any more template than the Box class (T).
*/
template <class T>
class TStreamedMatrixEncoder : public TStreamedMatrixEncoderLocal<TEncoder<T>>
{
    using TStreamedMatrixEncoderLocal<TEncoder<T>>::m_boxAlgorithm;
public:
    using TStreamedMatrixEncoderLocal<TEncoder<T>>::uninitialize;

    TStreamedMatrixEncoder() { }

    TStreamedMatrixEncoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TStreamedMatrixEncoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
