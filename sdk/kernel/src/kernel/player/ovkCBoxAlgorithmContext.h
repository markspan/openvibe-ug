#pragma once

#include "../ovkTKernelObject.h"
#include "ovkCPlayerContext.h"

namespace OpenViBE {
namespace Kernel {
class CSimulatedBox;

class CBoxAlgorithmCtx final : public TKernelObject<IBoxAlgorithmContext>
{
public:

    CBoxAlgorithmCtx(const IKernelContext& ctx, CSimulatedBox* simulatedBox, const IBox* box);
    ~CBoxAlgorithmCtx() override {}
    const IBox* getStaticBoxContext() override { return m_pStaticBoxContext; }
    IBoxIO* getDynamicBoxContext() override { return m_pDynamicBoxContext; }
    IPlayerContext* getPlayerContext() override { return &m_oPlayerContext; }
    bool markAlgorithmAsReadyToProcess() override;

    _IsDerivedFromClass_Final_(TKernelObject<IBoxAlgorithmContext>, OVK_ClassId_Kernel_Player_BoxAlgorithmContext)

    bool isAlgorithmReadyToProcess() const { return m_bReadyToProcess; }

protected:

    const IBox* m_pStaticBoxContext = nullptr;
    IBoxIO* m_pDynamicBoxContext    = nullptr;
    // here we prefer value type over reference/pointer
    // in order to improve performance at runtime (no heap allocation)
    CPlayerContext m_oPlayerContext;
    bool m_bReadyToProcess = false;
};
}  // namespace Kernel
}  // namespace OpenViBE
