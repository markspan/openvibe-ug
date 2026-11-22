// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.64. The LSL Export box dates what it sends on LSL's clock, by its age in the scenario.

#include "LSLTime.h"

#include <gtest/gtest.h>

#include <cstdint>

using OpenViBE::Plugins::NetworkIO::lslTimeOfScenarioDate;

namespace {
uint64_t seconds(const double s) { return uint64_t(s * 4294967296.0); }
}  // namespace

TEST(LSLTime, aStimulationOfThisRoundIsNow)
{
    EXPECT_DOUBLE_EQ(lslTimeOfScenarioDate(seconds(12.5), seconds(12.5), 85947.25), 85947.25);
}

TEST(LSLTime, anOlderOneIsAsOldOnLSLsClock)
{
    EXPECT_NEAR(lslTimeOfScenarioDate(seconds(12.0), seconds(12.5), 85947.25), 85946.75, 1e-9);
}

TEST(LSLTime, theScenarioTimeOfTheFirstChunkDoesNotShiftIt)
{
    // Upstream took the box's first chunk for the scenario's time 0: a box that first ran at 2.5 s sent everything 2.5 s late
    const double first = lslTimeOfScenarioDate(seconds(2.5), seconds(2.5), 1000.0);
    const double later = lslTimeOfScenarioDate(seconds(7.5), seconds(7.5), 1005.0);
    EXPECT_DOUBLE_EQ(first, 1000.0);
    EXPECT_DOUBLE_EQ(later - first, 5.0);
}

TEST(LSLTime, aDateAheadOfTheScenarioIsInTheFuture)
{
    EXPECT_NEAR(lslTimeOfScenarioDate(seconds(3.25), seconds(3.0), 500.0), 500.25, 1e-9);
}
