// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.63. The impedances measured when a recording scenario starts reach the clients with the settings.

#include <gtest/gtest.h>

#include "Acquisition.h"
#include "AbstractDriver.h"
#include "AbstractWriter.h"

#include <openvibe/acquisition-messaging/ovChannelInfo.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

namespace {

constexpr auto SHORT = std::chrono::milliseconds(200);  // instead of the amplifier's seconds

// A driver with three channels, Fp1, a trigger and Cz, that measures impedances in ohm: zeros at first, as eego gives them, then
// 4.2 and 12 kOhm, none for the trigger
class ImpedanceDriver final : public AbstractDriver
{
public:
    explicit ImpedanceDriver(const bool canMeasure) : m_canMeasure(canMeasure)
    {
        m_channelCount = 3;
        m_channelNames.setValue({"Fp1", "Trigger", "Cz"});
    }

    bool init() override { return true; }
    bool unInit() override { return true; }
    bool start() override { return true; }
    bool stop() override { return true; }
    int read(sampleType* /*data*/, uint64_t /*size*/) override { return 0; }
    void setChunkSize(uint32_t /*chunkSize*/) override { }

    bool supportsImpedanceCheck() const override { return m_canMeasure; }
    bool startImpedanceCheck() override { return m_measuring = m_canMeasure; }
    bool stopImpedanceCheck() override
    {
        m_measuring = false;
        return true;
    }
    int readImpedances(double* values, const size_t count) override
    {
        if (!m_measuring || count < 3) { return 0; }
        ++reads;
        const bool first = reads == 1;
        values[0]        = first ? 0.0 : 4200.0;
        values[1]        = std::nan("");
        values[2]        = first ? 0.0 : 12000.0;
        return 3;
    }

    std::atomic<int> reads{0};

private:
    const bool m_canMeasure;
    bool m_measuring = false;
};

// The channel information sent with the settings
class InfoWriter final : public AbstractWriter
{
public:
    InfoWriter() : AbstractWriter("test") { }
    std::vector<ovChannelInfo> sent;

protected:
    size_t send(ovMsg* msg, const uint64_t /*clientId*/) override
    {
        if (msg->msgType != ovMsg::OV_MSG_ACQUISITION_CONFIG) { return 0; }
        const auto& config = msg->msgData.acquisitionConfigStruct;
        sent.push_back(decodeChannelInfo(config.channelNames, config.channelNamesSize, config.channelCount));
        return msg->msgSize;
    }
};

constexpr uint64_t CLIENT = 7;

void expectImpedances(const ovChannelInfo& info)
{
    EXPECT_FALSE(info.impedanceTime.empty());
    ASSERT_EQ(info.impedances.size(), 3U);
    EXPECT_DOUBLE_EQ(info.impedances[0], 4.2);
    EXPECT_TRUE(std::isnan(info.impedances[1])) << "a channel the amplifier gave none for has none";
    EXPECT_DOUBLE_EQ(info.impedances[2], 12.0);
}

}  // namespace

TEST(AcquisitionImpedanceTest, ARunningAcquisitionStopsForTheImpedancesAndSendsThemWhenItRunsAgain)
{
    Acquisition acquisition;
    auto* driver = new ImpedanceDriver(true);
    acquisition.addDriver(DriverPtr(driver), "impedances");
    auto* writer = new InfoWriter;  // the acquisition owns it
    acquisition.addWriter(writer);
    acquisition.addClient(CLIENT);

    acquisition.start();
    ASSERT_TRUE(acquisition.isRunning());
    ASSERT_FALSE(writer->sent.empty());
    EXPECT_TRUE(writer->sent.back().impedances.empty()) << "none measured yet";

    std::string outcome;
    EXPECT_TRUE(acquisition.measureImpedances(outcome, SHORT)) << outcome;
    EXPECT_GE(driver->reads.load(), 2) << "the zeros of the first reading are not kept";
    EXPECT_TRUE(acquisition.isRunning()) << "it runs again";
    expectImpedances(writer->sent.back());

    acquisition.stop();
}

TEST(AcquisitionImpedanceTest, WithoutAnAcquisitionRunningTheImpedancesWaitForTheNextOne)
{
    Acquisition acquisition;
    acquisition.addDriver(DriverPtr(new ImpedanceDriver(true)), "impedances");
    auto* writer = new InfoWriter;
    acquisition.addWriter(writer);

    std::string outcome;
    EXPECT_TRUE(acquisition.measureImpedances(outcome, SHORT)) << outcome;
    EXPECT_FALSE(acquisition.isRunning()) << "nothing is started that was not running";

    acquisition.addClient(CLIENT);
    acquisition.start();
    ASSERT_FALSE(writer->sent.empty());
    expectImpedances(writer->sent.back());
    acquisition.stop();
}

TEST(AcquisitionImpedanceTest, ADriverThatCanNotMeasureLeavesTheAcquisitionRunningWithoutImpedances)
{
    Acquisition acquisition;
    acquisition.addDriver(DriverPtr(new ImpedanceDriver(false)), "no impedances");
    auto* writer = new InfoWriter;
    acquisition.addWriter(writer);
    acquisition.addClient(CLIENT);
    acquisition.start();

    std::string outcome;
    EXPECT_FALSE(acquisition.measureImpedances(outcome, SHORT));
    EXPECT_FALSE(outcome.empty());
    EXPECT_TRUE(acquisition.isRunning());
    ASSERT_FALSE(writer->sent.empty());
    EXPECT_TRUE(writer->sent.back().impedances.empty());
    acquisition.stop();
}

TEST(AcquisitionImpedanceTest, PlayAfterTheServersOwnImpedanceCheckStartsTheAcquisition)
{
    Acquisition acquisition;
    acquisition.addDriver(DriverPtr(new ImpedanceDriver(true)), "impedances");
    ASSERT_TRUE(acquisition.startImpedanceCheck());
    acquisition.start();
    EXPECT_TRUE(acquisition.isRunning()) << "upstream left impedance mode and started nothing";
    EXPECT_EQ(acquisition.state(), Acquisition::State::Streaming);
    acquisition.stop();
}
