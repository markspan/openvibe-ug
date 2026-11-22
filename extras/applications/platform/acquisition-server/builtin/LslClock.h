/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, not in upstream OpenViBE. The server's clock against LSL's, for the LSL Output writer (UG.md 2.21) and the LSL
 * driver (UG.md 2.64), so that both convert the same way.
 */
#pragma once

#include <lsl_cpp.h>

#include <chrono>
#include <cstdint>
#include <limits>

namespace LslClock {

/// The server's clock, which dates samples and stimulations (AbstractDriver::timestamp()), in seconds.
inline double serverSeconds()
{
    return 1e-9 * double(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

/**
 * @brief What to add to the server's clock, in seconds, to get LSL's (lsl::local_clock()).
 *
 * Neither clock is adjusted while they run, so this holds for a whole acquisition. Of a few readings, the one with the shortest
 * time between its reads, in case the thread is interrupted during one.
 */
inline double measureOffset()
{
    double offset   = 0.0;
    double shortest = std::numeric_limits<double>::max();
    for (int i = 0; i < 10; ++i) {
        const double before = serverSeconds();
        const double lsl    = lsl::local_clock();
        const double after  = serverSeconds();
        if (after - before < shortest) {
            shortest = after - before;
            offset   = lsl - (before + after) / 2.0;
        }
    }
    return offset;
}

/// A time on LSL's clock, in seconds, as a stamp of the server's clock, in ns; 0 for a time before the server's clock began.
inline uint64_t toServerStamp(const double lslSeconds, const double offset)
{
    const double seconds = lslSeconds - offset;
    return seconds > 0 ? uint64_t(seconds * 1e9 + 0.5) : 0;
}

/// A stamp of the server's clock, in ns, as a time on LSL's clock, in seconds.
inline double toLslSeconds(const uint64_t stamp, const double offset) { return 1e-9 * double(stamp) + offset; }

}  // namespace LslClock
