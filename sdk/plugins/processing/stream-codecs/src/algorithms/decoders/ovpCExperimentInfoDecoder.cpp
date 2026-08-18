#include "ovpCExperimentInfoDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

CExperimentInfoDecoder::CExperimentInfoDecoder() {}

// ________________________________________________________________________________________________________________
//

bool CExperimentInfoDecoder::initialize()
{
    CEBMLBaseDecoder::initialize();

    op_ExperimentID.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_ExperimentID));
    op_experimentDate.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_ExperimentDate));
    op_subjectID.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectID));
    op_subjectName.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectName));
    op_subjectAge.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectAge));
    op_subjectGender.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectGender));
    op_LaboratoryID.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_LaboratoryID));
    op_pLaboratoryName.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_LaboratoryName));
    op_TechnicianID.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_TechnicianID));
    op_pTechnicianName.initialize(getOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_TechnicianName));

    return true;
}

bool CExperimentInfoDecoder::uninitialize()
{
    op_pTechnicianName.uninitialize();
    op_TechnicianID.uninitialize();
    op_pLaboratoryName.uninitialize();
    op_LaboratoryID.uninitialize();
    op_subjectGender.uninitialize();
    op_subjectAge.uninitialize();
    op_subjectName.uninitialize();
    op_subjectID.uninitialize();
    op_experimentDate.uninitialize();
    op_ExperimentID.uninitialize();

    CEBMLBaseDecoder::uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CExperimentInfoDecoder::isMasterChild(const EBML::CIdentifier& identifier)
{
    if (identifier == OVTK_NodeId_Header_ExperimentInfo) { return true; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Experiment) { return true; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Experiment_ID) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Experiment_Date) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Subject) { return true; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Subject_ID) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Subject_Name) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Subject_Age) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Subject_Gender) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Context) { return true; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryID) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryName) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianID) { return false; }
    if (identifier == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianName) { return false; }
    return CEBMLBaseDecoder::isMasterChild(identifier);
}

void CExperimentInfoDecoder::openChild(const EBML::CIdentifier& identifier)
{
    m_nodes.push(identifier);

    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ExperimentInfo)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment_ID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment_Date)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_ID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Name)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Age)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Gender)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryName)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianName)) { }
    else { CEBMLBaseDecoder::openChild(identifier); }
}

void CExperimentInfoDecoder::processChildData(const void* buffer, const size_t size)
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ExperimentInfo)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment_ID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment_Date)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_ID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Name)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Age)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Gender)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryName)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianName))
    {
        if (top == OVTK_NodeId_Header_ExperimentInfo_Experiment_ID) { op_ExperimentID = m_readerHelper->getUInt(buffer, size); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Experiment_Date) { op_experimentDate->set(m_readerHelper->getStr(buffer, size)); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Subject_ID) { op_subjectID = m_readerHelper->getUInt(buffer, size); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Name) { op_subjectName->set(m_readerHelper->getStr(buffer, size)); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Age) { op_subjectAge = m_readerHelper->getUInt(buffer, size); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Gender) { op_subjectGender = m_readerHelper->getUInt(buffer, size); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryID) { op_LaboratoryID = m_readerHelper->getUInt(buffer, size); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryName) { op_pLaboratoryName->set(m_readerHelper->getStr(buffer, size)); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianID) { op_TechnicianID = m_readerHelper->getUInt(buffer, size); }
        if (top == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianName) { op_pTechnicianName->set(m_readerHelper->getStr(buffer, size)); }
    }
    else { CEBMLBaseDecoder::processChildData(buffer, size); }
}

void CExperimentInfoDecoder::closeChild()
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ExperimentInfo)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment_ID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Experiment_Date)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_ID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Name)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Age)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Subject_Gender)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_LaboratoryName)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianID)
        || (top == OVTK_NodeId_Header_ExperimentInfo_Context_TechnicianName)) { }
    else { CEBMLBaseDecoder::closeChild(); }

    m_nodes.pop();
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
