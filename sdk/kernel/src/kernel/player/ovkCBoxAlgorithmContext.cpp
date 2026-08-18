#include "ovkCBoxAlgorithmContext.h"
#include "ovkCPlayerContext.h"
#include "ovkCSimulatedBox.h"

namespace OpenViBE {
namespace Kernel {

CBoxAlgorithmCtx::CBoxAlgorithmCtx(const IKernelContext& ctx, CSimulatedBox* simulatedBox, const IBox* box)
    : TKernelObject<IBoxAlgorithmContext>(ctx), m_pStaticBoxContext(box), m_pDynamicBoxContext(simulatedBox), m_oPlayerContext(ctx, simulatedBox) {}


bool CBoxAlgorithmCtx::markAlgorithmAsReadyToProcess()
{
    m_bReadyToProcess = true;
    return true;
}

}  // namespace Kernel
}  // namespace OpenViBE
