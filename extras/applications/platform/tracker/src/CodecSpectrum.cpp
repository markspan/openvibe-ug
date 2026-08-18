#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeSpectrum.h"

#include "Decoder.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {
template <>
bool DecoderAdapter<TypeSpectrum, Toolkit::TSpectrumDecoder<BoxAlgorithmProxy>>::getHeaderImpl(TypeSpectrum::Header& h)
{
    CMatrix* decoded = m_impl.getOutputMatrix();
    h.m_Header.copy(*decoded);

    h.m_Sampling = m_impl.getOutputSamplingRate();

    CMatrix* abscissas = m_impl.getOutputFrequencyAbscissa();
    h.m_Abscissas.copy(*abscissas);

    return true;
}

template <>
bool DecoderAdapter<TypeSpectrum, Toolkit::TSpectrumDecoder<BoxAlgorithmProxy>>::getBufferImpl(TypeSpectrum::Buffer& b)
{
    const CMatrix* decoded = m_impl.getOutputMatrix();

    b.m_buffer.copy(*decoded);

    return true;
}

template <>
bool EncoderAdapter<TypeSpectrum, Toolkit::TSpectrumEncoder<BoxAlgorithmProxy>>::encodeHeaderImpl(const TypeSpectrum::Header& hdr)
{
    m_impl.getInputSamplingRate() = hdr.m_Sampling;

    CMatrix* header = m_impl.getInputMatrix();
    header->copy(hdr.m_Header);

    CMatrix* abscissas = m_impl.getInputFrequencyAbscissa();
    abscissas->copy(hdr.m_Abscissas);

    return m_impl.encodeHeader();
}

template <>
bool EncoderAdapter<TypeSpectrum, Toolkit::TSpectrumEncoder<BoxAlgorithmProxy>>::encodeBufferImpl(const TypeSpectrum::Buffer& buf)
{
    CMatrix* buffer = m_impl.getInputMatrix();
    buffer->copy(buf.m_buffer);

    return m_impl.encodeBuffer();
}
}  // namespace Tracker
}  // namespace OpenViBE
