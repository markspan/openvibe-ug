#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTDecoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TStreamedMatrixDecoderLocal : public T
{
protected:

    Kernel::TParameterHandler<CMatrix*> m_oMatrix;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;

    virtual bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixDecoder));
        m_codec->initialize();
        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_InputParameterId_MemoryBufferToDecode));
        m_oMatrix.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));

        return true;
    }

public:
    using T::initialize;

    virtual bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_oMatrix.uninitialize();
        m_iBuffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    virtual Kernel::TParameterHandler<CMatrix*>& getOutputMatrix() { return m_oMatrix; }

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TStreamedMatrixDecoder : public TStreamedMatrixDecoderLocal<TDecoder<T>>
{
    using TStreamedMatrixDecoderLocal<TDecoder<T>>::m_boxAlgorithm;
public:
    using TStreamedMatrixDecoderLocal<TDecoder<T>>::uninitialize;

    TStreamedMatrixDecoder() { }

    TStreamedMatrixDecoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TStreamedMatrixDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
