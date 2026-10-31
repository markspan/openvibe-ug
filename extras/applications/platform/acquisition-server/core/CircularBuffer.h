/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file CircularBuffer.h
 * @brief Header file for the CircularBuffer class.
 *
 * This file declares the `CircularBuffer` class, which provides a thread-safe
 * circular buffer implementation for managing `samplesBuffer` objects. It uses
 * multiple internal queues to facilitate the flow of buffers between different
 * stages of the data acquisition process.
 */
#pragma once

#include "samplesBuffer.h"
#include "SampleBufferHandler.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdint>
#include <deque>

 /**
  * @def INTERNAL_BUFFERS_COUNT
  * @brief Defines the number of internal buffers managed by the CircularBuffer.
  */
#define INTERNAL_BUFFERS_COUNT 10

  /**
   * @brief A thread-safe circular buffer for managing samplesBuffer objects.
   *
   * The `CircularBuffer` class provides a mechanism for storing and retrieving
   * `samplesBuffer` objects in a concurrent environment. It utilizes three internal
   * deques (`m_buffFree`, `m_buffToClean`, `m_buffToProcess`) along with mutexes
   * to ensure thread-safe access. It works in conjunction with the
   * `SampleBufferHandler` for allocating and managing the underlying buffers.
   */
class CircularBuffer
{
public:

    CircularBuffer(SampleBufferHandler* sampleBufferHandler, uint32_t internalBufferCount = INTERNAL_BUFFERS_COUNT);
    CircularBuffer(CircularBuffer const &) = delete;
    ~CircularBuffer();


    samplesBuffer* popFreeBuffer();
    samplesBuffer* popBufferToClean();
    samplesBuffer* popBufferToProcess();
    /**
     * @brief Waits for a buffer to process and pops it.
     * @return The buffer, or nullptr once setRunning(false) is called.
     *
     * UG fork: upstream polls popBufferToProcess() in a loop, which keeps a CPU core busy for the whole acquisition.
     */
    samplesBuffer* waitBufferToProcess();

    void pushFreeBuffer (samplesBuffer* buff);
    void pushBufferToClean (samplesBuffer* buff);
    void pushBufferToProcess(samplesBuffer* buff);

    void processBuffer();
    void cleanBuffers();

    CircularBuffer& operator=(CircularBuffer const&) = delete;

    /**
     * @brief Starts or ends the acquisition; ending it wakes every thread that waits on this buffer.
     *
     * UG fork: sets the flag under each queue's lock, so that a thread that is about to wait cannot miss the wake-up.
     */
    void setRunning(bool run);

    // UG fork: the free buffer count, which the server shows to tell how close the acquisition is to stalling
    /// Number of buffers in the pool.
    uint32_t bufferCount() const { return m_internalBufferCount; }
    /// Fewest free buffers there were since the previous call (or since the pool was made), which it then resets.
    uint32_t takeMinFreeCount();

private:
    /**
     * @brief Allocates the internal pool of `samplesBuffer` objects.
     */
    void allocateRoundBuffer();

    /**
     * @brief Frees the internal pool of `samplesBuffer` objects.
     */
    void freeRoundBuffer();


private:
    using byte = unsigned char; ///< Alias for unsigned char.

    std::atomic<bool> m_bCancel;

    std::condition_variable m_condBuffFree;
    std::condition_variable m_condBuffToClean;    ///< UG fork: signalled when a buffer is pushed to clean
    std::condition_variable m_condBuffToProcess;  ///< UG fork: signalled when a buffer is pushed to process
    uint32_t m_minFreeCount = 0;                  ///< UG fork: see takeMinFreeCount(); guarded by m_mutexBuffFree
    std::mutex m_mutexBuffFree; ///< Mutex to protect access to the free buffer queue.
    std::mutex m_mutexBuffToClean; ///< Mutex to protect access to the buffer-to-clean queue.
    std::mutex m_mutexBuffToProcess; ///< Mutex to protect access to the buffer-to-process queue.

    std::deque<samplesBuffer*> m_buffFree; ///< Queue of free `samplesBuffer` objects.
    std::deque<samplesBuffer*> m_buffToClean; ///< Queue of `samplesBuffer` objects to be cleaned.
    std::deque<samplesBuffer*> m_buffToProcess; ///< Queue of `samplesBuffer` objects to be processed.

    //samplesBuffer* m_allBuffers[INTERNAL_BUFFERS_COUNT]; ///< Array to hold all allocated `samplesBuffer` objects.
    samplesBuffer** m_allBuffers; ///< Array to hold all allocated `samplesBuffer` objects.
    uint32_t m_internalBufferCount;

    SampleBufferHandler* m_sampleBufferHandler; ///< Pointer to the `SampleBufferHandler` instance.
};