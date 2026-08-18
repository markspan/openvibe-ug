#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeChannelLocalization.h"

#include "Decoder.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {
template <>
bool DecoderAdapter<TypeChannelLocalization, Toolkit::TChannelLocalisationDecoder<BoxAlgorithmProxy>>::getHeaderImpl(TypeChannelLocalization::Header& h)
{
    h.m_Dynamic = m_impl.getOutputDynamic();

    CMatrix* decoded = m_impl.getOutputMatrix();
    h.m_Header.copy(*decoded);

    return true;
}

template <>
bool DecoderAdapter<TypeChannelLocalization, Toolkit::TChannelLocalisationDecoder<BoxAlgorithmProxy>>::getBufferImpl(TypeChannelLocalization::Buffer& b)
{
    CMatrix* decoded = m_impl.getOutputMatrix();
    b.m_buffer.copy(*decoded);

    return true;
}

template <>
bool EncoderAdapter<TypeChannelLocalization, Toolkit::TChannelLocalisationEncoder<BoxAlgorithmProxy>>::encodeHeaderImpl(
    const TypeChannelLocalization::Header& hdr)
{
    m_impl.getInputDynamic() = hdr.m_Dynamic;

    CMatrix* header = m_impl.getInputMatrix();
    header->copy(hdr.m_Header);

    return m_impl.encodeHeader();
}

template <>
bool EncoderAdapter<TypeChannelLocalization, Toolkit::TChannelLocalisationEncoder<BoxAlgorithmProxy>>::encodeBufferImpl(
    const TypeChannelLocalization::Buffer& buf)
{
    CMatrix* buffer = m_impl.getInputMatrix();
    buffer->copy(buf.m_buffer);

    return m_impl.encodeBuffer();
}
}  // namespace Tracker
}  // namespace OpenViBE
