/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file. Triggers without the values caught while the trigger lines switch. See UG.md, 2.25.
 */
#include <SteadyTrigger.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

// Feeds one value per sample, sample i stamped 1000 * (i + 1), and returns the changes
std::vector<SteadyTrigger::Change> follow(SteadyTrigger& trigger, const std::vector<uint32_t>& values)
{
    std::vector<SteadyTrigger::Change> changes;
    for (size_t i = 0; i < values.size(); ++i) {
        if (const auto change = trigger.feed(values[i], 1000 * (i + 1))) { changes.push_back(*change); }
    }
    return changes;
}

std::vector<SteadyTrigger::Change> follow(const std::vector<uint32_t>& values)
{
    SteadyTrigger trigger;
    trigger.reset(0);
    return follow(trigger, values);
}

}  // namespace

TEST(SteadyTriggerTest, aCleanPulseIsOneChangeDatedAtItsFirstSample)
{
    const auto changes = follow({0, 0, 110, 110, 110, 0, 0, 0});
    ASSERT_EQ(changes.size(), 2u);
    EXPECT_EQ(changes[0].value, 110u);
    EXPECT_EQ(changes[0].previous, 0u);
    EXPECT_EQ(changes[0].onset, 3000u);
    EXPECT_EQ(changes[1].value, 0u);
    EXPECT_EQ(changes[1].previous, 110u);
    EXPECT_EQ(changes[1].onset, 6000u);
}

// What the lab saw: switching on to 110, one line late, gave a trigger 108 one sample before the 110
TEST(SteadyTriggerTest, aValueCaughtWhileSwitchingOnIsPassedOverButDatesTheChange)
{
    const auto changes = follow({0, 108, 110, 110, 110});
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes[0].value, 110u);
    EXPECT_EQ(changes[0].previous, 0u);
    EXPECT_EQ(changes[0].onset, 2000u);
}

TEST(SteadyTriggerTest, aValueCaughtWhileSwitchingOffIsPassedOver)
{
    const auto changes = follow({110, 110, 108, 0, 0});
    ASSERT_EQ(changes.size(), 2u);
    EXPECT_EQ(changes[1].value, 0u);
    EXPECT_EQ(changes[1].previous, 110u);
    EXPECT_EQ(changes[1].onset, 3000u);
}

TEST(SteadyTriggerTest, aOneSampleBlipIsNoChange)
{
    EXPECT_TRUE(follow({0, 0, 4, 0, 0, 0}).empty());
}

TEST(SteadyTriggerTest, codesSentBackToBackAreEachAChange)
{
    const auto changes = follow({0, 5, 5, 7, 7, 0, 0});
    ASSERT_EQ(changes.size(), 3u);
    EXPECT_EQ(changes[1].value, 7u);
    EXPECT_EQ(changes[1].previous, 5u);
    EXPECT_EQ(changes[1].onset, 4000u);
}

TEST(SteadyTriggerTest, aChangeCarriesOverFromOneReadToTheNext)
{
    SteadyTrigger trigger;
    trigger.reset(0);
    EXPECT_FALSE(trigger.feed(0, 1000));
    EXPECT_FALSE(trigger.feed(108, 2000));  // the last sample of one read ...
    const auto change = trigger.feed(110, 3000);
    EXPECT_FALSE(change);
    const auto settled = trigger.feed(110, 4000);  // ... and the next read
    ASSERT_TRUE(settled);
    EXPECT_EQ(settled->value, 110u);
    EXPECT_EQ(settled->onset, 2000u);
}

TEST(SteadyTriggerTest, aChannelAlreadyOnAtTheStartIsAChangeWhenTheStartIsKnown)
{
    const auto changes = follow({3, 3, 3});
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes[0].value, 3u);
    EXPECT_EQ(changes[0].onset, 1000u);
}

TEST(SteadyTriggerTest, anUnknownStartTakesTheFirstValueSilently)
{
    SteadyTrigger trigger;
    trigger.resetUnknown();
    const auto changes = follow(trigger, {3, 3, 3, 7, 7});
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes[0].value, 7u);
    EXPECT_EQ(changes[0].previous, 3u);
}
