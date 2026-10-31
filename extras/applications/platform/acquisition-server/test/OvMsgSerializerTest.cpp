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
