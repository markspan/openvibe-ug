#pragma once

#include "../../ovp_defines.h"
#include "ovpCEBMLBaseDecoder.h"
#include <stack>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CExperimentInfoDecoder final : public CEBMLBaseDecoder
{
public:
    CExperimentInfoDecoder();
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseDecoder, OVP_ClassId_Algorithm_ExperimentInfoDecoder)

    // ebml callbacks
    bool isMasterChild(const EBML::CIdentifier& identifier) override;
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override;

protected:
    Kernel::TParameterHandler<uint64_t> op_ExperimentID;
    Kernel::TParameterHandler<CString*> op_experimentDate;

    Kernel::TParameterHandler<uint64_t> op_subjectID;
    Kernel::TParameterHandler<CString*> op_subjectName;
    Kernel::TParameterHandler<uint64_t> op_subjectAge;
    Kernel::TParameterHandler<uint64_t> op_subjectGender;

    Kernel::TParameterHandler<uint64_t> op_LaboratoryID;
    Kernel::TParameterHandler<CString*> op_pLaboratoryName;
    Kernel::TParameterHandler<uint64_t> op_TechnicianID;
    Kernel::TParameterHandler<CString*> op_pTechnicianName;

private:
    std::stack<EBML::CIdentifier> m_nodes;
};

class CExperimentInfoDecoderDesc final : public CEBMLBaseDecoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Experiment information stream decoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Decoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ExperimentInfoDecoder; }
    IPluginObject* create() override { return new CExperimentInfoDecoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CEBMLBaseDecoderDesc::getAlgorithmPrototype(prototype);

        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_ExperimentID, "Experiment identifier",
                                     Kernel::ParameterType_UInteger);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_ExperimentDate, "Experiment date", Kernel::ParameterType_String);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectID, "Subject identifier", Kernel::ParameterType_UInteger);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectName, "Subject name", Kernel::ParameterType_String);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectAge, "Subject age", Kernel::ParameterType_UInteger);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectGender, "Subject gender", Kernel::ParameterType_UInteger);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_LaboratoryID, "Laboratory identifier",
                                     Kernel::ParameterType_UInteger);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_LaboratoryName, "Laboratory name", Kernel::ParameterType_String);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_TechnicianID, "Technician identifier",
                                     Kernel::ParameterType_UInteger);
        prototype.addOutputParameter(OVP_Algorithm_ExperimentInfoDecoder_OutputParameterId_TechnicianName, "Technician name", Kernel::ParameterType_String);

        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseDecoderDesc, OVP_ClassId_Algorithm_ExperimentInfoDecoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
