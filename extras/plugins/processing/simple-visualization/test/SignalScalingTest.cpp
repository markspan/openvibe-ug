// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.26. Tests the Signal display's automatic vertical scale.

#include "SignalScaling.h"

#include <gtest/gtest.h>

#include <limits>

using namespace OpenViBE::Plugins::SimpleVisualization;

TEST(SignalScaling, electrodeNamesAreEEG)
{
    for (const char* name : { "Fp1", "Cz", "O2", "A12", "TP10", "Channel 3", "EXG1" }) { EXPECT_EQ(channelKind(name), EChannelKind::EEG) << name; }
}

TEST(SignalScaling, eogNamesAreEOG)
{
    for (const char* name : { "HEOG", "VEOG", "EOG L", "eog_r", "HEO", "VEO" }) { EXPECT_EQ(channelKind(name), EChannelKind::EOG) << name; }
}

TEST(SignalScaling, triggerAndCounterChannelsAreDigital)
{
    for (const char* name : { "Trigger", "SampleCounter", "Digi", "DIG", "Saw", "Status", "Markers" }) {
        EXPECT_EQ(channelKind(name), EChannelKind::Digital) << name;
    }
}

TEST(SignalScaling, otherBodySignalsAndAmplifierInputsAreOther)
{
    for (const char* name : { "ECG", "EMG left", "GSR", "Resp", "BIP1", "AUX2", "Temp" }) { EXPECT_EQ(channelKind(name), EChannelKind::Other) << name; }
}

TEST(SignalScaling, halfRangesRoundUpToOneTwoOrFive)
{
    EXPECT_DOUBLE_EQ(niceHalfRange(37.0), 50.0);
    EXPECT_DOUBLE_EQ(niceHalfRange(50.0), 50.0);
    EXPECT_DOUBLE_EQ(niceHalfRange(51.0), 100.0);
    EXPECT_DOUBLE_EQ(niceHalfRange(120.0), 200.0);
    EXPECT_DOUBLE_EQ(niceHalfRange(0.0031), 0.005);
    EXPECT_DOUBLE_EQ(niceHalfRange(1.0), 1.0);
}

TEST(SignalScaling, aFlatOrUnknownHalfRangeIsOne)
{
    EXPECT_DOUBLE_EQ(niceHalfRange(0.0), 1.0);
    EXPECT_DOUBLE_EQ(niceHalfRange(-3.0), 1.0);
    EXPECT_DOUBLE_EQ(niceHalfRange(std::numeric_limits<double>::quiet_NaN()), 1.0);
}

TEST(SignalScaling, eachChannelIsCentredOnItsOwnSpan)
{
    // One group: the half-range is shared, but the DC offsets are not
    const auto scales = scaleChannels({ { 990.0, 1010.0, true }, { -30.0, 10.0, true } }, { 0, 0 });
    ASSERT_EQ(scales.size(), 2U);
    EXPECT_DOUBLE_EQ(scales[0].centre, 1000.0);
    EXPECT_DOUBLE_EQ(scales[1].centre, -10.0);
}

TEST(SignalScaling, aGroupSharesTheLargestHalfRange)
{
    // Half-ranges 10 and 20; 20 with headroom is 21, rounded up to 50
    const auto scales = scaleChannels({ { -10.0, 10.0, true }, { 80.0, 120.0, true } }, { 7, 7 });
    EXPECT_DOUBLE_EQ(scales[0].halfRange, 50.0);
    EXPECT_DOUBLE_EQ(scales[1].halfRange, 50.0);
}

TEST(SignalScaling, separateGroupsKeepTheirOwnHalfRange)
{
    // EEG of ±40 and a trigger channel of 0 to 255, scaled by type
    const auto scales = scaleChannels({ { -40.0, 40.0, true }, { -35.0, 30.0, true }, { 0.0, 255.0, true } }, { -2, -2, -4 });
    EXPECT_DOUBLE_EQ(scales[0].halfRange, 50.0);
    EXPECT_DOUBLE_EQ(scales[1].halfRange, 50.0);
    EXPECT_DOUBLE_EQ(scales[2].halfRange, 200.0);
    EXPECT_DOUBLE_EQ(scales[2].centre, 127.5);
}

TEST(SignalScaling, aChannelWithoutNumbersDoesNotCountForItsGroup)
{
    const auto scales = scaleChannels({ { -10.0, 10.0, true }, { -1e6, 1e6, false } }, { 0, 0 });
    EXPECT_DOUBLE_EQ(scales[0].halfRange, 20.0);
    EXPECT_DOUBLE_EQ(scales[1].halfRange, SChannelScale().halfRange);
}
