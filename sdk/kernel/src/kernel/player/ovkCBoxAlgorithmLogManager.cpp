#include "ovkCPlayerContext.h"
#include "ovkCSimulatedBox.h"

namespace OpenViBE {
namespace Kernel {

void CBoxAlgorithmLogManager::log(const ELogLevel logLevel)
{
    CIdentifier boxId;
    m_simulatedBox.getBoxIdentifier(boxId);

    m_logManager << logLevel << "At time " << CTime(m_playerCtx.getCurrentTime()) << " <" << LogColor_PushStateBit << LogColor_ForegroundBlue
            << "Box algorithm" << LogColor_PopStateBit << "::" << boxId << " aka " << m_simulatedBox.getName() << "> ";
}

}  // namespace Kernel
}  // namespace OpenViBE
