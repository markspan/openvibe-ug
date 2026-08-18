#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "BoxAlgorithmProxy.h"
#include "Chunk.h"
#include "EncodedChunk.h"
#include "Stream.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class DecoderBase
 * \brief Base, non-typed abstract class for decoders
 * \details Derived classes can be type-specific
 * \author J. T. Lindgren
 *
 */
class DecoderBase
{
public:
    virtual bool decode(const EncodedChunk& chk) = 0;
    virtual ~DecoderBase() { }
};

// Fwd declare, realized by Codecs*
template <typename T>
class DecoderImpl;

/**
 * \class Decoder
 * \brief Decoder for a specific type T. 
 * \details The constructor is passed a stream to decode to. The decode() call pushes chunks to that stream.
 * \author J. T. Lindgren
 *
 */
template <class T>
class Decoder final : public DecoderBase
{
public:
    Decoder(const Kernel::IKernelContext& ctx, Stream<T>& stream) : m_stream(stream), m_decoder(ctx) { }

    bool decode(const EncodedChunk& chk) override
    {
        m_decoder.decode(chk);
        if (m_decoder.isHeaderReceived()) { return m_decoder.getHeader(m_stream.getHeader()); }
        if (m_decoder.isBufferReceived()) {
            typename T::Buffer* chunk = new typename T::Buffer();
            if (m_decoder.getBuffer(*chunk)) {
                m_stream.push(chunk);
                return true;
            }
            return false;
        }
        if (m_decoder.isEndReceived()) { return m_decoder.getEnd(m_stream.getEnd()); }
        return false;
    }

protected:
    Stream<T>& m_stream;
    DecoderImpl<T> m_decoder;
};

/**
 * \class DecoderAdapter
 * \brief This decoder is a wrapper over the decoders in OpenViBEToolkit. 
 *
 * The toolkit decoders require a box context to run, and do not return class objects.
 * This decoder wrapper addresses those limitations.
 * The usage is to create e.g. Decoder<TypeSignal> dec; and use the member functions to 
 * obtain the chunks with the signal type.
 *
 * \author J. T. Lindgren
 *
 */
template <class T, class TToolkitDecoder>
class DecoderAdapter : protected Contexted
{
public:
    explicit DecoderAdapter(const Kernel::IKernelContext& ctx) : Contexted(ctx), m_box(ctx), m_impl(m_box, 0) {}
    ~DecoderAdapter() override { }

    virtual bool decode(const EncodedChunk& source)
    {
        if (source.m_Buffer.size() == 0) {
            //  std::cout << "Empty chunk received\n";
            return false;
        }
        m_box.dummy.m_InBuffer.setSize(0, true);
        m_box.dummy.m_InBuffer.append(&source.m_Buffer[0], source.m_Buffer.size());
        m_lastStart = source.m_StartTime;
        m_lastEnd   = source.m_EndTime;
        return m_impl.decode(0);
    }

    virtual bool getHeader(typename T::Header& h)
    {
        setTimeStamps(h);
        return getHeaderImpl(h);
    }

    virtual bool getBuffer(typename T::Buffer& b)
    {
        setTimeStamps(b);
        return getBufferImpl(b);
    }

    virtual bool getEnd(typename T::End& e)
    {
        setTimeStamps(e);
        return getEndImpl(e);
    }

    virtual bool isHeaderReceived() { return m_impl.isHeaderReceived(); }
    virtual bool isBufferReceived() { return m_impl.isBufferReceived(); }
    virtual bool isEndReceived() { return m_impl.isEndReceived(); }

protected:
    // These two must be written as specilizations (Codec*cpp)
    bool getHeaderImpl(typename T::Header& h);
    bool getBufferImpl(typename T::Buffer& b);

    static bool getEndImpl(typename T::End& /*e*/) { return true; } // NOP regardless of type

    bool setTimeStamps(Chunk& chunk) const
    {
        chunk.m_StartTime = m_lastStart;
        chunk.m_EndTime   = m_lastEnd;
        return true;
    }

    BoxAlgorithmProxy m_box;

    CTime m_lastStart = CTime::min();
    CTime m_lastEnd   = CTime::min();

    TToolkitDecoder m_impl;
};
}  // namespace Tracker
}  // namespace OpenViBE
