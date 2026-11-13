/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file circularBuffer.cpp
 * @brief Implementation file for the CircularBuffer class.
 *
 * This file implements the methods of the `CircularBuffer` class, which provides
 * a thread-safe circular buffer for managing `samplesBuffer` objects.
 */
#include "CircularBuffer.h"

#include <algorithm>  // UG fork

 /**
  * @brief Constructor for the CircularBuffer class.
  *
  * Initializes the CircularBuffer with the provided SampleBufferHandler and
  * allocates the internal round buffer.
  *
  * @param sampleBufferHandler Pointer to the SampleBufferHandler.
  */
CircularBuffer::CircularBuffer(SampleBufferHandler* sampleBufferHandler, uint32_t internalBufferCount) : m_sampleBufferHandler(sampleBufferHandler), m_internalBufferCount(internalBufferCount)
{
    m_allBuffers = new samplesBuffer*[m_internalBufferCount]();
    allocateRoundBuffer();
    m_minFreeCount = m_internalBufferCount;  // UG fork: see takeMinFreeCount()
    m_bCancel = true;
}

/**
 * @brief Destructor for the CircularBuffer class.
 *
 * Frees the allocated round buffer.
 */
CircularBuffer::~CircularBuffer()
{
    setRunning(false);
    freeRoundBuffer();
    delete[] m_allBuffers;
}

/**
 * @brief Pops a free buffer from the free buffer queue.
 *
 * This method waits indefinitely until a free buffer becomes available. It
 * acquires a lock on the free buffer queue before accessing it.
 *
 * @return A pointer to a free samplesBuffer.
 */
samplesBuffer* CircularBuffer::popFreeBuffer()
{
    std::unique_lock<std::mutex> lock(m_mutexBuffFree);
    m_condBuffFree.wait(lock, [this] {
        return !m_buffFree.empty() || !m_bCancel; 
    });

    if (!m_bCancel) { 
        return nullptr; 
    }
    auto* buff = m_buffFree.front();
    m_buffFree.pop_front();
    m_minFreeCount = std::min(m_minFreeCount, uint32_t(m_buffFree.size()));  // UG fork
    return buff;
}

uint32_t CircularBuffer::takeMinFreeCount()
{
    std::lock_guard<std::mutex> lock(m_mutexBuffFree);
    const uint32_t count = m_minFreeCount;
    m_minFreeCount       = uint32_t(m_buffFree.size());
    return count;
}

void CircularBuffer::setRunning(const bool run)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexBuffFree);
        m_bCancel = run;
    }
    // Taking the other locks orders the change before any wait those queues' threads are about to start
    { std::lock_guard<std::mutex> lock(m_mutexBuffToClean); }
    { std::lock_guard<std::mutex> lock(m_mutexBuffToProcess); }
    m_condBuffFree.notify_all();
    m_condBuffToClean.notify_all();
    m_condBuffToProcess.notify_all();
}

/**
 * @brief Pops a buffer to be cleaned from the clean buffer queue.
 *
 * Acquires a lock on the clean buffer queue and retrieves the front buffer if
 * the queue is not empty.
 *
 * @return A pointer to a samplesBuffer to be cleaned, or nullptr if the queue is empty.
 */
samplesBuffer* CircularBuffer::popBufferToClean()
{
    samplesBuffer* buffRes = nullptr;

    std::lock_guard<std::mutex> lockClean(m_mutexBuffToClean);
    if (m_buffToClean.size() > 0)
    {
        buffRes = m_buffToClean.front();
        m_buffToClean.pop_front();
    }

    return buffRes;
}

/**
 * @brief Pops a buffer to be processed from the process buffer queue.
 *
 * Acquires a lock on the process buffer queue and retrieves the front buffer if
 * the queue is not empty.
 *
 * @return A pointer to a samplesBuffer to be processed, or nullptr if the queue is empty.
 */
samplesBuffer* CircularBuffer::popBufferToProcess()
{
    samplesBuffer* buffRes = nullptr;

    std::lock_guard<std::mutex> lockProcess(m_mutexBuffToProcess);
    if (m_buffToProcess.size() > 0)
    {
        buffRes = m_buffToProcess.front();
        m_buffToProcess.pop_front();
    }

    return buffRes;
}

// UG fork: upstream's reader polls popBufferToProcess() in a loop; this waits instead
samplesBuffer* CircularBuffer::waitBufferToProcess()
{
    std::unique_lock<std::mutex> lock(m_mutexBuffToProcess);
    m_condBuffToProcess.wait(lock, [this] { return !m_buffToProcess.empty() || !m_bCancel; });
    if (m_buffToProcess.empty()) { return nullptr; }

    samplesBuffer* buff = m_buffToProcess.front();
    m_buffToProcess.pop_front();
    return buff;
}

/**
 * @brief Pushes a buffer onto the free buffer queue.
 *
 * Acquires a lock on the free buffer queue before adding the buffer.
 *
 * @param buff Pointer to the samplesBuffer to push.
 */
void CircularBuffer::pushFreeBuffer(samplesBuffer* buff)
{
    {
        std::lock_guard<std::mutex> lockFree(m_mutexBuffFree);
        m_buffFree.push_back(buff);
    }
    m_condBuffFree.notify_one();
}

/**
 * @brief Pushes a buffer onto the buffer-to-clean queue.
 *
 * Acquires a lock on the clean buffer queue before adding the buffer.
 *
 * @param buff Pointer to the samplesBuffer to push.
 */
void CircularBuffer::pushBufferToClean(samplesBuffer* buff)
{
    {
        std::lock_guard<std::mutex> lockClean(m_mutexBuffToClean);
        m_buffToClean.push_back(buff);
    }
    m_condBuffToClean.notify_one();  // UG fork
}

/**
 * @brief Pushes a buffer onto the buffer-to-process queue.
 *
 * Acquires a lock on the process buffer queue before adding the buffer.
 *
 * @param buff Pointer to the samplesBuffer to push.
 */
void CircularBuffer::pushBufferToProcess(samplesBuffer* buff)
{
    {
        std::lock_guard<std::mutex> lockProcess(m_mutexBuffToProcess);
        m_buffToProcess.push_back(buff);
    }
    m_condBuffToProcess.notify_one();  // UG fork
}

/**
 * @brief Processes a buffer (currently moves it to the clean queue).
 *
 * This method pops a buffer from the process queue and immediately pushes it
 * onto the clean queue. This is a placeholder for actual processing logic.
 */
void CircularBuffer::processBuffer()
{
    samplesBuffer* buff = nullptr;
    while ((buff = popBufferToProcess()))
    {
        //interpolateTimestamps(buff, m_lastTimestamps); // Placeholder for potential processing.

        pushBufferToClean(buff);
    }
}

/**
 * @brief Cleans buffers by popping them from the clean queue and returning them to the free queue.
 *
 * This method waits until there is a buffer to clean (or setRunning(false) is called), then pops
 * buffers from the clean queue, uses the SampleBufferHandler to clean their contents, and pushes
 * them back onto the free queue for reuse.
 *
 * UG fork: upstream returns at once when there is nothing to clean, so that its caller's loop keeps a
 * CPU core busy for the whole acquisition.
 */
void CircularBuffer::cleanBuffers()
{
    {
        std::unique_lock<std::mutex> lock(m_mutexBuffToClean);
        m_condBuffToClean.wait(lock, [this] { return !m_buffToClean.empty() || !m_bCancel; });
    }

    samplesBuffer* buff = nullptr;
    while ((buff = popBufferToClean()))
    {
        m_sampleBufferHandler->cleanBuffer(buff);
        pushFreeBuffer(buff);
    }
}

/**
 * @brief Allocates the round buffer (internal pool of samplesBuffer objects).
 *
 * This method creates a fixed number of samplesBuffer objects using the
 * provided SampleBufferHandler and adds them to the free buffer queue.
 */
void CircularBuffer::allocateRoundBuffer()
{
    for (uint32_t i = 0; i < m_internalBufferCount; ++i)
    {
        samplesBuffer* buff = m_sampleBufferHandler->createBuffer();
        m_allBuffers[i]     = buff;
        pushFreeBuffer(buff);
    }
}

/**
 * @brief Frees the round buffer (internal pool of samplesBuffer objects).
 *
 * This method iterates through the allocated samplesBuffer objects and releases
 * them using the provided SampleBufferHandler.
 */
void CircularBuffer::freeRoundBuffer()
{
    for (uint32_t i = 0; i<m_internalBufferCount; i++)
    {
        m_sampleBufferHandler->removeBuffer(m_allBuffers[i]);
    }
}
