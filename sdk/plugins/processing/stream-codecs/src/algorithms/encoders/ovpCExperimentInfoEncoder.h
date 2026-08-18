#pragma once

#include "../../ovp_defines.h"
#include "ovpCEBMLBaseEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CExperimentInfoEncoder final : public CEBMLBaseEncoder
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processHeader() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseEncoder, OVP_ClassId_Algorithm_ExperimentInfoEncoder)

protected:
    Kernel::TParameterHandler<uint64_t> ip_ExperimentID;
    Kernel::TParameterHandler<CString*> ip_experimentDate;

    Kernel::TParameterHandler<uint64_t> ip_subjectID;
    Kernel::TParameterHandler<CString*> ip_subjectName;
    Kernel::TParameterHandler<uint64_t> ip_subjectAge;
    Kernel::TParameterHandler<uint64_t> ip_subjectGender;

    Kernel::TParameterHandler<uint64_t> ip_LaboratoryID;
    Kernel::TParameterHandler<CString*> ip_pLaboratoryName;
    Kernel::TParameterHandler<uint64_t> ip_TechnicianID;
    Kernel::TParameterHandler<CString*> ip_pTechnicianName;
};

class CExperimentInfoEncoderDesc final : public CEBMLBaseEncoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Experiment information stream encoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Encoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ExperimentInfoEncoder; }
    IPluginObject* create() override { return new CExperimentInfoEncoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CEBMLBaseEncoderDesc::getAlgorithmPrototype(prototype);

        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentID, "Experiment identifier", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentDate, "Experiment date", Kernel::ParameterType_String);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectID, "Subject identifier", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectName, "Subject name", Kernel::ParameterType_String);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectAge, "Subject age", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectGender, "Subject gender", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryID, "Laboratory identifier", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryName, "Laboratory name", Kernel::ParameterType_String);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianID, "Technician identifier", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianName, "Technician name", Kernel::ParameterType_String);

        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseEncoderDesc, OVP_ClassId_Algorithm_ExperimentInfoEncoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
