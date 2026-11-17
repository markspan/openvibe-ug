// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.30. A client that is connected when the acquisition is set up anew gets the new settings.

#include <gtest/gtest.h>

#include "Acquisition.h"
#include "AbstractDriver.h"
#include "AbstractWriter.h"

#include <vector>

namespace {

// A driver that only has settings
class SettingsDriver final : public AbstractDriver
{
public:
    bool init() override { return true; }
    bool unInit() override { return true; }
    bool start() override { return true; }
    bool stop() override { return true; }
    int read(sampleType* /*data*/, uint64_t /*size*/) override { return 0; }
    void setChunkSize(uint32_t /*chunkSize*/) override { }
};

// The settings sent, and to whom
class ConfigWriter final : public AbstractWriter
{
public:
    ConfigWriter() : AbstractWriter("test") { }

    struct Sent
    {
        uint64_t client;
        uint32_t sampleRate, channels, chunk;
    };
    std::vector<Sent> sent;

protected:
    size_t send(ovMsg* msg, const uint64_t clientId) override
    {
        if (msg->msgType != ovMsg::OV_MSG_ACQUISITION_CONFIG) { return 0; }
        const auto& config = msg->msgData.acquisitionConfigStruct;
        if (clientId != uint64_t(-1)) { sent.push_back({clientId, config.sampleRate, config.channelCount, config.sampleChunk}); }
        else {
            for (const uint64_t client : getClients()) { sent.push_back({client, config.sampleRate, config.channelCount, config.sampleChunk}); }
        }
        return msg->msgSize;
    }
};

constexpr uint64_t CLIENT = 5;

}  // namespace

TEST(AcquisitionConfigTest, AConnectedClientGetsTheSettingsOfANewAcquisition)
{
    Acquisition acquisition;
    acquisition.addDriver(DriverPtr(new SettingsDriver), "settings");
    auto* writer = new ConfigWriter;  // the acquisition owns it
    acquisition.addWriter(writer);

    acquisition.getChunkSampleCount()->setValue(4);
    acquisition.initAcquisition();
    EXPECT_TRUE(writer->sent.empty()) << "nobody connected yet";

    acquisition.addClient(CLIENT);
    ASSERT_EQ(writer->sent.size(), 1U);
    EXPECT_EQ(writer->sent[0].chunk, 4U);

    // The server restarted with blocks of 64, while the client stays connected: as when the client kept 4 samples of every 64
    acquisition.getChunkSampleCount()->setValue(64);
    acquisition.initAcquisition();
    ASSERT_EQ(writer->sent.size(), 2U) << "the connected client is told";
    EXPECT_EQ(writer->sent[1].client, CLIENT);
    EXPECT_EQ(writer->sent[1].chunk, 64U);
    EXPECT_EQ(writer->sent[1].sampleRate, writer->sent[0].sampleRate);
    EXPECT_EQ(writer->sent[1].channels, writer->sent[0].channels);
}
