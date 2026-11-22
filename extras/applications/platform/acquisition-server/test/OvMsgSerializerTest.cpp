/**
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * @file OvMsgSerializerTest.cpp
 * @brief Tests of the acquisition messages on the wire, the channel names among them.
 *        UG fork: new file, not in upstream OpenViBE.
 */

#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <cmath>

#include <openvibe/acquisition-messaging/ovChannelInfo.h>
#include <openvibe/acquisition-messaging/ovMsgSerializer.h>

namespace {

ovMsg configMessage(const std::string& names)
{
    ovMsg msg{};
    msg.msgType                                  = ovMsg::OV_MSG_ACQUISITION_CONFIG;
    msg.msgData.acquisitionConfigStruct.channelCount     = 3;
    msg.msgData.acquisitionConfigStruct.sampleRate       = 512;
    msg.msgData.acquisitionConfigStruct.sampleChunk      = 32;
    msg.msgData.acquisitionConfigStruct.channelNames     = names.data();
    msg.msgData.acquisitionConfigStruct.channelNamesSize = uint32_t(names.size());
    msg.msgSize                                  = uint32_t(5 * sizeof(uint32_t) + names.size());
    return msg;
}

}  // namespace

TEST(OvMsgSerializerTest, AcquisitionConfigCarriesTheChannelNames)
{
    const std::string names("Fp1\0\0Photodiode\0", 16);
    const auto [size, buffer] = serializeOvMsg(configMessage(names));
    const std::unique_ptr<uint8_t[]> owned(static_cast<uint8_t*>(buffer));
    ASSERT_EQ(size, 5 * sizeof(uint32_t) + names.size());

    const ovMsg msg = deserializeOvMsg(owned.get(), size);
    const auto& config = msg.msgData.acquisitionConfigStruct;
    EXPECT_EQ(msg.msgType, ovMsg::OV_MSG_ACQUISITION_CONFIG);
    EXPECT_EQ(config.channelCount, 3u);
    EXPECT_EQ(config.sampleRate, 512u);
    EXPECT_EQ(config.sampleChunk, 32u);
    ASSERT_EQ(config.channelNamesSize, names.size());
    EXPECT_EQ(std::string(config.channelNames, config.channelNamesSize), names);
    delete[] config.channelNames;
}

TEST(OvMsgSerializerTest, AcquisitionConfigWithoutNamesFromAnOlderServer)
{
    // What servers without channel names sent: the message header and the three counts
    const uint32_t wire[5] = {uint32_t(ovMsg::OV_MSG_ACQUISITION_CONFIG), 5 * sizeof(uint32_t), 8, 256, 16};

    const ovMsg msg = deserializeOvMsg(reinterpret_cast<const uint8_t*>(wire), sizeof(wire));
    EXPECT_EQ(msg.msgData.acquisitionConfigStruct.channelCount, 8u);
    EXPECT_EQ(msg.msgData.acquisitionConfigStruct.sampleChunk, 16u);
    EXPECT_EQ(msg.msgData.acquisitionConfigStruct.channelNamesSize, 0u);
    EXPECT_EQ(msg.msgData.acquisitionConfigStruct.channelNames, nullptr);
}

TEST(OvMsgSerializerTest, ChannelInfoCarriesTheMontage)
{
    ovChannelInfo info;
    info.names     = {"Fp1", "Cz", "HEOG"};
    info.montage   = "Refa-64.json";
    info.positions = {{true, 92.0, 108.0}, {true, 0.0, 0.0}, {}};

    const std::string block     = encodeChannelInfo(info, 3);
    const ovChannelInfo decoded = decodeChannelInfo(block.data(), block.size(), 3);
    EXPECT_EQ(decoded.names, info.names);
    EXPECT_EQ(decoded.montage, "Refa-64.json");
    ASSERT_EQ(decoded.positions.size(), 3u);
    EXPECT_TRUE(decoded.positions[0].known);
    EXPECT_DOUBLE_EQ(decoded.positions[0].theta, 92.0);
    EXPECT_DOUBLE_EQ(decoded.positions[0].phi, 108.0);
    EXPECT_TRUE(decoded.positions[1].known);
    EXPECT_FALSE(decoded.positions[2].known);

}

TEST(OvMsgSerializerTest, PositionsInOpenViBEsChannelLocalisationFrame)
{
    // The frame of share/openvibe/electrode_sets/electrode_set_standard_cartesian.txt: x to the nose, y to the left ear, z up
    const auto near = [](const ovCartesianPosition& p, const double x, const double y, const double z) {
        return std::abs(p.x - x) < 1e-9 && std::abs(p.y - y) < 1e-9 && std::abs(p.z - z) < 1e-9;
    };
    EXPECT_TRUE(near(toChannelLocalisation({true, 0.0, 0.0}), 0, 0, 1));     // Cz
    EXPECT_TRUE(near(toChannelLocalisation({true, 90.0, 0.0}), 0, -1, 0));   // T8
    EXPECT_TRUE(near(toChannelLocalisation({true, 90.0, 90.0}), 1, 0, 0));   // Fpz
    EXPECT_TRUE(near(toChannelLocalisation({true, 90.0, 180.0}), 0, 1, 0));  // T7

    // And back, as the BrainVision writer reads them
    for (const ovChannelInfo::Position position : {ovChannelInfo::Position{true, 92.0, 108.0}, ovChannelInfo::Position{true, 45.0, 270.0}}) {
        const ovChannelInfo::Position back = fromChannelLocalisation(toChannelLocalisation(position));
        EXPECT_TRUE(back.known);
        EXPECT_NEAR(back.theta, position.theta, 1e-9);
        EXPECT_NEAR(back.phi, position.phi, 1e-9);
    }
    EXPECT_FALSE(fromChannelLocalisation({0.0, 0.0, 0.0}).known);
}

TEST(OvMsgSerializerTest, ChannelInfoWithNamesOnly)
{
    // Without a montage the block is the names only, as servers before the montage sent it
    ovChannelInfo info;
    info.names = {"Fp1", "", "Photodiode"};
    const std::string block = encodeChannelInfo(info, 3);
    EXPECT_EQ(block, std::string("Fp1\0\0Photodiode\0", 16));

    const ovChannelInfo decoded = decodeChannelInfo(block.data(), block.size(), 3);
    EXPECT_EQ(decoded.names, info.names);
    EXPECT_TRUE(decoded.montage.empty());
    EXPECT_TRUE(decoded.positions.empty());

    // Nor does a missing block break it
    EXPECT_EQ(decodeChannelInfo(nullptr, 0, 2).names, (std::vector<std::string>{"", ""}));
}

// UG fork: the impedances of the last check travel after the positions (ovChannelInfo.h)
TEST(OvMsgSerializerTest, ChannelInfoCarriesTheImpedances)
{
    ovChannelInfo info;
    info.names         = {"Fp1", "Cz", "HEOG"};
    info.montage       = "Refa-64.json";
    info.positions     = {{true, 92.0, 108.0}, {true, 0.0, 0.0}, {}};
    info.impedanceTime = "2026-11-22 10:42:17";
    info.impedances    = {4.2, std::nan(""), 12.0};

    const std::string block     = encodeChannelInfo(info, 3);
    const ovChannelInfo decoded = decodeChannelInfo(block.data(), block.size(), 3);
    EXPECT_EQ(decoded.montage, "Refa-64.json");
    ASSERT_EQ(decoded.positions.size(), 3u);
    EXPECT_TRUE(decoded.positions[0].known);
    EXPECT_EQ(decoded.impedanceTime, "2026-11-22 10:42:17");
    ASSERT_EQ(decoded.impedances.size(), 3u);
    EXPECT_DOUBLE_EQ(decoded.impedances[0], 4.2);
    EXPECT_TRUE(std::isnan(decoded.impedances[1]));
    EXPECT_DOUBLE_EQ(decoded.impedances[2], 12.0);

    // A client from before the impedances reads the montage as it did: the same block, cut after the positions
    const std::string montageOnly = block.substr(0, block.find("impedances"));
    const ovChannelInfo older     = decodeChannelInfo(montageOnly.data(), montageOnly.size(), 3);
    EXPECT_EQ(older.montage, "Refa-64.json");
    EXPECT_TRUE(older.impedanceTime.empty());
    EXPECT_TRUE(older.impedances.empty());
}

TEST(OvMsgSerializerTest, ImpedancesWithoutAMontage)
{
    // eego's own check, without a montage: the montage and positions go empty, and are not kept
    ovChannelInfo info;
    info.names         = {"EEG1", "EEG2"};
    info.impedanceTime = "2026-11-22 10:42:17";
    info.impedances    = {3.0, 8.5};

    const std::string block     = encodeChannelInfo(info, 2);
    const ovChannelInfo decoded = decodeChannelInfo(block.data(), block.size(), 2);
    EXPECT_EQ(decoded.names, info.names);
    EXPECT_TRUE(decoded.montage.empty());
    EXPECT_TRUE(decoded.positions.empty());
    ASSERT_EQ(decoded.impedances.size(), 2u);
    EXPECT_DOUBLE_EQ(decoded.impedances[1], 8.5);
}

TEST(OvMsgSerializerTest, TheImpedanceColumnLabelCarriesTheTime)
{
    std::string time;
    EXPECT_TRUE(impedanceTimeFromColumnLabel(impedanceColumnLabel("2026-11-22 10:42:17"), time));
    EXPECT_EQ(time, "2026-11-22 10:42:17");
    EXPECT_FALSE(impedanceTimeFromColumnLabel("z", time));
}
