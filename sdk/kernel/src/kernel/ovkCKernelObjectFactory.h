#pragma once

#include "ovkTKernelObject.h"

#include <vector>
#include <mutex>

namespace OpenViBE {
namespace Kernel {
class CKernelObjectFactory final : public TKernelObject<IKernelObjectFactory>
{
public:

    explicit CKernelObjectFactory(const IKernelContext& ctx) : TKernelObject<IKernelObjectFactory>(ctx) {}
    IObject* createObject(const CIdentifier& classID) override;
    bool releaseObject(IObject* pObject) override;

    _IsDerivedFromClass_Final_(TKernelObject<IKernelObjectFactory>, OVK_ClassId_Kernel_KernelObjectFactory)

protected:

    std::vector<IObject*> m_oCreatedObjects;

    std::mutex m_oMutex;
};
}  // namespace Kernel
}  // namespace OpenViBE
