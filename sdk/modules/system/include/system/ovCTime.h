#pragma once

#include "defines.h"
#include <cstdlib>  // For Unix Compatibility

namespace System {
/**
 * \class Time
 * \brief Static functions to handle time within the framework
 *
 */
class System_API Time
{
public:

    /**
     * \brief Make the calling thread sleep 
     * \param milliSeconds : sleep duration in ms
     * \return Always true
     */
    static bool sleep(const size_t milliSeconds);

    /**
     * \brief Make the calling thread sleep 
     * \param seconds : sleep duration in fixed point 32:32 seconds
     * \return Always true
     */
    static bool zsleep(const uint64_t seconds);

    /**
     * \brief Retrieve time in ms (turn the 32:32 fixed point seconds to milliseconds).
     * \return Elapsed time in ms since the first call to this function or zgetTime functions
     */
    static uint32_t getTime() { return uint32_t((zgetTime() * 1000) >> 32); }

    /**
     * \brief Retrieve time in fixed point 32:32 seconds 
     * \return Elapsed time since the first call to the zgetTime functions or getTime.
     */
    static uint64_t zgetTime() { return zgetTimeRaw(true); }

    /**
     * \brief Retrieve time in fixed point 32:32 seconds 
     * \param sinceFirstCall : If sinceFirstCall is true, returns the time since the first call to the zgetTime function or getTime. 
     *        Otherwise, returns time since epoch of the clock.
     * \return Elapsed time 
     */
    static uint64_t zgetTimeRaw(bool sinceFirstCall = true);

    /**
     * \brief Check if the internal clock used by the framework is steady
     * \return True if the clock is steady, false otherwise
     * \note This is a theoretical check that queries the internal
     *       clock implementation for available services
     */
    static bool isClockSteady();

    /**
     * \brief Check if the internal clock used by the framework has
     *        a resolution higher than the required one
     * \param milliSeconds : Expected clock resolution (period between ticks) in ms (must be non-zero value)
     * \return True if the clock meets the requirements, false otherwise
     * \note This is a theoretical check that queries the internal
     *       clock implementation for available services
     */
    static bool checkResolution(const size_t milliSeconds);

private:

    Time() = delete;
};
}  // namespace System
