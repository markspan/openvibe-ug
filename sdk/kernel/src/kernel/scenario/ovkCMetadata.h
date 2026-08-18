#pragma once

#include <openvibe/kernel/scenario/ovIMetadata.h>
#include "../ovkTKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class CScenario;

class CMetadata final : public TKernelObject<IMetadata>
{
public:

    CMetadata(const IKernelContext& ctx, CScenario& ownerScenario) : TKernelObject<IMetadata>(ctx), m_ownerScenario(ownerScenario), m_data("") {}
    ~CMetadata() override {}

    CIdentifier getIdentifier() const override { return m_id; }
    CIdentifier getType() const override { return m_type; }
    CString getData() const override { return m_data; }
    bool setIdentifier(const CIdentifier& identifier) override;
    bool setType(const CIdentifier& typeID) override;
    bool setData(const CString& data) override;

    bool initializeFromExistingMetadata(const IMetadata& existingMetadata) override;

    bool acceptVisitor(IObjectVisitor& objectVisitor) override;

    _IsDerivedFromClass_Final_(TKernelObject<IMetadata>, OVK_ClassId_Kernel_Scenario_Metadata)

private:

    CScenario& m_ownerScenario;

    CIdentifier m_id   = CIdentifier::undefined();
    CIdentifier m_type = CIdentifier::undefined();
    CString m_data;
};
}  // namespace Kernel
}  // namespace OpenViBE
