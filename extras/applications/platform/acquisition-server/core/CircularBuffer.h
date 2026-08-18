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

    void pushFreeBuffer (samplesBuffer* buff);
    void pushBufferToClean (samplesBuffer* buff);
    void pushBufferToProcess(samplesBuffer* buff);

    void processBuffer();
    void cleanBuffers();

    CircularBuffer& operator=(CircularBuffer const&) = delete;

    void setRunning(bool run) { m_bCancel = run; m_condBuffFree.notify_all(); }

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