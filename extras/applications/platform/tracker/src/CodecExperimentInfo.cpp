#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeExperimentInfo.h"

#include "Decoder.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {
template <>
bool DecoderAdapter<TypeExperimentInfo, Toolkit::TExperimentInfoDecoder<BoxAlgorithmProxy>>::getHeaderImpl(TypeExperimentInfo::Header& h)
{
    h.m_ExperimentID   = m_impl.getOutputExperimentID();
    h.m_ExperimentDate = (*m_impl.getOutputExperimentDate()).toASCIIString();

    h.m_SubjectID     = m_impl.getOutputSubjectID();
    h.m_SubjectName   = (*m_impl.getOutputSubjectName()).toASCIIString();
    h.m_SubjectAge    = m_impl.getOutputSubjectAge();
    h.m_SubjectGender = m_impl.getOutputSubjectGender();

    h.m_LaboratoryID   = m_impl.getOutputLaboratoryID();
    h.m_LaboratoryName = (*m_impl.getOutputLaboratoryName()).toASCIIString();
    h.m_TechnicianID   = m_impl.getOutputTechnicianID();
    h.m_TechnicianName = (*m_impl.getOutputTechnicianName()).toASCIIString();

    return true;
}

template <>
bool DecoderAdapter<TypeExperimentInfo, Toolkit::TExperimentInfoDecoder<BoxAlgorithmProxy>>::getBufferImpl(TypeExperimentInfo::Buffer& /*b*/)
{
    // Should be no buffer in the experiment stream
    return true;
}

template <>
bool EncoderAdapter<TypeExperimentInfo, Toolkit::TExperimentInfoEncoder<BoxAlgorithmProxy>>::encodeHeaderImpl(const TypeExperimentInfo::Header& hdr)
{
    // @fixme the new() calls may imply memory leaks, a bit odd the codec takes pointers
    m_impl.getInputExperimentID()   = hdr.m_ExperimentID;
    m_impl.getInputExperimentDate() = new CString(hdr.m_ExperimentDate.c_str());

    m_impl.getInputSubjectID()     = hdr.m_SubjectID;
    m_impl.getInputSubjectName()   = new CString(hdr.m_SubjectName.c_str());
    m_impl.getInputSubjectAge()    = hdr.m_SubjectAge;
    m_impl.getInputSubjectGender() = hdr.m_SubjectGender;

    m_impl.getInputLaboratoryID()   = hdr.m_LaboratoryID;
    m_impl.getInputLaboratoryName() = new CString(hdr.m_LaboratoryName.c_str());
    m_impl.getInputTechnicianID()   = hdr.m_TechnicianID;
    m_impl.getInputTechnicianName() = new CString(hdr.m_TechnicianName.c_str());

    return m_impl.encodeHeader();
}


template <>
bool EncoderAdapter<TypeExperimentInfo, Toolkit::TExperimentInfoEncoder<BoxAlgorithmProxy>>::encodeBufferImpl(const TypeExperimentInfo::Buffer& /*buf*/)
{
    // Should be no buffer in the experiment stream
    return m_impl.encodeBuffer();
}
}  // namespace Tracker
}  // namespace OpenViBE
