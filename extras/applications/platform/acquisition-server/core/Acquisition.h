#pragma once
/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file Acquisition.h
 * @brief Header file for the Acquisition class.
 *
 * This file declares the `Acquisition` class, which is responsible for managing
 * the acquisition of data from a hardware driver, buffering it, and making it
 * available for reading. It handles driver management, data processing, and
 * potentially writing the acquired data.
 */

#include "AbstractDriver.h"
#include "samplesBuffer.h"
#include "CircularBuffer.h"
#include "AbstractWriter.h"

#include <openvibe/parameters/parameters.h>
#include <openvibe/parameters/parameterInt.h>
#include <openvibe/parameters/parameterListString.h>

#include "Thread.h" // Assuming this is a custom thread class
#include <thread>
#include <mutex>
#include <memory>


#include <cstdint>
#include <list>
#include <optional>

class DriftCalculator; ///< Forward declaration of the DriftCalculator class.

/**
 * @brief Manages the acquisition of data from a hardware driver.
 *
 * The `Acquisition` class orchestrates the process of acquiring data from a
 * hardware driver (represented by an `AbstractDriver` instance), storing it in a
 * circular buffer, and providing a mechanism to read the acquired data. It also
 * handles tasks like data processing, timestamp interpolation, and drift calculation.
 */
class Acquisition
{
public:
    enum class State { Idle, ImpedanceCheck, Streaming };

    Acquisition();
    ~Acquisition();

	// Driver management
    void addDriver(DriverPtr driver, const std::string& name = ""); ///< Sets the driver for data acquisition, taking ownership of it.
	void removeDriver(AbstractDriver* driver); ///< Removes the current driver.
	std::vector<AbstractDriver*> getDrivers() const; ///< Provides all driver instances used by this acquisition.

	// Writer management
    void addWriter(AbstractWriter* writer);
    void removeWriter(AbstractWriter* writer);
    std::vector<AbstractWriter*> getWriters() const;

	// Client management
    void addClient(uint64_t clientId);
    void removeClient(uint64_t clientId) { m_clientsList.remove(clientId); } ///< Removes a client from the clients list.
    std::list<uint64_t> getClients() const { return m_clientsList; } ///< Returns the asked clients list.

    // Ownership
    /// Sets which client may control this acquisition. An unset owner leaves it open to every client.
    void setOwner(std::optional<uint64_t> owner) { m_owner = owner; }
    /// True when no client owns this acquisition, so any of them may control it.
    bool isShared() const { return !m_owner.has_value(); }
    /// True when this acquisition belongs to the given client.
    bool isOwnedBy(const uint64_t clientId) const { return m_owner.has_value() && *m_owner == clientId; }
    /// True when the given client may change or drive this acquisition.
    bool isControllableBy(const uint64_t clientId) const { return isShared() || isOwnedBy(clientId); }

	// Acquisition control
    void initAcquisition(); ///< Initializes the acquisition process with the specified parameters.
    void uninitAcquisition(); ///< Uninitializes the acquisition process.

	// Data acquisition and processing
    void start();
    void driftProcessThreadFunction();
    void cleanBuffersThreadFunction();
    void processBufferThreadFunction();
    void driftProcess();
    void stop();

	// State management
    bool isRunning() const { return m_bRun; } ///< Checks if the acquisition process is currently running.
    State state() const { return m_state; }

	// Impedance check block
    bool supportsImpedanceCheck() const;
    bool usesCharmeleonForImpedance() const;  ///< UG fork: see AbstractDriver::usesCharmeleonForImpedance().
    bool startImpedanceCheck();
    bool stopImpedanceCheck();
    int  readImpedances(double* values, size_t count);

    void read(samplesBuffer*& buff);

    void setNbCycles(uint32_t cycles);

    OpenViBE::Parameters::ParameterListInt* getChunkSampleCount() { return &m_chunkSampleCount; }
    OpenViBE::Parameters::Parameters getDriverParameters() const { return !m_driver ? OpenViBE::Parameters::Parameters{} : m_driver->getParameters(); }

    /// Name of the primary driver, as registered with the AcquisitionManager. Empty when unset.
    const std::string& getDriverName() const { return m_driverName; }

    std::vector<std::string> getDriverChannelNames() const { return !m_driver ? std::vector<std::string>{} : m_driver->channelNames().value(); }
    void setDriverChannelNames(const std::vector<std::string>& names) { if (m_driver) m_driver->channelNames().setValue(names); }
    // UG fork: channel hints and Charmeleon, which upstream does not have; see the AbstractDriver methods of the same names
    std::vector<std::string> getDriverChannelHints() const { return !m_driver ? std::vector<std::string>{} : m_driver->channelHints(); }
    std::string getCharmeleonMontage() const { return !m_driver ? std::string{} : m_driver->charmeleonMontage(); }
    /// The arguments that make Charmeleon open the driver's amplifier; see AbstractDriver::charmeleonArguments().
    std::vector<std::string> getCharmeleonArguments() const { return !m_driver ? std::vector<std::string>{} : m_driver->charmeleonArguments(); }
    /// Hands the montage Charmeleon saved to the driver; see AbstractDriver::takeOverCharmeleonMontage().
    void takeOverCharmeleonMontage(const std::string& path) { if (m_driver) { m_driver->takeOverCharmeleonMontage(path); } }
    // UG fork: what the driver has to tell the user, and the channel names it decides itself, see AbstractDriver::note()
    AbstractDriver::Note getDriverNote() const { return !m_driver ? AbstractDriver::Note{} : m_driver->note(); }
    std::vector<bool> getDriverChannelLocks() const { return !m_driver ? std::vector<bool>{} : m_driver->channelLocks(); }
    /// Sets the function the driver calls when its note or channel locks change; see AbstractDriver::setStateListener().
    void setDriverStateListener(std::function<void()> listener) { if (m_driver) { m_driver->setStateListener(std::move(listener)); } }

    OpenViBE::Parameters::ParameterStringVector* getDriverChannelNamesParam() { return m_driver ? &m_driver->channelNames() : nullptr; }

    bool hasPrimaryDriver() const; ///< Checks if a primary driver is set.

    drift_st drift();

    /**
     * @brief How full the buffers between the amplifier and the clients got.
     *
     * UG fork: upstream shows the drift instead. Samples are lost only when the amplifier's own buffer overflows,
     * which happens when the server's buffers have all been taken for longer than that buffer lasts; both are shown.
     */
    struct BufferStatus
    {
        AbstractDriver::DeviceBuffer device;  ///< The amplifier's buffer, as the driver reports it
        uint32_t serverBuffers        = 0;    ///< Buffers in the server's pool; 0 while not acquiring
        uint32_t minFreeServerBuffers = 0;    ///< Fewest of them free since the previous call
    };
    /// The buffer status since the previous call, which this resets for the next one. Safe to call from any thread.
    BufferStatus takeBufferStatus();

protected:
    void write(samplesBuffer*& buff);

private:
    Acquisition(Acquisition const&) = delete;
    Acquisition& operator=(Acquisition const&) = delete;

    void acquisitionProcessTreadFunction();
    void processBuffer();

    static void interpolateTimestamps(samplesBuffer* buff, uint64_t lastTimestamps);
    static void canFixInvalidLastTimestamps(uint64_t& timeStamps);
    static uint64_t immediateTimestamp();

    void computeWindowSize(unsigned int sampleRate);

private:
    unsigned int m_windowSize; ///< Size of a processing window (likely in samples).
    uint64_t m_lastTimestamps; ///< Timestamp of the last processed sample.

    OpenViBE::Parameters::ParameterListInt m_chunkSampleCount; ///< Size of output chunks (in samples)

    uint32_t m_nbCycle;
    std::mutex m_driftMutex;
    drift_st m_drift;

    bool m_bRun; ///< Flag indicating whether the acquisition process is running.
    bool m_bFirstbufferProcessed; ///< Flag indicating whether the first buffer has been processed.
    State m_state = State::Idle;
    std::unique_ptr<thread> m_threadReader; ///< Unique pointer to the thread responsible for reading data from the driver.
    std::unique_ptr<thread> m_threadProcess; ///< Unique pointer to the thread responsible for processing the acquired data.
    std::unique_ptr<thread> m_threadClean; ///< Unique pointer to a thread for potential cleanup tasks.
    std::unique_ptr<thread> m_threadDrift; ///< Unique pointer to a thread for drift calculation.

    std::unique_ptr<SampleBufferHandler> m_sampleBufferHandler; ///< Unique pointer to the handler for managing sample buffers.
    std::unique_ptr<CircularBuffer>      m_circularBuffer; ///< Unique pointer to the circular buffer for storing acquired data.
    std::mutex m_circularBufferMutex; ///< UG fork: held while m_circularBuffer is replaced, and by takeBufferStatus()
    std::unique_ptr<DriftCalculator>     m_driftCalculator; ///< Unique pointer to the object responsible for calculating drift.

    DriverPtr m_driver; ///< Owning pointer to the abstract driver instance, released through its builder.
    std::string m_driverName; ///< Registered name of the primary driver, which the driver itself does not carry.
    std::vector<DriverPtr> m_auxiliaryDrivers; ///< Owning pointers to the auxiliary abstract drivers.
    std::vector<std::unique_ptr<AbstractWriter>> m_writers; ///< Vector of unique pointers to abstract writers.

    std::list<uint64_t> m_clientsList; ///< List of client identifiers.
    ovChannelInfo m_channelInfo; ///< UG fork: the channel names and montage sent to each client, set by initAcquisition()
    std::optional<uint64_t> m_owner; ///< Client allowed to control this acquisition, unset when shared.
};
