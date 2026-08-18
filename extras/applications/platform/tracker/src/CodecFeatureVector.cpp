// @todo this is identical to CodecFeatureMatrix. Refactor?

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeFeatureVector.h"

#include "Decoder.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {
template <>
bool DecoderAdapter<TypeFeatureVector, Toolkit::TFeatureVectorDecoder<BoxAlgorithmProxy>>::getHeaderImpl(TypeFeatureVector::Header& h)
{
    CMatrix* decoded = m_impl.getOutputMatrix();
    h.m_Header.copy(*decoded);

    return true;
}

template <>
bool DecoderAdapter<TypeFeatureVector, Toolkit::TFeatureVectorDecoder<BoxAlgorithmProxy>>::getBufferImpl(TypeFeatureVector::Buffer& b)
{
    const CMatrix* decoded = m_impl.getOutputMatrix();
    b.m_buffer.copy(*decoded);

    return true;
}

template <>
bool EncoderAdapter<TypeFeatureVector, Toolkit::TFeatureVectorEncoder<BoxAlgorithmProxy>>::encodeHeaderImpl(const TypeFeatureVector::Header& hdr)
{
    CMatrix* buffer = m_impl.getInputMatrix();
    buffer->copy(hdr.m_Header);

    return m_impl.encodeHeader();
}

template <>
bool EncoderAdapter<TypeFeatureVector, Toolkit::TFeatureVectorEncoder<BoxAlgorithmProxy>>::encodeBufferImpl(const TypeFeatureVector::Buffer& buf)
{
    CMatrix* buffer = m_impl.getInputMatrix();

    buffer->copy(buf.m_buffer);

    return m_impl.encodeBuffer();
}
}  // namespace Tracker
}  // namespace OpenViBE
