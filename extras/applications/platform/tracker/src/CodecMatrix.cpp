#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeMatrix.h"

#include "Decoder.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {
template <>
bool DecoderAdapter<TypeMatrix, Toolkit::TStreamedMatrixDecoder<BoxAlgorithmProxy>>::getHeaderImpl(TypeMatrix::Header& h)
{
    CMatrix* decoded = m_impl.getOutputMatrix();
    h.m_Header.copy(*decoded);

    return true;
}


template <>
bool DecoderAdapter<TypeMatrix, Toolkit::TStreamedMatrixDecoder<BoxAlgorithmProxy>>::getBufferImpl(TypeMatrix::Buffer& b)
{
    const CMatrix* decoded = m_impl.getOutputMatrix();
    b.m_buffer.copy(*decoded);

    return true;
}

template <>
bool EncoderAdapter<TypeMatrix, Toolkit::TStreamedMatrixEncoder<BoxAlgorithmProxy>>::encodeHeaderImpl(const TypeMatrix::Header& hdr)
{
    CMatrix* header = m_impl.getInputMatrix();
    header->copy(hdr.m_Header);

    return m_impl.encodeHeader();
}

template <>
bool EncoderAdapter<TypeMatrix, Toolkit::TStreamedMatrixEncoder<BoxAlgorithmProxy>>::encodeBufferImpl(const TypeMatrix::Buffer& buf)
{
    CMatrix* buffer = m_impl.getInputMatrix();
    buffer->copy(buf.m_buffer);

    return m_impl.encodeBuffer();
}
}  // namespace Tracker
}  // namespace OpenViBE
