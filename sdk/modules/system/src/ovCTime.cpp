#include "system/ovCTime.h"

#include <cmath>
#include <cassert>

// \warning On Windows, avoid "using namespace System;" here as it may cause confusion with stuff coming from windows/boost
// \note Support of C++11 steady clock:
//       - From GCC 4.8.1
//       - From Visual Studio 2015 (therefore a strategy is needed to handle Visual Studio 2013 version)

// time handling strategy selection
// \note With officialy supported compilers and required boost version
//       it should never fallback in a OV_USE_SYSTEM case
#if (defined(_MSC_VER) && _MSC_VER <= 1800)

#include <boost/chrono/config.hpp>

#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY

#include <boost/chrono.hpp>
#include <boost/thread.hpp>
namespace Timelib = boost;

#else
#error "Please use OpenViBE recommended version of Boost"
#endif // BOOST_CHRONO_HAS_CLOCK_STEADY

#else // defined(_MSC_VER) && _MSC_VER <= 1800

#include <chrono>
#include <thread>
namespace Timelib = std;

#endif // defined(_MSC_VER) && _MSC_VER <= 1800

using internal_clock = Timelib::chrono::steady_clock;
// using internal_clock = chrono::high_resolution_clock;

namespace System {

bool Time::sleep(const size_t milliSeconds)
{
    Timelib::this_thread::sleep_for(Timelib::chrono::milliseconds(milliSeconds));
    return true;
}

bool Time::zsleep(const uint64_t seconds)
{
    const uint32_t s = uint32_t(seconds >> 32);
    // zero the seconds with 0xFFFFFFFF, multiply to get the rest as fixed point microsec, then grab them (now in the 32 msbs)
    const uint64_t ms = ((seconds & 0xFFFFFFFFLL) * 1000000LL) >> 32;

    const Timelib::chrono::microseconds duration = Timelib::chrono::seconds(s) + Timelib::chrono::microseconds(ms);
    Timelib::this_thread::sleep_for(duration);

    return true;
}

uint64_t Time::zgetTimeRaw(const bool sinceFirstCall)
{
    static bool initialized = false;
    static internal_clock::time_point start;

    if (!initialized)
    {
        start       = internal_clock::now();
        initialized = true;
    }

    const internal_clock::time_point now          = internal_clock::now();
    const internal_clock::duration elapsed        = (sinceFirstCall ? now - start : now.time_since_epoch());
    const Timelib::chrono::microseconds elapsedMs = Timelib::chrono::duration_cast<Timelib::chrono::microseconds>(elapsed);

    const uint64_t microsPerSecond = 1000ULL * 1000ULL;
    const uint64_t seconds         = uint64_t(elapsedMs.count() / microsPerSecond);
    const uint64_t fraction        = uint64_t(elapsedMs.count() % microsPerSecond);

    // below in fraction part, scale [0,microsPerSecond-1] to 32bit integer range
    const uint64_t res = (seconds << 32) + fraction * (0xFFFFFFFFLL / (microsPerSecond - 1));
    return res;
}

bool Time::isClockSteady() { return internal_clock::is_steady; }

bool Time::checkResolution(const size_t milliSeconds)
{
    assert(milliSeconds != 0);
    const auto resolution = double(internal_clock::period::num) / internal_clock::period::den;
    return (size_t(std::ceil(resolution * 1000)) <= milliSeconds);
}

}  // namespace System
