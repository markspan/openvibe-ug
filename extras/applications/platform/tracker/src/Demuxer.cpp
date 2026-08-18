//
// @note some ov files which have lots of stimulation chunks take long time to import
// when launching tracker from visual studio. this is probably due to memory allocation, similar to slow simple dsp grammar parsing.
//
#include "Demuxer.h"

#include <iostream>
#include <thread>
#include <deque>
#include <vector>

#include "../../../../plugins/processing/file-io/src/ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include "Stream.h"

#include "CodecFactory.h"

namespace OpenViBE {
namespace Tracker {

bool Demuxer::initialize()
{
    // log() << Kernel::LogLevel_Info << "Source: Initializing with "   << signalFile << "\n";

    m_chunksSent = 0;
    m_pending    = false;
    m_target.setSource(m_origin.getSource());
    return true;
}

bool Demuxer::uninitialize() const
{
    log() << Kernel::LogLevel_Info << "Source: Uninitializing\n";
    m_origin.uninitialize();
    return true;
}

// bool Demuxer::pullChunk(MemoryBufferWithType& output)
bool Demuxer::step()
{
    // log() << Kernel::LogLevel_Info << "Source: Trying to pull a chunk\n";

    while (!m_origin.isEOF()) {
        while (!m_origin.isEOF() && m_reader.getCurrentNodeID() == EBML::CIdentifier()) {
            std::vector<uint8_t> bytes;
            m_origin.read(bytes, 1);

            //OV_ERROR_UNLESS_KRF(s == 1 || justStarted, "Unexpected EOF in " << m_filename, Kernel::ErrorType::BadParsing);
            if (!bytes.empty()) { m_reader.processData(&bytes[0], bytes.size()); }
        }
        if (!m_origin.isEOF() && m_reader.getCurrentNodeSize() != 0) {
            std::vector<uint8_t> bytes;
            m_origin.read(bytes, size_t(m_reader.getCurrentNodeSize()));

            //OV_ERROR_UNLESS_KRF(s == m_swap.getSize(), "Unexpected EOF in " << m_filename, Kernel::ErrorType::BadParsing);

            m_pendingChunk.m_Buffer.resize(0);
            m_pendingChunk.m_StartTime   = CTime::max();
            m_pendingChunk.m_EndTime     = CTime::max();
            m_pendingChunk.m_StreamIndex = std::numeric_limits<uint32_t>::max();

            m_reader.processData(&bytes[0], bytes.size());
        }

        if (m_pending) {
            // We have dada
            // log() << Kernel::LogLevel_Info << "Source: Found a chunk, queueing\n";
            const size_t streamIndex = m_pendingChunk.m_StreamIndex;

            m_pending = false;

            const StreamPtr stream = m_target.getStream(streamIndex);
            if (!stream) {
                log() << Kernel::LogLevel_Error << "Error: Trying to decode stream without creating it first (buffer before header in EBML?)\n";
                return false;
            }
            m_decoders[streamIndex]->decode(m_pendingChunk);

            return true;
        }
    }

    if (m_origin.isEOF()) { log() << Kernel::LogLevel_Trace << "Source file EOF reached\n"; }
    else { log() << Kernel::LogLevel_Warning << "Issue with source file\n"; }

    return false;
}

bool Demuxer::isMasterChild(const EBML::CIdentifier& identifier)
{
    if (identifier == EBML_Identifier_Header) { return true; }
    if (identifier == OVP_NodeId_OpenViBEStream_Header) { return true; }
    if (identifier == OVP_NodeId_OpenViBEStream_Header_Compression) { return false; }
    if (identifier == OVP_NodeId_OpenViBEStream_Header_StreamType) { return false; }
    if (identifier == OVP_NodeId_OpenViBEStream_Buffer) { return true; }
    if (identifier == OVP_NodeId_OpenViBEStream_Buffer_StreamIndex) { return false; }
    if (identifier == OVP_NodeId_OpenViBEStream_Buffer_StartTime) { return false; }
    if (identifier == OVP_NodeId_OpenViBEStream_Buffer_EndTime) { return false; }
    if (identifier == OVP_NodeId_OpenViBEStream_Buffer_Content) { return false; }
    return false;
}

void Demuxer::openChild(const EBML::CIdentifier& identifier)
{
    m_nodes.push(identifier);

    EBML::CIdentifier& top = m_nodes.top();

    if (top == EBML_Identifier_Header) { m_hasEBMLHeader = true; }
    if (top == OVP_NodeId_OpenViBEStream_Header) {
        if (!m_hasEBMLHeader) {
            //this->getLogManager() << Kernel::LogLevel_Info << "The file " << m_filename << " uses an outdated (but still compatible) version of the .ov file format\n";
        }
    }
    if (top == OVP_NodeId_OpenViBEStream_Header) {
        m_streamIdxToOutputIdxs.clear();
        m_streamIdxToTypeIDs.clear();
    }
}

void Demuxer::processChildData(const void* buffer, const size_t size)
{
    EBML::CIdentifier& top = m_nodes.top();

    // Uncomment this when ebml version will be used
    //if(top == EBML_Identifier_EBMLVersion) { const uint64_t versionNumber=(uint64_t)m_readerHelper.getUInt(buffer, size); }

    if (top == OVP_NodeId_OpenViBEStream_Header_Compression) {
        //if (m_readerHelper.getUInt(buffer, size) != 0) { OV_WARNING_K("Impossible to use compression as it is not yet implemented"); }
    }
    else if (top == OVP_NodeId_OpenViBEStream_Header_StreamType) {
        const uint64_t typeID = m_readerHelper.getUInt(buffer, size);
        const size_t index    = m_target.getNumStreams();
        m_target.createStream(index, typeID);
        DecoderBase* decoder = CodecFactory::getDecoder(m_kernelCtx, *m_target.getStream(index));
        m_decoders.push_back(decoder);
    }
    else if (top == OVP_NodeId_OpenViBEStream_Buffer_StreamIndex) {
        // @note if trying to do -1 to  map to [0,...] convention, something breaks
        m_pendingChunk.m_StreamIndex = size_t(m_readerHelper.getUInt(buffer, size));
        //log() << Kernel::LogLevel_Info << "Run into index " << m_pendingChunk.streamIndex << "\n";
    }
    else if (top == OVP_NodeId_OpenViBEStream_Buffer_StartTime) { m_pendingChunk.m_StartTime = m_readerHelper.getUInt(buffer, size); }
    else if (top == OVP_NodeId_OpenViBEStream_Buffer_EndTime) { m_pendingChunk.m_EndTime = m_readerHelper.getUInt(buffer, size); }
    else if (top == OVP_NodeId_OpenViBEStream_Buffer_Content) {
        m_pendingChunk.m_Buffer.resize(size_t(size));
        memcpy(&m_pendingChunk.m_Buffer[0], reinterpret_cast<const uint8_t*>(buffer), size_t(size));
    }
}

void Demuxer::closeChild()
{
    EBML::CIdentifier& top = m_nodes.top();

    if (top == OVP_NodeId_OpenViBEStream_Header) { } // Assign file streams to outputs here

    if (top == OVP_NodeId_OpenViBEStream_Buffer) {
        m_pending = ((m_pendingChunk.m_StreamIndex != std::numeric_limits<size_t>::max()) && (m_pendingChunk.m_StartTime != CTime::max())
                     && (m_pendingChunk.m_EndTime != CTime::max()));
    }

    m_nodes.pop();
}

}  // namespace Tracker
}  // namespace OpenViBE
