#include "ovpCBoxAlgorithmNull.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmNull::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmNull::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();
    const size_t nInput        = getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount();
    for (size_t i = 0; i < nInput; ++i) { for (size_t j = 0; j < boxContext->getInputChunkCount(i); ++j) { boxContext->markInputAsDeprecated(i, j); } }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
