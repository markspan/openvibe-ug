#pragma once
/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file SampleBufferHandler.h
 * @brief Header file for the SampleBufferHandler class.
 *
 * This file declares the SampleBufferHandler class, which is responsible for managing
 * buffers used to store samples and their timestamps. It provides methods for
 * creating, removing, and cleaning these buffers, as well as determining their sizes
 * based on the processing parameters.
 */

#include "samplesBuffer.h"

class SampleBufferHandlerPrivate; ///< Forward declaration of the private implementation class.

/**
 * @brief Manages buffers for storing samples and their timestamps.
 *
 * The SampleBufferHandler class encapsulates the logic for creating, destroying,
 * and manipulating buffers that hold sample data and corresponding timestamps.
 * It determines the necessary buffer sizes based on the provided configuration
 * (channel count, sample rate, and sample chunk size) and aims to optimize buffer
 * management for real-time processing.
 */
class SampleBufferHandler
{
public:
    static constexpr uint32_t MAX_STIM_COUNT = 64; ///< Hard cap on stimulations per buffer.

    SampleBufferHandler(unsigned int channelCount, unsigned int sampleRate, unsigned int sampleChunk);
    ~SampleBufferHandler();

    samplesBuffer* createBuffer() const;
    void removeBuffer(samplesBuffer* buff) const;
    void cleanBuffer(samplesBuffer* buff) const;

private:
    uint32_t determineSizes();
    void allocateSubBuffers(sampleType*& samplesBuff, uint64_t sampleBuffSize, timestampType*& timestampsBuff, uint64_t timestampsBuffSize) const;

private:
    using byte = unsigned char; ///< Alias for unsigned char.
    SampleBufferHandlerPrivate* m_pImpl; ///< Pointer to the private implementation details.
};

