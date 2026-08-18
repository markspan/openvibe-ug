#pragma once

#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "Encoder.h"
#include "Decoder.h"

#include "TypeError.h"

// This macro makes specializations by instantiating an adapter that handles a type with
// the actual encoder and decoder from the toolkit 
#define CODEC_IMPL_VIA_TOOLKIT(TYPENAME, TOOLKITENCODER, TOOLKITDECODER) \
    template<> class DecoderImpl<TYPENAME> : public DecoderAdapter< TYPENAME, OpenViBE::Toolkit::TOOLKITDECODER<BoxAlgorithmProxy> > \
    { \
    public: \
        DecoderImpl(const OpenViBE::Kernel::IKernelContext& ctx) : DecoderAdapter(ctx) { } \
    };\
    template<> class EncoderImpl<TYPENAME> : public EncoderAdapter< TYPENAME, OpenViBE::Toolkit::TOOLKITENCODER<BoxAlgorithmProxy> > \
    { \
    public: \
        EncoderImpl(const OpenViBE::Kernel::IKernelContext& ctx) : EncoderAdapter(ctx) { } \
    };

namespace OpenViBE {
namespace Tracker {
/**
 * \class DecoderImpl
 * \brief Fallback class for invalid situations
 * \author J. T. Lindgren
 *
 */
template <>
class DecoderImpl<TypeError>
{
public:
    explicit DecoderImpl(const Kernel::IKernelContext& /*ctx*/) { }
    virtual ~DecoderImpl() = default;

    virtual bool decode(const EncodedChunk& /*source*/) { return false; }
    virtual bool getHeader(TypeError::Header& /*h*/) { return false; }
    virtual bool getBuffer(TypeError::Buffer& /*b*/) { return false; }
    virtual bool getEnd(TypeError::End& /*e*/) { return false; }
    virtual bool isHeaderReceived() { return false; }
    virtual bool isBufferReceived() { return false; }
    virtual bool isEndReceived() { return false; }
};

/**
 * \class EncoderImpl
 * \brief Fallback class for invalid situations
 * \author J. T. Lindgren
 *
 */
template <>
class EncoderImpl<TypeError>
{
public:
    explicit EncoderImpl(const Kernel::IKernelContext& /*ctx*/) { }
    virtual ~EncoderImpl() = default;

    bool encodeHeader(EncodedChunk& /*chk*/, const TypeError::Header& /*hdr*/) { return false; }
    bool encodeBuffer(EncodedChunk& /*chk*/, const TypeError::Buffer& /*buf*/) { return false; }
    bool encodeEnd(EncodedChunk& /*chk*/, const TypeError::End& /*end*/) { return false; }
    bool setEncodeOffset(CTime /*newOffset*/) { return false; }
};
}  // namespace Tracker
}  // namespace OpenViBE
