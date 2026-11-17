/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * @file AbstractDriver.h
 * @brief Header file for the AbstractDriver class.
 *
 * This file declares the `AbstractDriver` class, which serves as an abstract base
 * class for hardware drivers. It defines a common interface for driver initialization,
 * data reading, and timestamps retrieval.
 */
#pragma once

#include "samplesBuffer.h"
#include "openvibe/parameters/parameters.h"
#include "openvibe/parameters/parameterStringVector.h"

#include <OVAcquisitionAbstractions_Export.h>

#include <functional>  // UG fork
#include <memory>
#include <mutex>  // UG fork
#include <string>  // UG fork
#include <vector>  // UG fork

 /**
  * @brief Abstract base class for hardware drivers.
  *
  * The `AbstractDriver` class defines a common interface that concrete hardware
  * driver implementations must adhere to. It provides pure virtual methods for
  * initialization and data reading, ensuring that derived classes implement these
  * essential functionalities. It also provides a default implementation for timestamps
  * retrieval and a virtual method for getting processed data.
  */
class OVACQUISITIONABSTRACTIONS_EXPORT AbstractDriver
{
public:
    /**
     * @brief Default constructor for the AbstractDriver class.
     *
     * Initializes an AbstractDriver object. As it's an abstract class, this
     * constructor is typically called by derived classes.
     */
    AbstractDriver();

    /**
     * @brief Virtual destructor for the AbstractDriver class.
     *
     * Ensures proper cleanup of derived class objects when deleted through
     * a base class pointer. This is crucial for polymorphism.
     */
    virtual ~AbstractDriver() = default;

    /**
     * @brief Initializes the hardware driver.
     *
     * This is a pure virtual method that must be implemented by derived classes
     * to perform the necessary initialization steps for the specific hardware.
     *
     * @return True if the initialization was successful, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Uninitializes the hardware driver.
     */
    virtual bool unInit() = 0;

    /**
     * @brief  Start the hardware driver acquisition.
     * @return True if the acquisition started successfully, false otherwise.
     */
    virtual bool start() = 0;

    /**
     * @brief  Stop the hardware driver acquisition.
     * @return True if the acquisition stopped successfully, false otherwise.
     */
    virtual bool stop() = 0;

    /**
     * @brief Retrieves the parameters of the driver.
     * @return A vector of parameters.
     */
    virtual OpenViBE::Parameters::Parameters getParameters() { return m_parameters; }

    /**
     * @brief Retrieves the current timestamp.
     *
     * Returns the current timestamps value. The specific implementation of how
     * the timestamps is obtained is left to derived classes.
     *
     * @return The current timestamps as a timestampType.
     */
    timestampType timestamp();

    /**
     * @brief Reads data from the hardware.
     *
     * This is a pure virtual method that must be implemented by derived classes
     * to read data from the associated hardware.
     *
     * @par Buffer layout
     * @parblock
     * @p data is <b>sample-major (interleaved)</b>: all channels of one sample sit next to each
     * other, and samples follow one another. With @c n = channelCount():
     *
     * @code
     * data[0]     is sample 0, channel 0
     * data[1]     is sample 0, channel 1
     * ...
     * data[n-1]   is sample 0, channel n-1
     * data[n]     is sample 1, channel 0
     * data[n+1]   is sample 1, channel 1
     * ...
     * data[i*n+j] is sample i, channel j
     * @endcode
     *
     * So a driver filling @p size samples writes @c data[i * channelCount() + j], and the base of
     * sample @c i is @c data + i * channelCount().
     *
     * This allows acquisition-server to output data without copying or transposing, for best efficiency.
     * @endparblock
     *
     * @param data A pointer to the buffer where the read data will be stored, laid out as above.
     * @param size The maximum number of samples to write (not the number of floats).
     * @return The number of samples actually read, or an error code (typically negative).
     * @note getData() assigns this return into an unsigned count, so returning a negative value
     *       is currently unsafe; report 0 samples on failure instead.
     */
    virtual int read(sampleType* data, uint64_t size) = 0;

    /**
     * @brief Reads stimulations that occurred since the last call.
     *
     * Override in drivers that produce stimulations. The default implementation
     * produces no stimulations. Must not exceed buff capacity (maxStimCount).
     *
     * @param buff  Pre-allocated array to fill.
     * @param maxCount Capacity of buff.
     * @return Number of stimulations written, or a negative error code.
     */
    virtual int readStimulations(ovStimulation* /*buff*/, uint32_t /*maxCount*/) { return 0; }

    /** Returns true if this driver supports impedance measurement. */
    virtual bool supportsImpedanceCheck() const { return false; }

    /**
     * Returns true if Charmeleon, when installed, should check impedances for this driver's
     * amplifiers instead of startImpedanceCheck() and friends.
     */
    virtual bool usesCharmeleonForImpedance() const { return false; }  // UG fork

    /**
     * @brief Command-line arguments that make Charmeleon open the amplifier this driver uses, e.g.
     *        {"--amplifier", "tmsi", "--protocol", "USB", "--device", "..."}; empty to let it choose.
     *
     * UG fork: upstream has no Charmeleon.
     */
    virtual std::vector<std::string> charmeleonArguments() const { return {}; }
    /** Path of the Charmeleon montage the driver uses, which Charmeleon then opens; empty for none. */
    virtual std::string charmeleonMontage() const { return {}; }  // UG fork
    /**
     * @brief The driver's Montage setting as the user gave it, empty for none; the acquisition server sends the
     *        montage's name and electrode positions to its clients with the channel names (ovChannelInfo.h).
     *
     * UG fork: upstream has no montages.
     */
    virtual std::string montageSetting() const { return {}; }
    /**
     * @brief Takes over the montage Charmeleon saved, e.g. after editing it: the driver reads it again, and uses
     *        @p path from now on when it is another file than its own (Charmeleon's Save As).
     *
     * UG fork: upstream has no Charmeleon.
     */
    virtual void takeOverCharmeleonMontage(const std::string& /*path*/) {}

    /**
     * @brief How full the amplifier's own sample buffer got, and how many samples it lost, as far as the driver can
     *        tell.
     *
     * UG fork: upstream does not show it. The amplifier's buffer is the last margin before samples are lost: it fills
     * whenever the server reads more slowly than the amplifier sends, so the server shows it during an acquisition.
     */
    struct DeviceBuffer
    {
        bool reported          = false;  ///< False when the driver can tell nothing about its buffer
        double peakSeconds     = 0;      ///< Most data waiting in the buffer since the previous takeDeviceBuffer(), in seconds
        double capacitySeconds = 0;      ///< How much data the buffer holds, in seconds; 0 when unknown
        bool lossCounted       = false;  ///< True when lostSamples is counted; false when the driver cannot tell
        uint64_t lostSamples   = 0;      ///< Samples lost since the acquisition started
    };

    /**
     * @brief The buffer state since the previous call, which this resets for the next one.
     *
     * UG fork. Called on the user interface's thread while the server's reader thread runs the driver, so a driver
     * keeps what this returns in atomics. The default reports nothing.
     */
    virtual DeviceBuffer takeDeviceBuffer() { return {}; }

    /** Enter impedance measurement mode. Called after init(), before start(). */
    virtual bool startImpedanceCheck() { return false; }

    /**
     * @brief Read current impedance values for all channels.
     *
     * Called periodically while in impedance check mode. Writes one value per
     * channel (in ohms) into @p values. @p count equals channelCount().
     * @return Number of values written, or a negative error code.
     */
    virtual int readImpedances(double* /*values*/, size_t /*count*/) { return 0; }

    /** Exit impedance measurement mode. Called before start(). */
    virtual bool stopImpedanceCheck() { return false; }

    /**
     * @brief Retrieves processed data into a samplesBuffer.
     *
     * This virtual method is intended to be implemented by derived classes to
     * populate a provided samplesBuffer with data read from the hardware.
     *
     * @param buff A pointer to the samplesBuffer where the retrieved data will be stored.
     */
    virtual void getData(samplesBuffer* buff);

    /**
     * @brief Retrieves the sampling rate of the hardware.
     * @return The sampling rate in Hz.
     */
    virtual OpenViBE::Parameters::ParameterInt& samplingRate() { return m_samplingRate; };

    /**
     * @brief Retrieves the number of channels of the hardware.
     * @return The number of channels.
     */
    virtual OpenViBE::Parameters::ParameterInt& channelCount() { return m_channelCount; };

    /**
     * @brief Retrieves the channel names of the hardware.
     * @return The channel names as a ParameterStringVector.
     */
    virtual OpenViBE::Parameters::ParameterStringVector& channelNames() { return m_channelNames; }

    /**
     * @brief Hints the channel names list shows in entries left empty, e.g. which device channel an entry stands for.
     * @return One hint per channel name (an empty one for none), or an empty vector for no hints at all.
     */
    virtual std::vector<std::string> channelHints() const { return {}; }  // UG fork

    /// A message for the user about the driver's settings or its last attempt to start, shown under the settings.
    /// UG fork: upstream drivers can only print to the console.
    struct Note
    {
        enum class Level { Info, Warning, Error };

        Level level = Level::Info;
        std::string text;  ///< Empty for no note

        bool operator==(const Note& other) const { return level == other.level && text == other.text; }
        bool operator!=(const Note& other) const { return !(*this == other); }
    };

    /// The driver's current note; its text is empty when there is nothing to say.
    Note note() const;

    /**
     * @brief The channel names entries the driver decides itself, e.g. from a montage.
     * @return One flag per channel name, or an empty vector when the user decides them all.
     */
    std::vector<bool> channelLocks() const;

    /**
     * @brief Sets the function called when the note or the channel locks change.
     *
     * It may be called on any thread, e.g. the acquisition's while a driver starts.
     */
    void setStateListener(std::function<void()> listener);

    /**
     * @brief Sets the chunk size for the driver.
     * @param chunkSize The chunk size to set in samples.
     */
    virtual void setChunkSize(uint32_t chunkSize) = 0;

	/**
	 * @brief Sets whether the driver is an auxiliary driver.
	 * @param aux True if the driver is auxiliary, false otherwise.
	 */
	virtual void setAuxiliaryStatus(bool aux) { m_bAuxiliaryDriver = aux; }

	/**
	 * @brief Checks if the driver is an auxiliary driver.
	 * @return True if the driver is auxiliary, false otherwise.
	 */
    bool isAuxiliaryDriver() const { return m_bAuxiliaryDriver; }

protected:
    /// Sets the note, see note(); an empty @p text clears it.
    /// UG fork: this, clearNote() and setChannelLocks().
    void setNote(Note::Level level, const std::string& text);
    void clearNote() { setNote(Note::Level::Info, {}); }

    /// Sets the channel locks, see channelLocks().
    void setChannelLocks(std::vector<bool> locks);

    /**
     * @brief The sampling rate for the driver.
     */
    OpenViBE::Parameters::ParameterInt m_samplingRate{"SamplingRate", "Sampling rate", 256, 1, 10000};

    /**
     * @brief The number of channels for the driver.
     */
    OpenViBE::Parameters::ParameterInt m_channelCount{"ChannelCount", "Number of channels", 4, 1, 1000};

    /**
     * @brief The channel names for the driver, kept in sync with m_channelCount.
     */
    OpenViBE::Parameters::ParameterStringVector m_channelNames;

    /**
     * @brief The list to hold the parameters of the driver.
     * This list must be filled by each drier.
     */
    OpenViBE::Parameters::Parameters m_parameters;

	/**
	 * @brief Flag indicating whether the driver is an auxiliary driver.
	 */
	bool m_bAuxiliaryDriver = false;

private:
    // UG fork: the note, the channel locks and who hears of their changes
    void notifyStateChanged();

    mutable std::mutex m_stateMutex;  ///< Guards the note, the locks and the listener
    Note m_note;
    std::vector<bool> m_channelLocks;
    std::function<void()> m_stateListener;
};

class AbstractDriverBuilder; ///< Forward declaration; the complete type is only needed by the deleter's implementation.

/**
 * @brief Deleter that hands a driver back to the builder that created it.
 *
 * A driver coming from a plugin is allocated inside that plugin; destroying it from the
 * host would pair the host's operator delete with the plugin's operator new. Routing the
 * destruction back through AbstractDriverBuilder::destroyDriver() keeps allocation and
 * deallocation on the same side of the library boundary. Falls back to a plain delete
 * when no builder is known.
 *
 * @note operator() is deliberately out of line: it lives in the abstraction library, so
 *       host and plugins all go through the same copy.
 */
struct OVACQUISITIONABSTRACTIONS_EXPORT DriverDeleter
{
	AbstractDriverBuilder const* builder = nullptr; ///< Builder the driver came from, or nullptr.

	void operator()(AbstractDriver* driver) const;
};

/// Owning pointer to a driver, released through the builder that created it.
using DriverPtr = std::unique_ptr<AbstractDriver, DriverDeleter>;
