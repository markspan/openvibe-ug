#include "ovpCEBMLBaseEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

// ________________________________________________________________________________________________________________
//

bool CEBMLBaseEncoder::initialize()
{
    op_buffer.initialize(getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));

    m_writer       = createWriter(m_callbackProxy);
    m_writerHelper = EBML::createWriterHelper();
    m_writerHelper->connect(m_writer);

    return true;
}

bool CEBMLBaseEncoder::uninitialize()
{
    m_writerHelper->disconnect();
    m_writerHelper->release();
    m_writerHelper = nullptr;

    m_writer->release();
    m_writer = nullptr;

    op_buffer.uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CEBMLBaseEncoder::process()
{
    if (isInputTriggerActive(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader))
    {
        m_writerHelper->openChild(OVTK_NodeId_Header);
        m_writerHelper->openChild(OVTK_NodeId_Header_StreamType);
        m_writerHelper->setUInt(0);
        m_writerHelper->closeChild();
        m_writerHelper->openChild(OVTK_NodeId_Header_StreamVersion);
        m_writerHelper->setUInt(0);
        m_writerHelper->closeChild();
        this->processHeader();
        m_writerHelper->closeChild();
        activateOutputTrigger(OVP_Algorithm_EBMLEncoder_OutputTriggerId_MemoryBufferUpdated, true);
    }

    if (isInputTriggerActive(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer))
    {
        m_writerHelper->openChild(OVTK_NodeId_Buffer);
        this->processBuffer();
        m_writerHelper->closeChild();
        activateOutputTrigger(OVP_Algorithm_EBMLEncoder_OutputTriggerId_MemoryBufferUpdated, true);
    }

    if (isInputTriggerActive(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd))
    {
        m_writerHelper->openChild(OVTK_NodeId_End);
        this->processEnd();
        m_writerHelper->closeChild();
        activateOutputTrigger(OVP_Algorithm_EBMLEncoder_OutputTriggerId_MemoryBufferUpdated, true);
    }

    return true;
}

// ________________________________________________________________________________________________________________
//

void CEBMLBaseEncoder::write(const void* buffer, const size_t size)
{
    const size_t currentBufferSize = op_buffer->getSize();
    op_buffer->setSize(currentBufferSize + size, false);
    memcpy(op_buffer->getDirectPointer() + currentBufferSize, buffer, size);
}
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
