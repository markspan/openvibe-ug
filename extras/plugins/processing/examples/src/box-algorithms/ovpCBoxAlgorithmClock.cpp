#include "ovpCBoxAlgorithmClock.h"

namespace OpenViBE {
namespace Plugins {
namespace Examples {

uint64_t CBoxAlgorithmClock::getClockFrequency()
{
    getLogManager() << m_logLevel << "Clock frequency requested at time " << CTime(getPlayerContext().getCurrentTime()) << "\n";
    return m_clockFrequency << 32;
}

bool CBoxAlgorithmClock::initialize()
{
    m_clockFrequency = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    const CString value = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_logLevel = Kernel::ELogLevel(getBoxAlgorithmContext()->getPlayerContext()->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_LogLevel, value));

    getLogManager() << m_logLevel << "Clock frequency tuned to " << m_clockFrequency << "\n";

    return true;
}

bool CBoxAlgorithmClock::processClock(Kernel::CMessageClock& /*msg*/)
{
    getLogManager() << m_logLevel << "Received clock message at time " << CTime(getPlayerContext().getCurrentTime()) << "\n";
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmClock::process()
{
    getLogManager() << m_logLevel << "Process function activated at " << getPlayerContext().getCurrentTime() << "\n";
    return true;
}
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
