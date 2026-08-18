/*
 * Prints user-specified greeting to the log with given frequency 
 */
#include "ovpCHelloWorld.h"

#include <openvibe/CTime.hpp>

namespace OpenViBE {
namespace Plugins {
namespace Examples {


uint64_t CHelloWorld::getClockFrequency() { return CTime(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0))).time(); }

bool CHelloWorld::processClock(Kernel::CMessageClock& /*msg*/)
{
    const CString myGreeting = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    getLogManager() << Kernel::LogLevel_Info << ": " << myGreeting << "\n";
    return true;
}
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
