#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CAlgorithmOVMatrixFileWriter final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_OVMatrixFileWriter)

protected:
    Kernel::TParameterHandler<CString*> ip_sFilename;
    Kernel::TParameterHandler<CMatrix*> ip_pMatrix;
};

class CAlgorithmOVMatrixFileWriterDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "OpenViBE Matrix file writer"; }
    CString getAuthorName() const override { return "Vincent Delannoy"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_OVMatrixFileWriter; }
    IPluginObject* create() override { return new CAlgorithmOVMatrixFileWriter; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Filename, "Filename", Kernel::ParameterType_String);
        prototype.addInputParameter(OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Matrix, "Matrix", Kernel::ParameterType_Matrix);
        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_OVMatrixFileWriterDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
