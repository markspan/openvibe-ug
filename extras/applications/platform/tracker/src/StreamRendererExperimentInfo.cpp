//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <system/ovCTime.h>

#include "StreamRendererExperimentInfo.h"

namespace OpenViBE {
namespace Tracker {

CString StreamRendererExperimentInfo::renderAsText(const size_t indent) const
{
    const TypeExperimentInfo::Header& hdr = m_stream->getHeader();

    std::stringstream ss;

    ss << std::string(indent, ' ') << "Experiment id: " << hdr.m_ExperimentID << std::endl;
    ss << std::string(indent, ' ') << "Experiment date: " << hdr.m_ExperimentDate << std::endl;

    ss << std::string(indent, ' ') << "Subject id: " << hdr.m_SubjectID << std::endl;
    ss << std::string(indent, ' ') << "Subject name: " << hdr.m_SubjectName << std::endl;
    ss << std::string(indent, ' ') << "Subject age: " << hdr.m_SubjectAge << std::endl;
    ss << std::string(indent, ' ') << "Subject gender: " << hdr.m_SubjectGender << std::endl;

    ss << std::string(indent, ' ') << "Laboratory id: " << hdr.m_LaboratoryID << std::endl;
    ss << std::string(indent, ' ') << "Laboratory name: " << hdr.m_LaboratoryName << std::endl;
    ss << std::string(indent, ' ') << "Technician id: " << hdr.m_TechnicianID << std::endl;
    ss << std::string(indent, ' ') << "Technician name: " << hdr.m_TechnicianName << std::endl;

    //  ss << string(indent, ' ') << "Channels: " << m_Header.m_header.getDimensionSize(0) << std::endl;
    //  ss << string(indent, ' ') << "Samples per chunk: " << m_Header.m_header.getDimensionSize(1) << std::endl;
    return ss.str().c_str();
}

bool StreamRendererExperimentInfo::showChunkList() { return StreamRendererLabel::showChunkList("Experiment information stream details"); }

}  // namespace Tracker
}  // namespace OpenViBE
