#include "ovpCBoxAlgorithmGenericStreamWriter.h"

#include <fs/Files.h>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

CBoxAlgorithmGenericStreamWriter::CBoxAlgorithmGenericStreamWriter() : m_writer(*this) {}

bool CBoxAlgorithmGenericStreamWriter::initialize()
{
    m_filename             = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const bool compression = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    if (compression) { OV_WARNING_K("Impossible to use compression as it is not yet implemented"); }

    return true;
}

bool CBoxAlgorithmGenericStreamWriter::uninitialize()
{
    if (m_file.is_open()) { m_file.close(); }
    return true;
}

bool CBoxAlgorithmGenericStreamWriter::generateFileHeader()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    m_swap.setSize(0, true);

    m_writerHelper.connect(&m_writer);

    m_writerHelper.openChild(EBML_Identifier_Header);
    m_writerHelper.openChild(EBML_Identifier_DocType);
    m_writerHelper.setStr("OpenViBE_Stream_File");
    m_writerHelper.closeChild();

    m_writerHelper.openChild(EBML_Identifier_EBMLVersion);
    m_writerHelper.setUInt(1);
    m_writerHelper.closeChild();

    m_writerHelper.openChild(EBML_Identifier_EBMLIdLength);
    m_writerHelper.setUInt(10);
    m_writerHelper.closeChild();
    m_writerHelper.closeChild();

    m_writerHelper.openChild(OVP_NodeId_OpenViBEStream_Header);
    m_writerHelper.openChild(OVP_NodeId_OpenViBEStream_Header_Compression);
    m_writerHelper.setUInt(0 /* compression flag */);
    m_writerHelper.closeChild();
    for (size_t i = 0; i < boxContext.getInputCount(); ++i)
    {
        CIdentifier id;
        boxContext.getInputType(i, id);

        m_writerHelper.openChild(OVP_NodeId_OpenViBEStream_Header_StreamType);
        m_writerHelper.setUInt(id.id());
        m_writerHelper.closeChild();
    }
    m_writerHelper.closeChild();
    m_writerHelper.disconnect();

    FS::Files::openOFStream(m_file, m_filename.toASCIIString(), std::ios::binary | std::ios::trunc);

    OV_ERROR_UNLESS_KRF(m_file.good(), "Error opening file [" << m_filename << "] for writing", Kernel::ErrorType::BadFileWrite);

    m_file.write(reinterpret_cast<const char*>(m_swap.getDirectPointer()), std::streamsize(m_swap.getSize()));

    m_isHeaderGenerate = true;
    return true;
}

bool CBoxAlgorithmGenericStreamWriter::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmGenericStreamWriter::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();

    if (!m_isHeaderGenerate) { if (!generateFileHeader()) { return false; } }

    m_swap.setSize(0, true);

    for (size_t i = 0; i < nInput; ++i)
    {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j)
        {
            m_writerHelper.connect(&m_writer);
            m_writerHelper.openChild(OVP_NodeId_OpenViBEStream_Buffer);
            m_writerHelper.openChild(OVP_NodeId_OpenViBEStream_Buffer_StreamIndex);
            m_writerHelper.setUInt(i);
            m_writerHelper.closeChild();
            m_writerHelper.openChild(OVP_NodeId_OpenViBEStream_Buffer_StartTime);
            m_writerHelper.setUInt(boxContext.getInputChunkStartTime(i, j));
            m_writerHelper.closeChild();
            m_writerHelper.openChild(OVP_NodeId_OpenViBEStream_Buffer_EndTime);
            m_writerHelper.setUInt(boxContext.getInputChunkEndTime(i, j));
            m_writerHelper.closeChild();
            m_writerHelper.openChild(OVP_NodeId_OpenViBEStream_Buffer_Content);
            m_writerHelper.setBinary(boxContext.getInputChunk(i, j)->getDirectPointer(), boxContext.getInputChunk(i, j)->getSize());
            m_writerHelper.closeChild();
            m_writerHelper.closeChild();
            m_writerHelper.disconnect();
            boxContext.markInputAsDeprecated(i, j);
        }
    }

    if (m_swap.getSize() != 0)
    {
        m_file.write(reinterpret_cast<const char*>(m_swap.getDirectPointer()), std::streamsize(m_swap.getSize()));
        OV_ERROR_UNLESS_KRF(m_file.good(), "Error opening file [" << m_filename << "] for writing", Kernel::ErrorType::BadFileWrite);
    }

    return true;
}

void CBoxAlgorithmGenericStreamWriter::write(const void* buffer, const size_t size) { m_swap.append(reinterpret_cast<const uint8_t*>(buffer), size); }

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
