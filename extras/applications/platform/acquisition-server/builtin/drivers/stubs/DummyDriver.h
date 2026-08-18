#pragma once
/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file DummyDriver.h
 * @brief Header file for DummyDriver class.
 *
 * Declares the DummyDriver class, a stub implementation of AbstractDriver for testing.
 */

#include "AbstractDriver.h"
#include <AbstractDriverBuilder.h>
#include <iostream>
#include <chrono>


/**
 * @brief Stub Implementation of AbstractDriver for tests.
 * @details This class simulates a driver for testing purposes.
 */
class DummyDriver : public AbstractDriver
{
public:
    DummyDriver();    
    ~DummyDriver() override;

    bool init() override;
    bool unInit() override;
    bool start() override;
    bool stop() override;

    int read(sampleType* data, uint64_t size) override;

    void setChunkSize(uint32_t chunkSize) override;

private:
    uint32_t m_chunkSize; ///< Chunk size.
    bool m_bRun; ///< Flag to indicate if the driver is running.
    std::chrono::high_resolution_clock::time_point m_lastSentTime; ///< Time of the last sent data.
    std::chrono::milliseconds m_stepDuration;
};

REGISTER_DRIVER(DummyDriver, "Dummy Driver", false);
