#pragma once

#include "../ovkTKernelObject.h"

#include <map>
#include <mutex>

namespace OpenViBE {
namespace Kernel {
class CAlgorithmProxy;

class CAlgorithmManager final : public TKernelObject<IAlgorithmManager>
{
public:

    explicit CAlgorithmManager(const IKernelContext& ctx) : TKernelObject<IAlgorithmManager>(ctx) {}
    ~CAlgorithmManager() override;
    CIdentifier createAlgorithm(const CIdentifier& algorithmClassID) override;
    CIdentifier createAlgorithm(const Plugins::IAlgorithmDesc& algorithmDesc) override;
    bool releaseAlgorithm(const CIdentifier& rAlgorithmIdentifier) override;
    bool releaseAlgorithm(IAlgorithmProxy& rAlgorithm) override;
    IAlgorithmProxy& getAlgorithm(const CIdentifier& rAlgorithmIdentifier) override;
    CIdentifier getNextAlgorithmIdentifier(const CIdentifier& previousID) const override;

    _IsDerivedFromClass_Final_(TKernelObject<IAlgorithmManager>, OVK_ClassId_Kernel_Algorithm_AlgorithmManager)

protected:

    CIdentifier getUnusedIdentifier() const;
    std::map<CIdentifier, CAlgorithmProxy*> m_algorithms;
    mutable std::mutex m_oMutex;
};
}  // namespace Kernel
}  // namespace OpenViBE
