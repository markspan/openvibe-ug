#include "ovpCBoxAlgorithmOSCController.h"

// #include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {

bool CBoxAlgorithmOSCController::initialize()
{
    const CString address = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const uint64_t port   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_oscAddress          = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    const char* tmp = m_oscAddress.toASCIIString();
    if (!tmp || !tmp[0] || tmp[0] != '/')
    {
        this->getLogManager() << Kernel::LogLevel_Error << "OSC Address must start with a '/'\n";
        return false;
    }

    // Connect the socket 
    const std::string str = std::string(address.toASCIIString());
    m_udpSocket.connectTo(str, uint32_t(port));

    if (!m_udpSocket.isOk())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Error connecting to socket\n";
        return false;
    }

    // Get appropriate decoder
    CIdentifier streamType;
    this->getStaticBoxContext().getInputType(0, streamType);

    m_decoder = nullptr;
    if (this->getTypeManager().isDerivedFromStream(streamType,OV_TypeId_StreamedMatrix))
    {
        m_decoder = &this->getAlgorithmManager().getAlgorithm(
            this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixDecoder));
    }
    else if (streamType == OV_TypeId_Stimulations)
    {
        m_decoder = &this->getAlgorithmManager().getAlgorithm(
            this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    }
    else
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Unsupported type\n";
        return false;
    }
    m_decoder->initialize();

    return true;
}

bool CBoxAlgorithmOSCController::uninitialize()
{
    if (m_udpSocket.isOk()) { m_udpSocket.close(); }

    if (m_decoder)
    {
        this->getAlgorithmManager().releaseAlgorithm(*m_decoder);
        m_decoder = nullptr;
    }

    return true;
}

bool CBoxAlgorithmOSCController::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmOSCController::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    CIdentifier streamType;
    this->getStaticBoxContext().getInputType(0, streamType);

    oscpkt::PacketWriter pw;
    oscpkt::Message msg;
    bool haveData = false;

    for (size_t j = 0; j < boxContext.getInputChunkCount(0); ++j)
    {
        if (this->getTypeManager().isDerivedFromStream(streamType,OV_TypeId_StreamedMatrix))
        {
            Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer(
                m_decoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_InputParameterId_MemoryBufferToDecode));
            Kernel::TParameterHandler<const CMatrix*> op_pMatrix(
                m_decoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));

            ip_buffer = boxContext.getInputChunk(0, j);
            m_decoder->process();

            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedBuffer))
            {
                // Check that the dimensions are acceptable
                const CMatrix* matrix = op_pMatrix;
                if (matrix->getDimensionCount() < 1 || matrix->getDimensionCount() > 2)
                {
                    this->getLogManager() << Kernel::LogLevel_Error << "Only matrixes of 1 or 2 dimensions are supported\n";
                    return false;
                }
                if (matrix->getDimensionCount() == 2 && matrix->getDimensionSize(0) != 1)
                {
                    this->getLogManager() << Kernel::LogLevel_Error << "The matrix should have only 1 channel. Use e.g. Channel Selector to prune\n";
                    return false;
                }

                if (!haveData)
                {
                    haveData = true;
                    pw.startBundle();
                }

                for (size_t k = 0; k < matrix->getBufferElementCount(); ++k)
                {
                    const float inputVal = float(matrix->getBuffer()[k]);
                    pw.addMessage(msg.init(m_oscAddress.toASCIIString()).pushFloat(inputVal));
                    // std::cout << "Add float " << inputVal << "\n";
                }
            }
        }
        else if (streamType == OV_TypeId_Stimulations)
        {
            Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer(
                m_decoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
            const Kernel::TParameterHandler<const CStimulationSet*> op_pStimulationSet(
                m_decoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

            ip_buffer = boxContext.getInputChunk(0, j);
            m_decoder->process();

            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer))
            {
                if (!haveData)
                {
                    haveData = true;
                    pw.startBundle();
                }

                for (size_t k = 0; k < op_pStimulationSet->size(); ++k)
                {
                    const uint64_t stimulus = op_pStimulationSet->getId(k);
                    pw.addMessage(msg.init(m_oscAddress.toASCIIString()).pushInt32(int32_t(stimulus)));
                    // std::cout << "Add stimulus " << stimulus << "\n";
                }
            }
        }
        else
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Unknown stream type\n";
            return false;
        }

        boxContext.markInputAsDeprecated(0, j);
    }

    if (haveData)
    {
        pw.endBundle();
        if (!m_udpSocket.sendPacket(pw.packetData(), pw.packetSize())) { this->getLogManager() << Kernel::LogLevel_Warning << "Error sending out UDP packet\n"; }
    }
    return true;
}
}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE
