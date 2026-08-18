#pragma once
/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file samplesBuffer.h
 * @brief Header file defining the structure for storing samples and timestamps.
 *
 * This file defines the `samplesBuffer` structure, which is used to hold a buffer
 * of sample data and their corresponding timestamps. It also defines related
 * type aliases and constants.
 */


#include <openvibe/acquisition-messaging/samples.h>

/**
 * @brief Structure for holding a buffer of samples and their timestamps.
 *
 * This structure contains pointers to arrays of timestamps and samples, as well as
 * a counter for the number of valid samples currently in the buffer and a static
 * constant for the maximum number of samples the buffer can hold.
 */
typedef struct samplesBuffer
{
    timestampType*  timestampsBuff;   ///< Buffer of timestamps for each sample.
    sampleType*     samplesBuff;      ///< Buffer of sample data.
    ovStimulation*  stimulationsBuff; ///< Buffer of stimulation data
    uint64_t        count;            ///< Number of valid samples in this buffer.
    uint32_t        stimCount;        ///< Number of valid stimulations in this buffer.
    uint32_t        maxStimCount;     ///< Capacity of stimulationsBuff (in stimulations).
    uint64_t        maxSampleCount;   ///< Capacity of samplesBuff (in samples).
}samplesBuffer;
