#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeChannelUnits.h"

#include "Decoder.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {
template <>
bool DecoderAdapter<TypeChannelUnits, Toolkit::TChannelUnitsDecoder<BoxAlgorithmProxy>>::getHeaderImpl(TypeChannelUnits::Header& h)
{
    h.m_Dynamic = m_impl.getOutputDynamic();

    CMatrix* decoded = m_impl.getOutputMatrix();
    h.m_Header.copy(*decoded);

    return true;
}

template <>
bool DecoderAdapter<TypeChannelUnits, Toolkit::TChannelUnitsDecoder<BoxAlgorithmProxy>>::getBufferImpl(TypeChannelUnits::Buffer& b)
{
    CMatrix* decoded = m_impl.getOutputMatrix();
    b.m_buffer.copy(*decoded);

    return true;
}

template <>
bool EncoderAdapter<TypeChannelUnits, Toolkit::TChannelUnitsEncoder<BoxAlgorithmProxy>>::encodeHeaderImpl(const TypeChannelUnits::Header& hdr)
{
    m_impl.getInputDynamic() = hdr.m_Dynamic;

    CMatrix* header = m_impl.getInputMatrix();
    header->copy(hdr.m_Header);

    return m_impl.encodeHeader();
}

template <>
bool EncoderAdapter<TypeChannelUnits, Toolkit::TChannelUnitsEncoder<BoxAlgorithmProxy>>::encodeBufferImpl(const TypeChannelUnits::Buffer& buf)
{
    CMatrix* buffer = m_impl.getInputMatrix();
    buffer->copy(buf.m_buffer);

    return m_impl.encodeBuffer();
}
}  // namespace Tracker
}  // namespace OpenViBE
