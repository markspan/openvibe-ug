#include "BoxAcquisitionClientLegacy.h"
#include <limits>

namespace OpenViBE {
namespace Plugins {
namespace Acquisition {

uint64_t CBoxAlgorithmAcquisitionClientLegacy::getClockFrequency() { return 64LL << 32; }

bool CBoxAlgorithmAcquisitionClientLegacy::initialize()
{
    m_decoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_AcquisitionDecoder));

    m_decoder->initialize();

    ip_acquisitionBuffer.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_AcquisitionDecoder_InputParameterId_MemoryBufferToDecode));
    op_bufferDuration.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_BufferDuration));
    op_experimentInfoBuffer.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_ExperimentInfoStream));
    op_signalBuffer.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_SignalStream));
    op_stimulationBuffer.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_StimulationStream));
    op_channelLocalisationBuffer.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_ChannelLocalisationStream));
    op_channelUnitsBuffer.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_AcquisitionDecoder_OutputParameterId_ChannelUnitsStream));

    m_lastStartTime    = 0;
    m_lastEndTime      = 0;
    m_connectionClient = nullptr;

    if (getStaticBoxContext().getOutputCount() < 5)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Code expects at least 5 box outputs. Did you update the box?\n";
        return false;
    }

    return true;
}

bool CBoxAlgorithmAcquisitionClientLegacy::uninitialize()
{
    if (m_connectionClient)
    {
        m_connectionClient->close();
        m_connectionClient->release();
        m_connectionClient = nullptr;
    }

    op_channelUnitsBuffer.uninitialize();
    op_channelLocalisationBuffer.uninitialize();
    op_stimulationBuffer.uninitialize();
    op_signalBuffer.uninitialize();
    op_experimentInfoBuffer.uninitialize();
    op_bufferDuration.uninitialize();
    ip_acquisitionBuffer.uninitialize();

    m_decoder->uninitialize();

    getAlgorithmManager().releaseAlgorithm(*m_decoder);

    m_decoder = nullptr;

    return true;
}

bool CBoxAlgorithmAcquisitionClientLegacy::processClock(Kernel::CMessageClock& /*msg*/)
{
    if (!m_connectionClient)
    {
        CString name      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
        const size_t port = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
        if (name.length() == 0)
        {
            this->getLogManager() << Kernel::LogLevel_Warning <<
                    "Empty server name, please set it to a correct value or set AcquisitionServer_HostName in config files. Defaulting to \"localhost\".\n";
            name = "localhost";
        }
        if (port == std::numeric_limits<size_t>::max() || port == std::numeric_limits<size_t>::min())
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Invalid value for port : " << port <<
                    ". Please set the port to a positive non-zero integer value.\n";
            return false;
        }

        m_connectionClient = Socket::createConnectionClient();
        m_connectionClient->connect(name, port);
        if (!m_connectionClient->isConnected())
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Could not connect to server " << name << ":" << port <<
                    ". Make sure the server is running and in Play state.\n";
            return false;
        }
    }

    if (m_connectionClient && m_connectionClient->isReadyToReceive() /* && getPlayerContext().getCurrentTime()>m_lastChunkEndTime */)
    {
        getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    }

    return true;
}

bool CBoxAlgorithmAcquisitionClientLegacy::process()
{
    if (!m_connectionClient || !m_connectionClient->isConnected()) { return false; }

    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    op_experimentInfoBuffer      = boxContext.getOutputChunk(0);
    op_signalBuffer              = boxContext.getOutputChunk(1);
    op_stimulationBuffer         = boxContext.getOutputChunk(2);
    op_channelLocalisationBuffer = boxContext.getOutputChunk(3);
    op_channelUnitsBuffer        = boxContext.getOutputChunk(4);

    while (m_connectionClient->isReadyToReceive())
    {
        size_t size = 0;
        if (!m_connectionClient->receiveBufferBlocking(&size, sizeof(size)))
        {
            getLogManager() << Kernel::LogLevel_Error << "Could not receive memory buffer size from the server. Is the server on 'Play'?\n";
            return false;
        }
        if (!ip_acquisitionBuffer->setSize(size, true))
        {
            getLogManager() << Kernel::LogLevel_Error << "Could not re allocate memory buffer with size " << size << "\n";
            return false;
        }
        if (!m_connectionClient->receiveBufferBlocking(ip_acquisitionBuffer->getDirectPointer(), size))
        {
            getLogManager() << Kernel::LogLevel_Error << "Could not receive memory buffer content of size " << size << "\n";
            return false;
        }

        m_decoder->process();


        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_AcquisitionDecoder_OutputTriggerId_ReceivedHeader)
            || m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_AcquisitionDecoder_OutputTriggerId_ReceivedBuffer)
            || m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_AcquisitionDecoder_OutputTriggerId_ReceivedEnd))
        {
            boxContext.markOutputAsReadyToSend(0, m_lastStartTime, m_lastEndTime);
            boxContext.markOutputAsReadyToSend(1, m_lastStartTime, m_lastEndTime);
            boxContext.markOutputAsReadyToSend(2, m_lastStartTime, m_lastEndTime);
            if (op_channelLocalisationBuffer->getSize() > 0) { boxContext.markOutputAsReadyToSend(3, m_lastStartTime, m_lastEndTime); }
            else { boxContext.setOutputChunkSize(3, 0, true); }

            if (op_channelUnitsBuffer->getSize() > 0) { boxContext.markOutputAsReadyToSend(4, m_lastStartTime, m_lastEndTime); }
            else { boxContext.setOutputChunkSize(4, 0, true); }
            m_lastStartTime = m_lastEndTime;
            m_lastEndTime += op_bufferDuration;
            // @todo ?
            // const double latency=CTime(m_lastChunkEndTime).toSeconds() - CTime(this->getPlayerContext().getCurrentTime()).toSeconds();
            const double latency = double(int64_t(m_lastEndTime - this->getPlayerContext().getCurrentTime()) / (1LL << 22)) / 1024.0;
            this->getLogManager() << Kernel::LogLevel_Debug << "Acquisition inner latency : " << latency << "\n";
        }
    }

    return true;
}
}  // namespace Acquisition
}  // namespace Plugins
}  // namespace OpenViBE
