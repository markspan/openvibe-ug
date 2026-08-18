#include "ovpCLog.h"

namespace OpenViBE {
namespace Plugins {
namespace Examples {

bool CLog::initialize()
{
    getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Info << "initialize\n";
    return true;
}

bool CLog::uninitialize()
{
    getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Info << "uninitialize\n";
    return true;
}

bool CLog::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Info << "processClock\n";
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CLog::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Info << "processInput\n";
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CLog::process()
{
    getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Info << "process\n";

    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();
    const size_t nInput        = getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount();

    for (size_t i = 0; i < nInput; ++i) { for (size_t j = 0; j < boxContext->getInputChunkCount(i); ++j) { boxContext->markInputAsDeprecated(i, j); } }

    return true;
}
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
