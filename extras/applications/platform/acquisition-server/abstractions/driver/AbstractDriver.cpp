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
#include <AbstractDriverBuilder.h> ///< For the complete AbstractDriverBuilder type used by DriverDeleter.

#include <cstdint>
#include <chrono>
#include <any>
#include <utility>

/**
 * @brief Releases a driver through the builder that created it.
 *
 * Kept out of line, and in this library rather than in a header, so that a driver
 * allocated inside a plugin is also freed inside that plugin.
 *
 * @param driver The driver instance to release.
 */
void DriverDeleter::operator()(AbstractDriver* driver) const
{
    if (builder) { builder->destroyDriver(driver); }
    else { delete driver; }
}

// UG fork: the note and channel locks, which upstream drivers do not have; see AbstractDriver.h

AbstractDriver::Note AbstractDriver::note() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_note;
}

std::vector<bool> AbstractDriver::channelLocks() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_channelLocks;
}

void AbstractDriver::setStateListener(std::function<void()> listener)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_stateListener = std::move(listener);
}

void AbstractDriver::setNote(const Note::Level level, const std::string& text)
{
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        const Note note{text.empty() ? Note::Level::Info : level, text};
        if (note == m_note) { return; }
        m_note = note;
    }
    notifyStateChanged();
}

void AbstractDriver::setChannelLocks(std::vector<bool> locks)
{
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (locks == m_channelLocks) { return; }
        m_channelLocks = std::move(locks);
    }
    notifyStateChanged();
}

void AbstractDriver::notifyStateChanged()
{
    std::function<void()> listener;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        listener = m_stateListener;
    }
    // Outside the lock, so that the listener can read the state back
    if (listener) { listener(); }
}

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
