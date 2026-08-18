#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#define OVP_ClassId_BoxAlgorithm_Nothing     OpenViBE::CIdentifier(0x273960C0, 0x485C407C)
#define OVP_ClassId_BoxAlgorithm_NothingDesc OpenViBE::CIdentifier(0x7A6167D9, 0x79070E22)

namespace OpenViBE {
namespace Plugins {
namespace Examples {
class CBoxAlgorithmNothing final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_Nothing)
};

class CBoxAlgorithmNothingDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Nothing"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "This box does nothing"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Examples/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-about"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Nothing; }
    IPluginObject* create() override { return new CBoxAlgorithmNothing; }

    bool getBoxPrototype(Kernel::IBoxProto& /*prototype*/) const override { return true; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_NothingDesc)
};
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
