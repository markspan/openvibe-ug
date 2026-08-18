/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 * @file AbstractDriver.cpp
 * @brief Implementation file for the AbstractDriver class.
 *
 * This file implements the non-pure virtual methods of the AbstractDriver
 * base class, providing default functionality for timestamp retrieval and
 * a basic implementation for getting data into a samplesBuffer.
 */

#include <AbstractDriver.h>
#include <Acquisition.h>

#include <cstdint>
#include <chrono>
#include <any>

AbstractDriver::AbstractDriver()
{
    m_channelCount.connect([this](std::any val) {
        int newCount = std::any_cast<int>(val);
        m_channelNames.resize(newCount);
    });
}

 /**
  * @brief Retrieves a monotonic timestamp, in nanoseconds.
  *
  * Returns a duration in nanoseconds since an unspecified reference point,
  * using `std::chrono::steady_clock`. Only meaningful as a difference between
  * two calls to this function (elapsed time) - not a wall-clock/calendar time.
  *
  * @return The current timestamp as a timestampType (nanoseconds, monotonic).
  */
timestampType AbstractDriver::timestamp()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

/**
 * @brief Retrieves data from the driver and populates a samplesBuffer.
 *
 * This method reads data from the underlying hardware driver using the pure
 * virtual `read()` method and stores it in the provided `samplesBuffer`. It also
 * records the timestamp of the last read sample. Note that the current implementation
 * reads only 1 byte of data, which might not be the intended behavior for a typical driver.
 *
 * @param buff A pointer to the samplesBuffer where the retrieved data will be stored.
 */
void AbstractDriver::getData(samplesBuffer* buff)
{
    buff->count = read(buff->samplesBuff, buff->maxSampleCount);
    uint64_t readTime = timestamp();
    if (buff->count > 0)
    {
        buff->timestampsBuff[buff->count - 1] = readTime;
    }
    buff->stimCount = readStimulations(buff->stimulationsBuff, buff->maxStimCount);
    if (buff->stimCount > 0)
    {
        // Mostly 1 at a time, read is called very rapidly.
        // If there are more than 1, assume they are at the same time.
        for (int i = 0; i < buff->stimCount; i++)
        {
            if (buff->stimulationsBuff[i].date == 0)
                buff->stimulationsBuff[i].date = readTime;
        }
    }
}
