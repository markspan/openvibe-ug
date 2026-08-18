#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeSignal.h"

#include "Decoder.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {
template <>
bool DecoderAdapter<TypeSignal, Toolkit::TSignalDecoder<BoxAlgorithmProxy>>::getHeaderImpl(TypeSignal::Header& h)
{
    h.m_Sampling = m_impl.getOutputSamplingRate();

    CMatrix* decoded = m_impl.getOutputMatrix();
    h.m_Header.copy(*decoded);

    return true;
}

template <>
bool DecoderAdapter<TypeSignal, Toolkit::TSignalDecoder<BoxAlgorithmProxy>>::getBufferImpl(TypeSignal::Buffer& b)
{
    const CMatrix* decoded = m_impl.getOutputMatrix();
    b.m_buffer.copy(*decoded);

    return true;
}

template <>
bool EncoderAdapter<TypeSignal, Toolkit::TSignalEncoder<BoxAlgorithmProxy>>::encodeHeaderImpl(const TypeSignal::Header& hdr)
{
    m_impl.getInputSamplingRate() = hdr.m_Sampling;

    CMatrix* header = m_impl.getInputMatrix();
    header->copy(hdr.m_Header);

    return m_impl.encodeHeader();
}

template <>
bool EncoderAdapter<TypeSignal, Toolkit::TSignalEncoder<BoxAlgorithmProxy>>::encodeBufferImpl(const TypeSignal::Buffer& buf)
{
    CMatrix* buffer = m_impl.getInputMatrix();
    buffer->copy(buf.m_buffer);

    return m_impl.encodeBuffer();
}
}  // namespace Tracker
}  // namespace OpenViBE
