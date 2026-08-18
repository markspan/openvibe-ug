#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTStreamedMatrixDecoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TAcquisitionDecoderLocal : public T
{
protected:

    Kernel::TParameterHandler<uint64_t> op_bufferDuration;
    Kernel::TParameterHandler<CMemoryBuffer*> op_experimentInfoStream;
    Kernel::TParameterHandler<CMemoryBuffer*> op_signalStream;
    Kernel::TParameterHandler<CMemoryBuffer*> op_stimulationStream;
    Kernel::TParameterHandler<CMemoryBuffer*> op_channelLocalisationStream;
    Kernel::TParameterHandler<CMemoryBuffer*> op_channelUnitsStream;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_AcquisitionDecoder));
        m_codec->initialize();

        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_AcquisitionDecoder_InputParameterId_MemoryBufferToDecode));

        op_bufferDuration.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_BufferDuration));
        op_experimentInfoStream.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_ExperimentInfoStream));
        op_signalStream.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_SignalStream));
        op_stimulationStream.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_StimulationStream));
        op_channelLocalisationStream.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_ChannelLocalisationStream));
        op_channelUnitsStream.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_ChannelUnitsStream));

        return true;
    }

public:

    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        op_channelUnitsStream.uninitialize();
        op_channelLocalisationStream.uninitialize();
        op_stimulationStream.uninitialize();
        op_signalStream.uninitialize();
        op_experimentInfoStream.uninitialize();
        op_bufferDuration.uninitialize();

        m_iBuffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<uint64_t>& getBufferDuration() { return op_bufferDuration; }
    Kernel::TParameterHandler<CMemoryBuffer*>& getExperimentInfoStream() { return op_experimentInfoStream; }
    Kernel::TParameterHandler<CMemoryBuffer*>& getSignalStream() { return op_signalStream; }
    Kernel::TParameterHandler<CMemoryBuffer*>& getStimulationStream() { return op_stimulationStream; }
    Kernel::TParameterHandler<CMemoryBuffer*>& getChannelLocalisationStream() { return op_channelLocalisationStream; }
    Kernel::TParameterHandler<CMemoryBuffer*>& getChannelUnitsStream() { return op_channelUnitsStream; }

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_AcquisitionDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_AcquisitionDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_AcquisitionDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TAcquisitionDecoder : public TAcquisitionDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>
{
    using TAcquisitionDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::m_boxAlgorithm;

public:
    using TAcquisitionDecoderLocal<TStreamedMatrixDecoderLocal<TDecoder<T>>>::uninitialize;

    TAcquisitionDecoder() { }

    explicit TAcquisitionDecoder(T& boxAlgorithm)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm);
    }

    virtual ~TAcquisitionDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
