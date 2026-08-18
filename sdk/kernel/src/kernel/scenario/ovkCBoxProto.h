#pragma once

#include "../ovkTKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class CBoxProto : public TKernelObject<IBoxProto>
{
public:

    CBoxProto(const IKernelContext& ctx, IBox& box) : TKernelObject<IBoxProto>(ctx), m_box(box) {}
    ~CBoxProto() override = default;
    bool addInput(const CString& name, const CIdentifier& typeID, const CIdentifier& id = CIdentifier::undefined(), const bool notify = true) override;
    bool addOutput(const CString& name, const CIdentifier& typeID, const CIdentifier& id = CIdentifier::undefined(), const bool notify = true) override;

    //virtual bool addSetting(const CString& name, const CIdentifier& typeID, const CString& value); 
    bool addSetting(const CString& name, const CIdentifier& typeID, const CString& value,
                    const bool modifiable = false, const CIdentifier& id = CIdentifier::undefined(), const bool notify = true) override;
    bool addFlag(const EBoxFlag boxFlag) override;
    bool addFlag(const CIdentifier& flagID) override;
    bool addInputSupport(const CIdentifier& typeID) override { return m_box.addInputSupport(typeID); }
    bool addOutputSupport(const CIdentifier& typeID) override { return m_box.addOutputSupport(typeID); }

    _IsDerivedFromClass_Final_(TKernelObject<IBoxProto>, OVK_ClassId_Kernel_Scenario_BoxProto)

protected:

    IBox& m_box;

private:

    CBoxProto() = delete;
};
}  // namespace Kernel
}  // namespace OpenViBE
