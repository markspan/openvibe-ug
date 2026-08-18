#include "ovpCExperimentInfoEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CExperimentInfoEncoder::initialize()
{
    CEBMLBaseEncoder::initialize();

    ip_ExperimentID.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentID));
    ip_experimentDate.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentDate));
    ip_subjectID.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectID));
    ip_subjectName.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectName));
    ip_subjectAge.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectAge));
    ip_subjectGender.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectGender));
    ip_LaboratoryID.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryID));
    ip_pLaboratoryName.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryName));
    ip_TechnicianID.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianID));
    ip_pTechnicianName.initialize(getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianName));

    return true;
}

bool CExperimentInfoEncoder::uninitialize()
{
    ip_pTechnicianName.uninitialize();
    ip_TechnicianID.uninitialize();
    ip_pLaboratoryName.uninitialize();
    ip_LaboratoryID.uninitialize();
    ip_subjectGender.uninitialize();
    ip_subjectAge.uninitialize();
    ip_subjectName.uninitialize();
    ip_subjectID.uninitialize();
    ip_experimentDate.uninitialize();
    ip_ExperimentID.uninitialize();

    CEBMLBaseEncoder::uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CExperimentInfoEncoder::processHeader()
{
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo);
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Experiment);
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Experiment_ID);
    m_writerHelper->setUInt(ip_ExperimentID);
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Experiment_Date);
    m_writerHelper->setStr(ip_experimentDate->toASCIIString());
    m_writerHelper->closeChild();
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Subject);
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Subject_ID);
    m_writerHelper->setUInt(ip_subjectID);
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Subject_Name);
    m_writerHelper->setStr(ip_subjectName->toASCIIString());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Subject_Age);
    m_writerHelper->setUInt(ip_subjectAge);
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Subject_Gender);
    m_writerHelper->setUInt(ip_subjectGender);
    m_writerHelper->closeChild();
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Context);
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryID);
    m_writerHelper->setUInt(ip_LaboratoryID);
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryName);
    m_writerHelper->setStr(ip_pLaboratoryName->toASCIIString());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianID);
    m_writerHelper->setUInt(ip_TechnicianID);
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianName);
    m_writerHelper->setStr(ip_pTechnicianName->toASCIIString());
    m_writerHelper->closeChild();
    m_writerHelper->closeChild();
    m_writerHelper->closeChild();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
