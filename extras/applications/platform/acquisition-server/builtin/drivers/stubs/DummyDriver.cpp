/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file DummyDriver.cpp
 * @brief Implementation file for the DummyDriver class.
 *
 * Implements the DummyDriver class, a "stub" implementation of AbstractDriver.
 */

#include "DummyDriver.h" // Includes the DummyDriver class header

#include <cmath>

/**
 * @brief Default constructor for DummyDriver.
 *
 * Prints a message indicating that the dummy driver is created.
 */
DummyDriver::DummyDriver()
{
    std::cout << "DummyDriver: Constructor called." << std::endl;
    m_channelCount.setValue(4); ///< Number of channels.
    m_samplingRate.setValue(256); ///< Sample rate.
    m_chunkSize   = 32; ///< Chunk size.
    m_bRun        = false; ///< Flag to indicate if the driver is running.
    m_stepDuration = std::chrono::milliseconds(static_cast<uint64_t>(static_cast<double>(m_chunkSize) / m_samplingRate * 1000));
    m_lastSentTime = std::chrono::high_resolution_clock::now() - m_stepDuration;

    m_parameters.push_back(&m_samplingRate);
    m_parameters.push_back(&m_channelCount);
}

/**
 * @brief Destructor for DummyDriver.
 *
 * Prints a message indicating that the dummy driver is destroyed.
 */
DummyDriver::~DummyDriver()
{
    std::cout << "DummyDriver: Destructor called." << std::endl;
}

/**
 * @brief Initializes the dummy driver.
 *
 * This method simulates driver initialization. For this stub, it always returns true.
 * @return true, indicating successful initialization.
 */
bool DummyDriver::init()
{
    std::cout << "DummyDriver: Initialization successful." << std::endl;
    return true;
}

/**
 * @brief Uninitializes the dummy driver.
 *
 * This method simulates driver uninitialization. For this stub, it always returns true.
 * @return true, indicating successful uninitialization.
 */
bool DummyDriver::unInit()
{
    std::cout << "DummyDriver: Uninitialization successful." << std::endl;
    return true;
}

/**
 * @brief Starts the dummy driver.
 *
 * This method simulates starting the driver. For this stub, it sets the running flag to true.
 * @return true, indicating that the driver has started successfully.
 */
bool DummyDriver::start()
{
    m_bRun = true; ///< Set the running flag to true.
    return m_bRun;
}

/**
 * @brief Stops the dummy driver.
 *
 * This method simulates stopping the driver. For this stub, it sets the running flag to false.
 * @return true, indicating that the driver has stopped successfully.
 */
bool DummyDriver::stop()
{
    m_bRun = false; ///< Set the running flag to false.
    return !m_bRun;
}

/**
 * @brief Reads dummy data.
 *
 * This method simulates reading data from the driver.
 * It fills the data buffer with a simple pattern of integers.
 * The pattern alternates between positive and negative values.
 * It's a triangular wave pattern.
 */
int DummyDriver::read(sampleType* data, uint64_t size)
{
    int sizeRes = 0; ///< Initialize the size result to zero.

    static uint64_t sampleIndex = 0;
    const double amplitude = 10.0;
    const double twoPi = 2.0 * M_PI;

    if (m_bRun)
    {
        std::chrono::high_resolution_clock::time_point curTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> timeDiff = curTime - m_lastSentTime;
        if (timeDiff.count() >= m_stepDuration.count())
        {
            for (int i = 0; i < size; ++i)
            {
                double t = static_cast<double>(sampleIndex + i) / m_samplingRate;
                for (int j = 0; j < m_channelCount; ++j)
                {
                    double frequency = static_cast<double>(j + 1); // ch0=1Hz, ch1=2Hz, ...
                    data[i * m_channelCount + j] = static_cast<sampleType>(amplitude * std::sin(twoPi * frequency * t));
                }
            }
            sampleIndex += size;
            sizeRes = size;
            m_lastSentTime = curTime;
        }
    }

    return sizeRes;
}

/**
 * @brief Sets the chunk size.
 * @param chunkSize The chunk size to set.
 */
void DummyDriver::setChunkSize(uint32_t chunkSize)
{
    m_chunkSize = chunkSize; // Store the chunk size
    std::cout << "DummyDriver: setChunkSize called with " << chunkSize << std::endl;
}
