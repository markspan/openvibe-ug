/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file ThreadedBuffer.SampleBufferHandler.cpp
 * @brief Implementation file for the SampleBufferHandler class.
 *
 * This file implements the SampleBufferHandler class, which is responsible for managing
 * buffers used to store samples and their timestamps. It handles buffer creation,
 * deletion, cleaning, and size determination based on the provided parameters.
 */


#include "SampleBufferHandler.h"

#include <cstdlib>
#include <cstdint>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#define aligned_free(ptr) _aligned_free(ptr)
#else
#define aligned_free(ptr) free(ptr)
#endif // WIN32


#define NS_PER_SEC 1000000000 ///< Defines the number of nanoseconds in one second.
using byte = unsigned char; ///< Defines an alias for unsigned char as 'byte' for better readability.
//uint64_t volatile  samplesBuffer::maxSampleCount = 0; ///< Static constant member of the samplesBuffer struct, initialized to 0. This will be updated later by the SampleBufferHandler.

/**
 * @brief Private implementation details for the SampleBufferHandler class.
 *
 * This class encapsulates the internal data members of the SampleBufferHandler to
 * hide implementation details from the public interface. This is a common C++
 * technique for achieving data hiding and reducing compilation dependencies.
 */
class SampleBufferHandlerPrivate
{
public:
    unsigned int channelCount = 0; ///< The number of channels.
    uint32_t sampleChunk = 0; ///< The number of samples in a single processing chunk.
    uint64_t sampleRate = 0; ///< The sampling rate in samples per second.

    uint64_t timestampsBuffSize = 0; ///< The size of the timestamp buffer in bytes.
    uint64_t sampleBuffSize = 0; ///< The size of the sample buffer in bytes.
    uint32_t maxSampleCount = 0;
};

/**
 * @brief Constructor for the SampleBufferHandler class.
 *
 * Initializes a new SampleBufferHandler object with the specified parameters.
 *
 * @param channelCount The number of channels.
 * @param sampleRate The sampling rate in samples per second.
 * @param sampleChunk The number of samples in a single processing chunk.
 */
SampleBufferHandler::SampleBufferHandler(unsigned int channelCount, unsigned int sampleRate, unsigned int sampleChunk) : m_pImpl(new SampleBufferHandlerPrivate)
{
    m_pImpl->sampleRate = sampleRate;
    m_pImpl->sampleChunk = sampleChunk;    
    m_pImpl->channelCount = channelCount;
    m_pImpl->maxSampleCount = determineSizes();
}

/**
 * @brief Destructor for the SampleBufferHandler class.
 *
 * Releases the memory allocated for the private implementation details.
 */
SampleBufferHandler::~SampleBufferHandler()
{
    delete m_pImpl;
}

/**
 * @brief Creates a new samplesBuffer object.
 *
 * Allocates memory for the sample and timestamps buffers within the samplesBuffer object.
 *
 * @return A pointer to the newly created samplesBuffer object.
 */
samplesBuffer* SampleBufferHandler::createBuffer() const
{
    samplesBuffer* buff = new samplesBuffer;
    buff->maxSampleCount = m_pImpl->maxSampleCount;
    buff->maxStimCount   = MAX_STIM_COUNT;
    buff->stimulationsBuff = new ovStimulation[MAX_STIM_COUNT];
    allocateSubBuffers(buff->samplesBuff, m_pImpl->sampleBuffSize, buff->timestampsBuff, m_pImpl->timestampsBuffSize);
    cleanBuffer(buff);

    return buff;
}

/**
 * @brief Removes a samplesBuffer object.
 *
 * Frees the memory allocated for the sample and timestamp buffers within the
 * provided samplesBuffer object, and then deletes the samplesBuffer object itself.
 *
 * @param buff A pointer to the samplesBuffer object to be removed.
 */
void SampleBufferHandler::removeBuffer(samplesBuffer* buff) const
{
    aligned_free(buff->samplesBuff);
    aligned_free(buff->timestampsBuff);
    delete[] buff->stimulationsBuff;
    delete buff;
}

/**
 * @brief Cleans the contents of a samplesBuffer object.
 *
 * Fills the sample buffer with a neutral signal (zeros) and the timestamp buffer
 * with invalid timestamps (0xFF). Resets the sample count to zero.
 *
 * @param buff A pointer to the samplesBuffer object to be cleaned.
 */
void SampleBufferHandler::cleanBuffer(samplesBuffer* buff) const
{
    memset(buff->samplesBuff, 0,       buff->maxSampleCount * m_pImpl->channelCount * sizeof(sampleType));
    memset(buff->timestampsBuff, 0xFF, buff->maxSampleCount * sizeof(timestampType));
    buff->count     = 0;
    buff->stimCount = 0;
}

/**
 * @brief Determines the required sizes for the sample and timestamp buffers.
 *
 * Calculates the buffer sizes based on the channel count, sample rate, sample chunk size,
 * and a desired buffer duration related to the display refresh rate.
 */
uint32_t SampleBufferHandler::determineSizes()
{
    // ////////////////////////////////////////////////////////////////////////
    // Determine constants from acquisition parameters
    auto timestampsSize = sizeof(uint64_t); ///< Size of a single timestamp in bytes.
    auto timestampsChunkSize = m_pImpl->sampleChunk * timestampsSize; ///< Size of the timestamps for one chunk in bytes.
    auto sampleSize = m_pImpl->channelCount * sizeof(sampleType); ///< Size of a single sample (all channels) in bytes.
    auto samplesChunkSize = sampleSize * m_pImpl->sampleChunk; ///< Size of one chunk of samples in bytes.

    auto periode_ns = UINT64_C(1) * NS_PER_SEC / m_pImpl->sampleRate; ///< Time duration of one sample in nanoseconds.
    auto periodeChunk_ns = periode_ns * m_pImpl->sampleChunk; ///< Time duration of one chunk in nanoseconds.

    // ////////////////////////////////////////////////////////////////////////
    // Determines the number of chunks we want to be able to samplesBuffer
    constexpr auto eyesRefres_ns = NS_PER_SEC / UINT64_C(50); // Eyes refresh rate x2 in ns (assuming a 50 Hz refresh rate, multiplied by 2 for safety margin or double buffering).
    auto chunkCount = eyesRefres_ns / periodeChunk_ns; ///< Calculate the number of chunks needed to cover the desired duration.
    auto chunkCountRest = eyesRefres_ns % periodeChunk_ns; ///< Calculate the remaining time after dividing by the chunk duration.
    if (chunkCount < 3) // Ensure a minimum of 3 chunks for buffering.
    {
        if (chunkCountRest) // If there's a remainder, add one more chunk to cover the full duration.
        {
            chunkCount++;
        }
    }
    else
    {
        chunkCount = periodeChunk_ns * 3; // If enough chunks are already calculated, limit it to a duration equivalent to 3 chunks (this line seems incorrect and should likely be `chunkCount = eyesRefres_ns / periodeChunk_ns;` or similar logic to ensure a minimum buffer size).
    }

    // ////////////////////////////////////////////////////////////////////////
    // Determine buffers sizes
    m_pImpl->sampleBuffSize = chunkCount * samplesChunkSize; ///< Calculate the total size of the sample buffer in bytes.
    m_pImpl->timestampsBuffSize = chunkCount * timestampsChunkSize; ///< Calculate the total size of the timestamp buffer in bytes.
    
    return m_pImpl->sampleChunk * static_cast<uint32_t>(chunkCount); // Update the static maxSampleCount in the samplesBuffer struct. This cast is potentially dangerous and relies on memory layout. A safer approach might be to have a setter method or a non-static member.
}

/**
 * @brief Allocates aligned memory for the sample and timestamp sub-buffers.
 *
 * Allocates memory for the sample and timestamp buffers with a specified alignment
 * (4096 bytes in this case). It also attempts to warm up the cache and memory pages
 * by writing to each page.
 *
 * @param samplesBuff A reference to a pointer that will store the address of the allocated sample buffer.
 * @param sampleBuffSize The desired size of the sample buffer in bytes.
 * @param timestampsBuff A reference to a pointer that will store the address of the allocated timestamp buffer.
 * @param timestampsBuffSize The desired size of the timestamp buffer in bytes.
 */
void SampleBufferHandler::allocateSubBuffers(sampleType*& samplesBuff, uint64_t sampleBuffSize, timestampType*& timestampsBuff, uint64_t timestampsBuffSize) const
{
    // ////////////////////////////////////////////////////////////////////////
    // Memories allocations with alignments
    samplesBuff = reinterpret_cast<sampleType*>(aligned_alloc(4096, sampleBuffSize)); // Allocate aligned memory for the sample buffer.
    timestampsBuff = reinterpret_cast<timestampType*>(aligned_alloc(4096, timestampsBuffSize)); // Allocate aligned memory for the timestamp buffer.

    // ////////////////////////////////////////////////////////////////////////
    // Warm up the cache and memory pages
    int samplesBuffPageCount = static_cast<int>((sampleBuffSize >> 12) + ((sampleBuffSize & 0x0FFF) != 0 ? 1 : 0)); // Calculate the number of 4KB pages in the sample buffer.
    for (int i = 0; i < samplesBuffPageCount; ++i)
    {
        *(uint32_t*)(&(reinterpret_cast<byte*>(samplesBuff))[i << 12]) = 0xFFFFFFFF; // Write to the beginning of each 4KB page to potentially warm up the cache and memory pages.

    }
    int timestampsBuffPageCount = static_cast<int>((timestampsBuffSize >> 12) + ((timestampsBuffSize & 0x0FFF) != 0 ? 1 : 0)); // Calculate the number of 4KB pages in the timestamp buffer.
    for (int i = 0; i < timestampsBuffPageCount; ++i)
    {
        *(uint32_t*)(&(reinterpret_cast<byte*>(timestampsBuff))[i << 12]) = 0xFFFFFFFF; // Write to the beginning of each 4KB page to potentially warm up the cache and memory pages.
    }
}
