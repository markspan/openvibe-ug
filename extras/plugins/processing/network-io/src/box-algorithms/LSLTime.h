/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, not in upstream OpenViBE. The LSL time of what a scenario dates, for the LSL Export box. See UG.md, 2.64.
 */
#pragma once

#include <cstdint>

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {

/**
 * @brief The time on LSL's clock (lsl::local_clock()) of something the scenario dated @p date.
 *
 * What is so old in the scenario's time is as old on LSL's clock: a stimulation made in this round is "now", one made half a second
 * of scenario time ago was made half a second ago. This is what LSL asks of an outlet that knows the age of its samples
 * (local_clock() minus the age), and it holds however late the player runs, as a box's stimulations are dated when the box runs.
 *
 * UG fork: upstream added the dates to the wall-clock time of the box's first chunk, taken to be the scenario's time 0, so that
 * every time was off by the scenario time of that first chunk; with the LSL_UseOVTimestamps token, it sent the scenario's own
 * seconds, which no other LSL stream can be aligned with (sccn/lsl_archived #100, #333).
 *
 * @param date        The date, in OpenViBE's 32:32 fixed point seconds.
 * @param scenarioNow The scenario's time now (getPlayerContext().getCurrentTime()), same unit.
 * @param lslNow      lsl::local_clock() now, in seconds.
 */
inline double lslTimeOfScenarioDate(const uint64_t date, const uint64_t scenarioNow, const double lslNow)
{
    const double age = date <= scenarioNow ? double(scenarioNow - date) : -double(date - scenarioNow);
    return lslNow - age / 4294967296.0;
}

}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE
