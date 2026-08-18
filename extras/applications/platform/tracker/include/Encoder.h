#pragma once

#include <string.h> // memcpy() on Ubuntu

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "EncodedChunk.h"

#include "BoxAlgorithmProxy.h"
#include "Chunk.h"

#include "Stream.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class EncoderBase
 * \brief Base, non-typed abstract class for encoders
 * \author J. T. Lindgren
 *
 */
class EncoderBase
{
public:
    virtual ~EncoderBase() { }

    // Encodes the current chunk of the attached stream (in the derived class)
    // chkType will contain details about the specific chunk type (since EncodedChunk doesn't show it explicitly)
    virtual bool encode(EncodedChunk& chk, EChunkType& chkType) = 0;

    // Advance all timestamps of the encoded chunks by this amount
    // Used for catenating multiple tracks
    virtual bool setEncodeOffset(CTime newOffset) = 0;
};

// Fwd declare, realized by Codecs*
template <class T>
class EncoderImpl;

/**
 * \class Encoder
 * \brief Encoder class taking in typed Stream and it into EBML-containing encoded chunks.
 * \details The constructor is passed a Stream to encode from. The encode() call pulls the next chunk from the Stream and returns an EncodedChunk.
 * \author J. T. Lindgren
 *
 */
template <class T>
class Encoder final : public EncoderBase
{
public:
    Encoder(const Kernel::IKernelContext& ctx, Stream<T>& stream) : m_stream(stream), m_encoder(ctx) { }

    // Encode the current chunk. The caller must step the stream.
    bool encode(EncodedChunk& chk, EChunkType& chkType) override
    {
        const size_t position = m_stream.getPosition();

        if (position == size_t(-1)) {
            //          std::cout << "Encode header\n";
            chkType = EChunkType::Header;
            return m_encoder.encodeHeader(chk, m_stream.getHeader());
        }
        if (position < m_stream.getChunkCount()) {
            chkType = EChunkType::Buffer;

            const bool retVal = m_encoder.encodeBuffer(chk, *m_stream.getChunk(position));
            //  std::cout << "Encoder gave " << chk.bufferData.size() << "\n";

            return retVal;
        }
        if (position == m_stream.getChunkCount()) {
            chkType = EChunkType::End;

            auto end = m_stream.getEnd();

            // Some streams might not have an end in the file. Here we force one to be present.
            // @fixme move to load
            if (end.m_StartTime == CTime::max() || end.m_EndTime == CTime::max()) {
                // @note modifies the stream
                end.m_StartTime = m_stream.getDuration();
                end.m_EndTime   = end.m_StartTime;
            }

            return m_encoder.encodeEnd(chk, end);
        }
        return false;
    }

    bool setEncodeOffset(CTime newOffset) override { return m_encoder.setEncodeOffset(newOffset); }

protected:
    Stream<T>& m_stream;
    EncoderImpl<T> m_encoder;
};

/**
 * \class EncoderAdapter
 * \brief Adapter to use implementations from Toolkit
 *
 * Note that this class does not set the encoded chunk times or index, these are currently 
 * handled outside the encoder.
 *
 * \author J. T. Lindgren
 *
 */
template <class T, class TToolkitEncoder>
class EncoderAdapter : protected Contexted
{
public:
    explicit EncoderAdapter(const Kernel::IKernelContext& ctx) : Contexted(ctx), m_box(ctx), m_impl(m_box, 0) {}
    ~EncoderAdapter() override { }

    // If chunk timestamps are adjusted, this function should be used to adjust any possible
    // timestamps inside the chunks accordingly
    bool setEncodeOffset(const CTime newOffset)
    {
        m_offset = newOffset;
        return true;
    }

    bool encodeHeader(EncodedChunk& chk, const typename T::Header& inputHdr)
    {
        m_box.dummy.setOutputChunkSize(0, 0, true);
        chk.m_StartTime = inputHdr.m_StartTime + m_offset;
        chk.m_EndTime   = inputHdr.m_EndTime + m_offset;

        encodeHeaderImpl(inputHdr);

        fillChunk(chk);

        return true;
    }

    bool encodeBuffer(EncodedChunk& chk, const typename T::Buffer& inputBuf)
    {
        m_box.dummy.setOutputChunkSize(0, 0, true);
        chk.m_StartTime = inputBuf.m_StartTime + m_offset;
        chk.m_EndTime   = inputBuf.m_EndTime + m_offset;

        encodeBufferImpl(inputBuf);

        fillChunk(chk);

        return true;
    }

    bool encodeEnd(EncodedChunk& chk, const typename T::End& inputEnd)
    {
        m_box.dummy.setOutputChunkSize(0, 0, true);

        chk.m_StartTime = inputEnd.m_StartTime + m_offset;
        chk.m_EndTime   = inputEnd.m_EndTime + m_offset;

        encodeEndImpl(inputEnd);

        fillChunk(chk);

        return true;
    }

protected:
    // These two must be written as specializations (Codec*cpp)
    bool encodeHeaderImpl(const typename T::Header& hdr);
    bool encodeBufferImpl(const typename T::Buffer& buf);

    bool encodeEndImpl(const typename T::End& /*end*/) { return m_impl.encodeEnd(); }

    bool fillChunk(EncodedChunk& chk)
    {
        const size_t chunkSize = size_t(m_box.dummy.getOutputChunkSize(0));
        chk.m_Buffer.resize(chunkSize);
        if (chunkSize > 0) { memcpy(&chk.m_Buffer[0], m_box.dummy.getOutputChunkBuffer(0), chunkSize); }
        return true;
    }

    BoxAlgorithmProxy m_box;

    TToolkitEncoder m_impl;

    CTime m_offset = CTime::min();
};
}  // namespace Tracker
}  // namespace OpenViBE
