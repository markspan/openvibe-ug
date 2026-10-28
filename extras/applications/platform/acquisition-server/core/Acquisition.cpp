/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file AquisitionServer.cpp
 * @brief Implementation file for the AquisitionServer class.
 *
 * This file implements the methods of the `Acquisition` class, which manages
 * the acquisition of data from a hardware driver, buffers it, and makes it
 * available for reading. It includes functionality for data processing,
 * timestamp interpolation, and drift calculation.
 */

#include "Acquisition.h"
#include "DriftCalculator.h" // for DriftCalculator

#include <chrono> // for high_resolution_clock
#include <iostream>

using Clock = std::chrono::steady_clock; ///< Monotonic clock for interval measurements.


/**
 * @brief Constructor for the Acquisition class.
 *
 * Initializes the Acquisition object by setting the running flag to false,
 * the last timestamp to 0, and the window size to 0.
 */
Acquisition::Acquisition() : m_bRun(false),
                             m_lastTimestamps(0), 
                             m_windowSize(0),
                             m_nbCycle(0),
                             m_chunkSampleCount("Sample count per sent block",
                                                "Number of samples required to send a block",
                                                {4, 8, 16, 32, 64, 128, 256, 512},
                                                32),
                             m_drift({0, 0, 0, 0})
{ }

/**
 * @brief Processes a buffer of acquired data.
 *
 * This method pops a buffer from the circular buffer's processing queue,
 * interpolates any missing timestamps, updates the last processed timestamp,
 * and feeds the timestamps to the drift calculator.
 */
void Acquisition::processBuffer()
{
    auto buff = m_circularBuffer->popBufferToProcess(); ///< Get a buffer to process from the circular buffer.
    if (buff)
    {
        if (buff->count > 0) {
            interpolateTimestamps(buff, m_lastTimestamps); ///< Fill in any invalid timestamps in the buffer.
            m_lastTimestamps = buff->timestampsBuff[buff->count - 1]; ///< Update the timestamp of the last processed sample.

            if (!m_bFirstbufferProcessed)
            {
                // buff->timestampsBuff[0] here is interpolated between m_lastTimestamps (captured in
                // start(), before the reader thread/driver clock even exists) and the one real
                // timestamp in this buffer (its last sample). That bakes a thread-launch-latency-sized,
                // session-varying bias into the drift anchor. Derive the anchor instead from the real
                // timestamp and the nominal period, so it doesn't depend on that interpolation at all.
                uint64_t const periodNs = 1'000'000'000ULL / uint64_t(uint32_t(m_driver->samplingRate()));
                uint64_t const anchor   = buff->timestampsBuff[buff->count - 1] - buff->count * periodNs;
                m_driftCalculator->setFirstTimeStamp(anchor); ///< Set the initial timestamp for drift calculation.
                m_bFirstbufferProcessed = true; ///< Mark that the first buffer has been processed.
            }

            m_driftCalculator->fillNewTimestamps(std::vector<uint64_t>(buff->timestampsBuff, buff->timestampsBuff + buff->count)); //To be optimized: Copying the vector might be inefficient for large buffers. Consider passing iterators or the raw pointer and count.
        }

        for (auto& writer : m_writers)
        {
            writer->writeSamples(buff);
        }

        m_circularBuffer->pushBufferToClean(buff);
    }
}

/**
 * @brief Destructor for the Acquisition class.
 *
 * Calls the `clear()` method to ensure proper cleanup of resources when the
 * Acquisition object is destroyed.
 */
Acquisition::~Acquisition()
{
    uninitAcquisition();
}

/**
 * @brief Sets the driver for data acquisition.
 * 
 * This method takes ownership of the provided driver, which is later released through the
 * builder that created it (see DriverDeleter) rather than with a plain delete.
 * @param driver Owning pointer to the driver instance to be used for data acquisition.
 */
void Acquisition::addDriver(DriverPtr driver, const std::string& name)
{
    if (driver) {
        if (driver->isAuxiliaryDriver()) {
            m_auxiliaryDrivers.push_back(std::move(driver));
        } else {
            m_driver     = std::move(driver);
            m_driverName = name;
        }
    }
}

/**
 * @brief Removes the current driver.
 */
void Acquisition::removeDriver(AbstractDriver* driver)
{
	if (m_driver.get() == driver) {
		m_driver.reset();
		m_driverName.clear();
	}
	else {
		auto it = std::find_if(m_auxiliaryDrivers.begin(), m_auxiliaryDrivers.end(), [driver](const auto& d) { return d.get() == driver; });
		if (it != m_auxiliaryDrivers.end()) {
			m_auxiliaryDrivers.erase(it);
		}
	}
}

/**
 * @brief provides a vector of all drivers used in the acquisition, including the main driver and any auxiliary drivers.
 * @return A vector of pointers to all drivers used in the acquisition.
 */
std::vector<AbstractDriver*> Acquisition::getDrivers() const
{
    std::vector<AbstractDriver*> driversVectorRes;

    if (m_driver) {
        driversVectorRes.push_back(m_driver.get());
    }
    for (const auto& auxDriver : m_auxiliaryDrivers) {
        driversVectorRes.push_back(auxDriver.get());
    }

    return driversVectorRes;
}

/**
 * @brief Append a writer instance to the list of writers.
 */
void Acquisition::addWriter(AbstractWriter* writer) {
    if (writer) {
        writer->setClientProvider([this]() { return m_clientsList; });
        m_writers.emplace_back(writer);
    }
}

/**
 * @brief Removes a writer instance from the list of writers.
 */
void Acquisition::removeWriter(AbstractWriter* writer) {
    m_writers.erase(std::remove_if(m_writers.begin(), m_writers.end(), [writer](const auto& w) { return w.get() == writer; }), m_writers.end());
}

/**
 * @brief Provides all writers instances used by this acquisition 
 */
std::vector<AbstractWriter*> Acquisition::getWriters() const {
    std::vector<AbstractWriter*> result;
    for (auto& w : m_writers) {
        result.push_back(w.get());
    }
    return result;
}

/**
 * @brief Creates and configures the acquisition process.
 *
 * Initializes the sample buffer handler, circular buffer, and drift calculator
 * based on the provided parameters. It also clears any existing configuration.
 *
 * @param channelCount The number of channels to acquire.
 * @param sampleRate The sampling rate of the acquired data in samples per second.
 * @param sampleChunk The number of samples to process in each chunk.
 */
void Acquisition::initAcquisition()
{
    uninitAcquisition(); ///< Stop any running acquisition and uninitAcquisition existing resources.
    m_driver->setChunkSize(m_chunkSampleCount.value());
    if (m_driver->init()) ///< Initialize the driver for data acquisition.
    {
        unsigned int sampleRate = m_driver->samplingRate();
        unsigned int channelCount = m_driver->channelCount();
        
        computeWindowSize(sampleRate);

        m_sampleBufferHandler = std::unique_ptr<SampleBufferHandler>(new SampleBufferHandler(channelCount, sampleRate, m_chunkSampleCount.value())); ///< Create a new sample buffer handler with the given parameters.
        m_circularBuffer      = std::unique_ptr<CircularBuffer>(new CircularBuffer(m_sampleBufferHandler.get())); ///< Create a new circular buffer, using the sample buffer handler for buffer allocation.
        m_driftCalculator     = std::unique_ptr<DriftCalculator>(new DriftCalculator(sampleRate, m_windowSize)); ///< Create a new drift calculator with the given sample rate and window size.

        for (auto& w : m_writers) {
            w->init(m_chunkSampleCount.value(), channelCount);
            w->onAcquisitionSetup(sampleRate, m_driver->channelNames().value());
        }
    }
}

/**
 * @brief Checks if the driver supports impedance measurement.
 * @return True if the driver supports impedance measurement, false otherwise.
 */
bool Acquisition::supportsImpedanceCheck() const
{
    return m_driver && m_driver->supportsImpedanceCheck();
}

/**
 * @brief Checks if the driver hands impedance checking to Charmeleon when it is installed.
 * @return True if Charmeleon should check impedances for this driver, false otherwise.
 */
bool Acquisition::usesCharmeleonForImpedance() const
{
    return m_driver && m_driver->usesCharmeleonForImpedance();
}

/**
 * @brief Starts the impedance measurement process.
 * @return True if the impedance measurement process was started successfully, false otherwise.
 */
bool Acquisition::startImpedanceCheck()
{
    bool res = false;
    if (m_state == State::Idle && m_driver) {
        m_driver->setChunkSize(m_chunkSampleCount.value());
        if (m_driver->init()) {
            if (m_driver->startImpedanceCheck()) {
                m_state = State::ImpedanceCheck;
                res = true;
            } else {
                m_driver->unInit();
            }
        }
    }
    return res;
}

/**
 * @brief Stops the impedance measurement process.
 * @return True if the impedance measurement process was stopped successfully, false otherwise.
 */
bool Acquisition::stopImpedanceCheck()
{
    bool res = false;
    if (m_state == State::ImpedanceCheck) {
        m_driver->stopImpedanceCheck();
        m_driver->unInit();
        m_state = State::Idle;
        res = true;
    }
    return res;
}

/**
 * @brief Reads the impedance values from the driver.
 * @param values The array to store the impedance values.
 * @param count The number of impedance values to read.
 * @return The number of impedance values read, or a negative value if an error occurred.
 */
int Acquisition::readImpedances(double* values, size_t count)
{
    int res = 0;
    if (m_state == State::ImpedanceCheck && m_driver) {
        res = m_driver->readImpedances(values, count);
    }
    return res;
}


void Acquisition::computeWindowSize(unsigned int sampleRate)
{
    m_windowSize = sampleRate;
    //TODO: Implement a more sophisticated window size calculation based on the sample rate and other parameters.
    // This is a placeholder implementation. The actual logic should be defined based on the requirements.
    // For example, you might want to set the window size based on the desired latency or processing time.
    // m_windowSize = ...; // Calculate the window size based on the sample rate and other factors.
    // Ensure that the window size is a reasonable value (e.g., not too small or too large).
    // use m_nbCycle to determine the window size
}

/**
 * @brief Starts the data acquisition process.
 *
 * Sets the running flag to true, records the acquisition start time, and creates
 * threads for reading data, processing buffers, cleaning buffers, and calculating drift.
 * This method ensures that the necessary components are initialized before starting the threads.
 */
void Acquisition::start()
{
    if (m_state == State::ImpedanceCheck) {
        // Driver already initialized; exit impedance mode without full reinit, then set up streaming buffers.
        m_driver->stopImpedanceCheck();
    } else if (m_state != State::Streaming) {
        // Idle → Streaming: full driver init + buffer setup.
        initAcquisition();

        if (m_sampleBufferHandler.get()) {  // Ensure that the acquisition has been properly created.
            m_bRun = true;
            m_bFirstbufferProcessed = false;
            m_state = State::Streaming;

            m_circularBuffer->setRunning(m_bRun);
            m_lastTimestamps = immediateTimestamp();
            m_driftCalculator->setStartTimeAcq(m_lastTimestamps); // Record the start time of the acquisition for drift calculation.
            m_driftCalculator->setStopTimeAcq(0); // Reset the stop time.

            m_threadReader  = std::unique_ptr<thread>(new thread([this] { acquisitionProcessTreadFunction(); })); // Create a thread that continuously reads data from the driver and writes it to the circular buffer.
            m_threadProcess = std::unique_ptr<thread>(new thread([this] { processBufferThreadFunction(); })); // Create a thread that continuously processes buffers from the circular buffer.
            m_threadClean   = std::unique_ptr<thread>(new thread([this] { cleanBuffersThreadFunction(); }));  // Create a thread that continuously cleans up buffers in the circular buffer.
            m_threadDrift   = std::unique_ptr<thread>(new thread([this] { driftProcessThreadFunction(); }));  // Create a thread that continuously runs the drift calculation.
        }
    }    
}


/**
 * @brief A placeholder function for the acquisition process.
 *
 * This function is intended to be run in a separate thread. It continuously
 * reads data from the driver and writes it to the processing queue.
 */
void Acquisition::acquisitionProcessTreadFunction()
{
    samplesBuffer* buff = m_circularBuffer->popFreeBuffer(); ///< Get a free buffer from the circular buffer.

    m_driver->start(); ///< Start the driver for data acquisition.
    for (auto& auxDriver : m_auxiliaryDrivers) {
        auxDriver->start();
    }

    while (this->m_bRun) {
        this->read(buff);
    }

    return;
}

/**
 * @brief Thread function for processing drift calculations.
 *
 * This function runs in a separate thread and continuously calculates the drift
 * values by calling the `driftProcess()` method.
 */
void Acquisition::driftProcessThreadFunction()
{
    while (this->m_bRun)
    {
        driftProcess();
    }
}

/**
 * @brief Thread function for cleaning buffers.
 *
 * This function runs in a separate thread and continuously cleans the buffers
 * in the circular buffer by calling the `cleanBuffers()` method.
 */
void Acquisition::cleanBuffersThreadFunction()
{
    while (this->m_bRun)
    {
        m_circularBuffer->cleanBuffers();
    }
}

/**
 * @brief Thread function for processing buffers.
 *
 * This function runs in a separate thread and continuously processes buffers
 * by calling the `processBuffer()` method.
 */
void Acquisition::processBufferThreadFunction()
{
    while (this->m_bRun)
    {
        processBuffer();
    }
}

/**
 * @brief Calculates and prints the drift values.
 *
 * This function runs in a separate thread and continuously calculates the drift
 * values. It prints the calculated drift values every second.
 */
void Acquisition::driftProcess()
{
    static Clock::time_point prevTime = Clock::now(); ///< Record the start time of the drift calculation.
    Clock::time_point curTime = Clock::now(); ///< Record the current time for this drift calculation.
    
    m_driftCalculator->run(); 
    if (curTime.time_since_epoch() - prevTime.time_since_epoch() > std::chrono::milliseconds(1000)) ///< Check if the time since the last drift calculation exceeds 100 milliseconds.
    {
        std::lock_guard<std::mutex> lock(m_driftMutex);
        m_driftCalculator->readDrifts(m_drift);
        prevTime = curTime; ///< Update the previous time to the current time.
    }
}

/**
 * @brief Stops the data acquisition process.
 *
 * Records the acquisition stop time, sets the running flag to false, and waits
 * for all acquisition threads to finish their execution.
 */
void Acquisition::stop()
{
    if (m_bRun)
    {
        m_driftCalculator->setStopTimeAcq(immediateTimestamp());
        m_driver->stop();
        m_bRun = false;
        m_circularBuffer->setRunning(m_bRun);
        m_threadReader->join();
        m_threadProcess->join();
        m_threadClean->join();
        m_threadDrift->join();
        m_driver->unInit();
        m_state = State::Idle;
        m_sampleBufferHandler = nullptr;
        m_circularBuffer      = nullptr;
        m_driftCalculator     = nullptr;
        std::lock_guard<std::mutex> lock(m_driftMutex);
        m_drift = {0, 0, 0, 0};
    }
    m_threadReader  = nullptr;
    m_threadProcess = nullptr;
    m_threadClean   = nullptr;
    m_threadDrift   = nullptr;
}

/**
 * @brief Clears any buffered data and resets the acquisition state.
 *
 * Stops the acquisition process and releases the unique pointers to the
 * sample buffer handler, circular buffer, and drift calculator.
 */
void Acquisition::uninitAcquisition()
{
    stop();
    for (auto& w : m_writers) {
        w->uninit();
    }
}

/**
* @brief Indicates if the acquisition has a primary driver or not.
* @return true if a primary driver is available, false otherwise.
*/
bool Acquisition::hasPrimaryDriver() const
{
    return m_driver.get() != nullptr;
}

/**
 * @brief Writes a buffer of acquired data to the processing queue.
 *
 * Checks if the buffer contains any data. If it does, it attempts to fix any
 * invalid last timestamps and pushes the buffer to the circular buffer's
 * processing queue. If the buffer is empty, it's pushed to the clean queue.
 * Finally, it tries to get a new free buffer if the acquisition is running.
 *
 * @param buff A reference to a pointer to the `samplesBuffer` to be written.
 */
void Acquisition::write(samplesBuffer*& buff)
{
    if (buff->count > 0 || buff->stimCount > 0) ///< Check if the buffer has any valid samples.
    {
        if (buff->count > 0) {
            canFixInvalidLastTimestamps(buff->timestampsBuff[buff->count - 1]); ///< Attempt to fix the last timestamp if it's invalid.
        }
        m_circularBuffer->pushBufferToProcess(buff); ///< Push the buffer to the queue for processing.
    }
    else
    {
        m_circularBuffer->pushBufferToClean(buff); ///< If the buffer is empty, push it to the clean queue.
    }

    buff = m_bRun ? m_circularBuffer->popFreeBuffer() : nullptr; ///< Get a new free buffer from the circular buffer if the acquisition is running.
}

/**
 * @brief Reads data from the driver and writes it to the processing queue.
 *
 * Calls the `getData()` method of the assigned `AbstractDriver` to read data
 * into the provided buffer and then calls the `write()` method to enqueue it
 * for processing.
 *
 * @param buff A reference to a pointer to the `samplesBuffer` to read data into.
 */
void Acquisition::read(samplesBuffer*& buff)
{
    m_driver->getData(buff); ///< Get data from the hardware driver.
    write(buff); ///< Write the received data to the processing queue.

    for (auto& auxDriver : m_auxiliaryDrivers) {
        if (!buff) break; ///< write() may null-out buff when acquisition stops so we need to check for that
        auxDriver->getData(buff); ///< Get data from the auxiliary hardware drivers.
        write(buff); ///< Write the received data to the processing queue.
    }
}

/**
 * @brief Sets the number of cycles for the acquisition drift window.
 *
 * This method sets the number of cycles to be used in the drift window calculation.
 * It ensures that the number of cycles is within a specified range (50 to 100).
 *
 * @param cycles The number of cycles to set.
 */
void Acquisition::setNbCycles(uint32_t cycles)
{
    if (cycles < 50)
    {
        cycles = 50;
    }

    if (cycles > 100)
    {
        cycles = 100;
    }

    m_nbCycle = cycles;
}

void Acquisition::addClient(uint64_t clientId)
{
    // Prepare ovMsg with acquisition config and send to client before adding it to the list and stream starts going
    for (auto& w : m_writers) {
        w->writeAcquisitionConfig(m_driver->channelCount(), m_driver->samplingRate(), m_chunkSampleCount.value(), clientId);
    }
    m_clientsList.push_back(clientId);
}

// ////////////////////////////////////////////////////////////////////////////
// Static helpers

/**
 * @brief Interpolates timestamps within a buffer.
 *
 * Finds ranges of invalid timestamps (marked by `TIMESTAMP_INVALID`) within the
 * provided buffer and interpolates the timestamps based on the valid timestamps
 * before and after the invalid range.
 *
 * @param buff The `samplesBuffer` to interpolate timestamps in.
 * @param lastTimestamps The timestamp of the last valid sample before this buffer.
 */
void Acquisition::interpolateTimestamps(samplesBuffer* buff, uint64_t lastTimestamps)
{
    std::list<std::pair<int, int>> emptyRanges; ///< List to store the start and end indexes of empty timestamp ranges.

    bool bInEmptyRange = false; ///< Flag to indicate if the current index is within an empty timestamp range.
    int lastValidTimestampsIndex = -1; ///< Index of the last encountered valid timestamp.

    // ////////////////////////////////////////////////////////////////////////
    // Find empty ranges
    for (uint32_t i = 0; i < buff->count; ++i)
    {
        if (buff->timestampsBuff[i] == TIMESTAMP_INVALID) ///< Check if the current timestamp is invalid.
        {
            bInEmptyRange = true; ///< Mark that we are now in an empty range.
        }
        else
        {
            if (bInEmptyRange) ///< If we were in an empty range and now encountered a valid timestamp.
            {
                emptyRanges.push_back({ lastValidTimestampsIndex, i }); ///< Store the start and end indices of the empty range.
                bInEmptyRange = false; ///< Reset the flag as the empty range has ended.
            }
            lastValidTimestampsIndex = i; ///< Update the index of the last valid timestamp.
        }
    }

    // ////////////////////////////////////////////////////////////////////////
    // Interpolate and fill empty ranges
    for (auto& emptyRange : emptyRanges) ///< Iterate through all identified empty timestamp ranges.
    {
        // interpolate timestamps
        uint64_t beginTimestamps = emptyRange.first == -1 ? lastTimestamps : buff->timestampsBuff[emptyRange.first]; ///< Get the timestamp at the beginning of the range (either the last valid timestamp from the previous buffer or the timestamp before the invalid range).
        uint64_t endTimestamps = buff->timestampsBuff[emptyRange.second]; ///< Get the timestamp at the end of the invalid range.

        auto emptyTimestampsCount = (emptyRange.second - emptyRange.first) - 1; ///< Calculate the number of invalid timestamps in the range.
        auto timeIncrement = (endTimestamps - beginTimestamps) / (emptyTimestampsCount + 1); ///< Calculate the time increment between consecutive timestamps in the range.

        // fill timestamps
        for (int i = 1; i < emptyTimestampsCount + 1; ++i) ///< Iterate through the indices within the empty range.
        {
            buff->timestampsBuff[emptyRange.first + i] = (i * timeIncrement) + beginTimestamps; ///< Calculate and set the interpolated timestamp.
        }
    }
}

/**
 * @brief Attempts to fix an invalid last timestamp.
 *
 * Checks if the provided timestamp is marked as invalid (`TIMESTAMP_INVALID`) and,
 * if so, replaces it with the current immediate timestamp.
 *
 * @param timeStamps A reference to the timestamp value to check and potentially fix.
 */
void Acquisition::canFixInvalidLastTimestamps(uint64_t& timeStamps)
{
    if (timeStamps == TIMESTAMP_INVALID) ///< Check if the timestamp is invalid.
    {
        timeStamps = immediateTimestamp(); ///< Replace the invalid timestamp with the current immediate timestamp.
    }
}

/**
 * @brief Gets the current immediate timestamp using a monotonic clock.
 *
 * Returns a duration in nanoseconds since an unspecified reference point,
 * using steady_clock. Only meaningful as a difference between two
 * calls to this function (elapsed time) - not a wall-clock/calendar time.
 *
 * @return The current timestamp as a uint64_t (nanoseconds, monotonic).
 */
uint64_t Acquisition::immediateTimestamp()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}

drift_st Acquisition::drift() {
    std::lock_guard<std::mutex> lock(m_driftMutex);
    return m_drift;
}
