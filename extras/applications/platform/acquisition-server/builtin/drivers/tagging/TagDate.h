/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, see speller.md
 */
#pragma once

#include <cstdint>

/// A tag older than this, or from the future, does not come from a clock on this PC: its time means nothing here.
static constexpr uint64_t MAX_TAG_AGE_NS = 2000000000ULL;

/**
 * @brief The date of a TCP tag in the driver's clock.
 *
 * A tag carries its time as System::Time::zgetTimeRaw(), which is fixed point seconds. Drivers date samples in nanoseconds of
 * the same steady clock, with another zero. So the tag's age is what carries over, and subtracting it gives the date.
 *
 * @param tagTime Time in the tag, fixed point.
 * @param now     The present, fixed point.
 * @param nowNs   The present, in the driver's clock.
 * @return The moment the tag stands for, or the present when the tag's time means nothing here.
 */
inline uint64_t tagDate(const uint64_t tagTime, const uint64_t now, const uint64_t nowNs)
{
    if (tagTime > now) { return nowNs; }

    const uint64_t age   = now - tagTime;
    const uint64_t ageNs = (age >> 32) * 1000000000ULL + (((age & 0xFFFFFFFFULL) * 1000000000ULL) >> 32);
    return ageNs > MAX_TAG_AGE_NS || ageNs > nowNs ? nowNs : nowNs - ageNs;
}
