/**
 * @file CircularBufferTest.cpp
 * @brief Tests of the circular buffer's waiting and of its free buffer count.
 *
 * UG fork: new file, not in upstream OpenViBE, whose processing and cleaning threads poll the buffer instead of
 * waiting on it.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>

#include "CircularBuffer.h"
#include "SampleBufferHandler.h"

namespace {

constexpr uint32_t BUFFER_COUNT = 4;
constexpr auto TIMEOUT          = std::chrono::seconds(5);

class CircularBufferTest : public ::testing::Test
{
protected:
    SampleBufferHandler m_handler{8, 512, 32};
    std::unique_ptr<CircularBuffer> m_buffer = std::make_unique<CircularBuffer>(&m_handler, BUFFER_COUNT);
};

TEST_F(CircularBufferTest, WaitBufferToProcessWaitsForAPushedBuffer)
{
    auto waiting = std::async(std::launch::async, [this] { return m_buffer->waitBufferToProcess(); });
    EXPECT_EQ(waiting.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);

    samplesBuffer* buff = m_buffer->popFreeBuffer();
    m_buffer->pushBufferToProcess(buff);

    ASSERT_EQ(waiting.wait_for(TIMEOUT), std::future_status::ready);
    EXPECT_EQ(waiting.get(), buff);
    m_buffer->pushFreeBuffer(buff);
}

TEST_F(CircularBufferTest, WaitBufferToProcessReturnsNothingWhenStopped)
{
    auto waiting = std::async(std::launch::async, [this] { return m_buffer->waitBufferToProcess(); });
    EXPECT_EQ(waiting.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);

    m_buffer->setRunning(false);

    ASSERT_EQ(waiting.wait_for(TIMEOUT), std::future_status::ready);
    EXPECT_EQ(waiting.get(), nullptr);
}

TEST_F(CircularBufferTest, CleanBuffersWaitsForABufferAndReturnsItToTheFreeOnes)
{
    samplesBuffer* buff = m_buffer->popFreeBuffer();
    EXPECT_EQ(m_buffer->takeMinFreeCount(), BUFFER_COUNT - 1);

    auto cleaning = std::async(std::launch::async, [this] { m_buffer->cleanBuffers(); });
    EXPECT_EQ(cleaning.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);

    m_buffer->pushBufferToClean(buff);
    ASSERT_EQ(cleaning.wait_for(TIMEOUT), std::future_status::ready);
    // One was still taken when the count was last reset; the next reset finds all of them free again
    EXPECT_EQ(m_buffer->takeMinFreeCount(), BUFFER_COUNT - 1);
    EXPECT_EQ(m_buffer->takeMinFreeCount(), BUFFER_COUNT);
}

TEST_F(CircularBufferTest, CleanBuffersReturnsWhenStopped)
{
    auto cleaning = std::async(std::launch::async, [this] { m_buffer->cleanBuffers(); });
    EXPECT_EQ(cleaning.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);

    m_buffer->setRunning(false);
    EXPECT_EQ(cleaning.wait_for(TIMEOUT), std::future_status::ready);
}

TEST_F(CircularBufferTest, TakeMinFreeCountReportsTheFewestSinceThePreviousCall)
{
    EXPECT_EQ(m_buffer->bufferCount(), BUFFER_COUNT);
    EXPECT_EQ(m_buffer->takeMinFreeCount(), BUFFER_COUNT);

    samplesBuffer* first  = m_buffer->popFreeBuffer();
    samplesBuffer* second = m_buffer->popFreeBuffer();
    m_buffer->pushFreeBuffer(first);
    m_buffer->pushFreeBuffer(second);

    // Both were taken at one point, though both are back by now
    EXPECT_EQ(m_buffer->takeMinFreeCount(), BUFFER_COUNT - 2);
    EXPECT_EQ(m_buffer->takeMinFreeCount(), BUFFER_COUNT);
}

}  // namespace
