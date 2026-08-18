#pragma once

#include <toolkit/ovtk_all.h>

#define OVP_ClassId_BoxAlgorithm_Null                                              OpenViBE::CIdentifier(0x601118A8, 0x14BF700F)
#define OVP_ClassId_BoxAlgorithm_NullDesc                                          OpenViBE::CIdentifier(0x6BD21A21, 0x0A5E685A)

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmNull final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, OVP_ClassId_BoxAlgorithm_Null)
};

class CBoxAlgorithmNullDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Null"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Consumes input and produces nothing. It can be used to show scenario design intent."; }
    CString getDetailedDescription() const override { return "Directing to Null instead of leaving a box output unconnected may add a tiny overhead."; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Null; }
    IPluginObject* create() override { return new CBoxAlgorithmNull(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stream", OV_TypeId_EBMLStream);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_NullDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
