#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeStimulation.h"

#include "Decoder.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {

template <>
bool DecoderAdapter<TypeStimulation, Toolkit::TStimulationDecoder<BoxAlgorithmProxy>>::getHeaderImpl(TypeStimulation::Header& /*target*/) { return true; }

template <>
bool DecoderAdapter<TypeStimulation, Toolkit::TStimulationDecoder<BoxAlgorithmProxy>>::getBufferImpl(TypeStimulation::Buffer& b)
{
    CStimulationSet* decoded = m_impl.getOutputStimulationSet();

    b.m_buffer.clear();
    for (size_t i = 0; i < decoded->size(); ++i) { b.m_buffer.push_back(decoded->getId(i), decoded->getDate(i), decoded->getDuration(i)); }
    return true;
}

template <>
bool EncoderAdapter<TypeStimulation, Toolkit::TStimulationEncoder<BoxAlgorithmProxy>>::encodeHeaderImpl(const TypeStimulation::Header& /*hdr*/)
{
    return m_impl.encodeHeader();
}

template <>
bool EncoderAdapter<TypeStimulation, Toolkit::TStimulationEncoder<BoxAlgorithmProxy>>::encodeBufferImpl(const TypeStimulation::Buffer& buf)
{
    CStimulationSet* inputSet = m_impl.getInputStimulationSet();

    inputSet->clear();
    for (size_t i = 0; i < buf.m_buffer.size(); ++i) {
        inputSet->push_back(buf.m_buffer.getId(i), buf.m_buffer.getDate(i) + m_offset.time(), buf.m_buffer.getDuration(i));
    }

    return m_impl.encodeBuffer();
}

}  // namespace Tracker
}  // namespace OpenViBE
