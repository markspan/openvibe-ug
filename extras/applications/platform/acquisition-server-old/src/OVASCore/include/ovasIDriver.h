#pragma once

#include "ovas_base.h"
#include "ovasCHeader.h"

namespace OpenViBE {
namespace AcquisitionServer {
class IHeader;

enum class EDriverFlag { IsUnstable, IsDeprecated };

/**
 * \class IDriverContext
 * \author Yann Renard (INRIA/IRISA)
 * \date 2009-10-12
 * \brief Base class for kernel functioanlities access from the driver classes
 * \sa IDriver
 */
class IDriverContext
{
public:
    /**
     * \brief Destructor
     */
    virtual ~IDriverContext() { }

    /**
     * \brief Gets the kernel Log Manager
     * \return the kernel Log Manager
     * \sa Kernel::ILogManager
     */
    virtual Kernel::ILogManager& getLogManager() const = 0;

    /**
     * \brief Gets the kernel Configuration Manager
     * \return the kernel Configuration Manager
     * \sa Kernel::IConfigurationManager
     */
    virtual Kernel::IConfigurationManager& getConfigurationManager() const = 0;

    /**
     * \brief Gets connection status
     * \return \e true if the driver is connected
     * \return \e false if the driver is not connected
     * \sa isStarted
     */
    virtual bool isConnected() const = 0;

    /**
     * \brief Gets acquisition status
     * \return \e true if acquisition is started
     * \return \e false if acquisition is stoped
     * \sa isConnected
     */
    virtual bool isStarted() const = 0;

    /**
     * \brief Checks if impedance check is required (when available)
     * \return \e true if impedance check is required
     * \return \e false if impedance check is not required
     *
     * It is up to the driver to check whether impedance check is required by
     * the acquisition server or not. If this test is required, then the driver
     * should perform impedance measures while initialized but not yet started.
     */
    virtual bool isImpedanceCheckRequested() const = 0;

    /**
     * \brief Gets drift sample count
     * \return \e the drift sample count
     * \sa correctDriftSampleCount
     *
     * This function returns the difference between the theorical
     * number of samples this driver should have sent so far and
     * the number of samples it actually sent. This drift sample
     * count is computed by the acquisition server and can be used
     * to correct a drifting device behavior.
     *
     * \note If this number is less than 0 then samples are missing
     * \note If this number if more than 0 then there are too much samples
     * \note If this number is exactly 0 then the driver sent the exact
     *        number of samples it had to send
     */
    virtual int64_t getDriftSampleCount() const = 0;

    /**
     * \brief Gets the drift sample count tolerance
     * \return \e the drift sample count tolerance
     *
     * Gets the tolerance configured for the acquisition server. This
     * tolerance is present to avoid numerous corrections on a drift
     * value that would oscillate in a small range around 0. In such case,
     * a correction in a way would probably turn in a correction in the
     * opposite way a few seconds later, resulting in crap measurements
     * because of irregular but valid source.
     *
     * If the actual drift is in the [-tolerance +tolerance] range,
     * better don't correct it.
     */
    virtual int64_t getDriftToleranceSampleCount() const = 0;

    /**
     * \brief Gets the suggested drift correction sample count
     * \return \e the suggested drift correction sample count
     *
     * In case you don't want to manage how much samples should
     * be used to correct the drift, you could simply use this function. The
     * algorithm used to compute the returned value should be considered
     * as undefined. This computation could change over time and versions
     * of OpenViBE. That means the driver should not make any assumption
     * on the way of computing the actual value returned by this function.
     */
    virtual int64_t getSuggestedDriftCorrectionSampleCount() const = 0;

    /**
     * \brief Corrects a drifting device
     * \return \e true in case of success
     * \return \e false in case of error
     * \sa getDriftSampleCount
     * \sa setInnerLatencySampleCount
     *
     * Some devices don't sent the number of samples they promised to
     * while requesting sampling rate. This can be for multiple reasons
     * but the most probable one is that the device does not have its
     * internal clock synchronized with the internal computer clock.
     * OpenViBE data being dated with a start / end time, it is important
     * for synchronisation purpose that all the theorical number of
     * samples per second is preserved.
     *
     * In case the difference between the theorical number of samples
     * this driver should have sent so far and the number of samples
     * it actually sent becomes too big, this function helps to
     * correct the drifting, removing or adding dummy samples.
     * In a strict signal processing point of view those samples
     * can't be considered as valid. However, this is the only way
     * to guarantee that the timings are preserved.
     *
     * \note Passing a negative value removes samples
     * \note Passing a positive value adds samples
     * \note Passing 0 does nothing
     * \note This function can be called several times if needed
     *       but does not change what \ref getDriftSampleCount
     *       returns until the next \ref IDriver::loop execution
     * \warning Please be careful when calling this function.
     *          Consider the fact that there could be some drifting
     *          in the device and ths OS-level driver itself, or even
     *          in the communication pipeline to the prorprietary
     *          acquisition software. As an OpenViBE driver developer,
     *          you should allow an arbitrary small drifting (which
     *          range depends of the actual driver you are creating).
     */
    virtual bool correctDriftSampleCount(const int64_t nSample) = 0;

    /**
     * \brief Sets a fixed latency due to the driver implementation and / or hardware
     * \param[in] nSample : the number of samples for the inner latency
     *
     * Some drivers / hardware devices implement processes that delay the
     * overall acquisition stream. Such processes include for instance
     * sample buffering or temporal filtering. The delay induced by these
     * processes can not be known by the acquisition server in a generic way
     * and are usually not provided by the device manufacturer. An intelligent
     * setup based on hardware tagging could however induce a fair estimate of this
     * latency so that it can be injected in the drift correction process.
     * Driver developers will generally not have to care about this feature.
     * However, if it is important to have neurophysiologically relevant measures
     * (more particularly on ERPs) and it is know that the driver and / or hardware
     * has a latency, this latency compensation can be injected in the drift correction
     * process using the aforementionned function.
     *
     * \note Passing a positive value supposes the driver / hardware induce a delay which shall be corrected
     * \note Passing a negative value supposes the driver / hardware is able to predict and send future measures before they actually get measured (probably not recommended)
     * \note Passing 0 supposes both the driver and hardware are ideals and send samples immediately to the acquisition server as they happen on the scalp
     * \note Default configuration supposes both the driver and hardware are ideal, thus sets the inner latency to 0
     *
     * \sa getInnerLatencySampleCount
     */
    virtual void setInnerLatencySampleCount(const int64_t nSample) = 0;

    /**
     * \brief Gets the fixed latency due to the driver implementation and / or hardware
     * \return \e The inner latency of the driver in sample count
     * \sa setInnerLatencySampleCount
     */
    virtual int64_t getInnerLatencySampleCount() const = 0;

    /**
     * \brief Updates impedance for a specific channel
     * \param index [in] : the index of the channel should be updated
     * \param impedance [in] : the impedance of the specified channel (in ohm)
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * This can be used during initialization phase to check
     * if impedances are correct. This function should be connected
     * while the driver is initialized and not started.
     *
     * If \c impedance is -1, then the impedance is considered as "unknown / measuring"
     * If \c impedance is -2, then the impedance is considered as "unknown / not available"
     * If \c impedance is any other negative value, then the impedance is considered as "unknown"
     */
    virtual bool updateImpedance(const size_t index, const double impedance) = 0;
};

/**
 * \class IDriverCallback
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-04-01
 * \brief Base class for all the OpenViBE acquisition server driver callbacks
 *
 * Objects derived from this class are called by any driver to provide
 * built sample buffers.
 *
 * \sa IDriver
 * \sa IDriver::loop
 */
class IDriverCallback
{
public:
    /**
     * \brief Gives new sample buffer
     * \param samples [in] : a buffer containing all the samples
     *
     * This is used by the acquisition server to be notified when the
     * driver has finished to build the whole buffer of data to send.
     * This function is called by the driver during the \e IDriver::loop
     * and should give an array of \c nSamplesPerChannel x \c nChannel
     * organised by channel first.
     * 
     * The caller retains the ownership of the samples pointer.
     *
     * \code
     * samples[0] is channel 0 sample 0
     * samples[1] is channel 0 sample 1
     * ...
     * samples[nSamplesPerChannel-1] is channel 0 sample nSamplesPerChannel-1
     * samples[nSamplesPerChannel  ] is channel 1 sample 0
     * samples[nSamplesPerChannel+1] is channel 1 sample 1
     * ...
     * samples[i*nSamplesPerChannel+j] is channel i sample j
     * \endcode
     *
     * \sa IDriver::loop
     */
    virtual void setSamples(const float* samples) = 0;

    /**
     * \brief Gives new sample buffer
     * \param samples [in] : a buffer containing all the samples
     * \param size : size of buffer
     *
     * This is used by the acquisition server to be notified when the
     * driver has finished to build the whole buffer of data to send.
     * This function is called by the driver during the \e IDriver::loop
     * and should give an array of \c nSamplesPerChannel x \c nChannel
     * organised by channel first.
     * 
     * The caller retains the ownership of the samples pointer.
     *
     * \code
     * samples[0] is channel 0 sample 0
     * samples[1] is channel 0 sample 1
     * ...
     * samples[nSamplesPerChannel-1] is channel 0 sample nSamplesPerChannel-1
     * samples[nSamplesPerChannel  ] is channel 1 sample 0
     * samples[nSamplesPerChannel+1] is channel 1 sample 1
     * ...
     * samples[i*nSamplesPerChannel+j] is channel i sample j
     * \endcode
     *
     * \sa IDriver::loop
     */
    virtual void setSamples(const float* samples, const size_t size) = 0;

    /**
     * \brief Gives a new stimulation set corresponding to the last sample buffer
     * \param stimSet [in] : the stimulation set associated with the last sample buffer
     *
     * This is used by the acquisition server to be notified when the
     * driver has finished to build the whole buffer of data to send.
     * This function is called by the driver during the \e IDriver::loop
     * and immediatly after the IDriverCallback::setSamples function, even
     * if the stimulation set is empty.
     *
     * \warning The stimulation dates are relative to the
     *          last buffer start time.
     *
     * \sa IDriver::loop
     */
    virtual void setStimulationSet(const CStimulationSet& stimSet) = 0;

    /**
     * \brief Destructor
     */
    virtual ~IDriverCallback() { }
};

/**
 * \class IDriver
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-04-01
 * \brief Base class for all the OpenViBE acquisition server drivers
 *
 * This class should be used by hardware driver developers in order
 * to add new peripherals to the OpenViBE acquisition server. This driver
 * is then used by the server to get cerebral activity measurments and to
 * send this measured activity to the platform so it can be processed.
 *
 * The behavior of the driver is splitted into 3 phases :
 *  - configuration
 *  - initialization / uninitialization
 *  - acquisition start / stop
 *
 * The implemented driver could either have direct access to the hardware
 * or read the measurements from a proprietarry server, depending on the
 * hardware manufacturer choice. Thus, the configuration phase of this driver
 * should be able to provide the information OpenViBE needs that are
 * not present from the hardware manufacturer'.
 *
 * Important job for the driver is to give the acquisition server fixed
 * size measured buffers... This implies that if the hardware sends random
 * size buffers, sample per sample buffers, fixed length buffer but not the
 * size the acquisition server requested, it would be the driver's job to
 * rebuild correct buffers.
 *
 * \sa CAcquisitionServer
 */
class IDriver
{
public:
    /**
     * \brief Constructor
     * \param ctx [in] : the driver context
     */
    explicit IDriver(IDriverContext& ctx) : m_driverCtx(ctx) { }

    /**
     * \brief Destructor
     */
    virtual ~IDriver() { }

    /** \name General purpose functions */
    //@{

    /**
     * \brief Gets the driver name (usually the hardware name)
     * \return the driver name.
     */
    virtual const char* getName() = 0;
    /**
     * \brief Tests if a flag is set for this driver
     * \param flag [in] : the flag to test
     * \return \e true if the tested flag is set
     * \return \e false if the tested flag is not set
     * \note Default implementation always returns false, meaning that no flag is set
     */
    virtual bool isFlagSet(const EDriverFlag flag) const { return false; }

    //@}
    /** \name Driver configuration */
    //@{

    /**
     * \brief This function should tell whether the dirver is configurable or not
     * \return \e true if this driver is configurable.
     * \return \e false if this driver is not configurable.
     * \sa configure
     */
    [[deprecated("Any driver is configurable with minimum driver parameters.")]]
    virtual bool isConfigurable() { return true; };

    /**
     * \brief This function is called by the acquisition server when the
     * configuration settings have been validated.
     * \return \e true if the configuration ended succesfully
     * \return \e false if the configuration failed
     *
     * Drivers do not need to load their own configuration UI anymore (see \e IDriver::parameters), and this method is now
     * called by the Acquisition Server when the configuration settings have been validated.
     * This is the place where the driver can store the configuration
     * or any device specific action that could be needed.
     */
    virtual bool configure() { return true; };

    /**
     * \brief Gets the driver specific parameters
     * \return the driver parameters
     *
     * Returns the parameters of the driver.
     * This is called by the acquisition server in order to display the
     * parameters in the UI.
     *
     * The developer should therefore use OpenViBE::Parameters as members of
     * its driver class and ensure to store them in the m_parameters vector.
    */
    OpenViBE::Parameters::Parameters parameters() { return m_parameters; }

    //@}
    /** \name Initialization / uninitialization */
    //@{

    /**
     * \brief Initializes the driver
     * \param nSamplePerSentBlock [in] : the number of samples to
     *        send per channel per driver send operation.
     * \param callback [in] : the callback object to call when the buffer
     *        are filled correctly.
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * When called, this function should prepare the driver to receive
     * data from the hardware. This means doing everything from checking
     * the hardware is present, checking its configuration, getting header
     * information from it and so on. This function should return only when
     * it has a valid header to return to the acquisition server... For
     * some hardware, this could nead data sending request (so it gets basic
     * information), and then request data sending to stop...
     *
     * \sa uninitialize
     * \sa getHeader
     * \sa IHeader
     */
    virtual bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) = 0;
    /**
     * \brief Uninitializes the driver
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * This function disconnects any link to the hardware and frees any
     * allocated structures/objects related to the hardware. When this
     * is called, the driver may be re-configured and re-initialized
     * to start a new acquisition session.
     */
    virtual bool uninitialize() = 0;
    /**
     * \brief Gets the header information for the session
     * \return the header information for the session
     *
     * The returned header is used by the acquisition server to send
     * encapsulated data to the platform at the beginning of the sending
     * process. This header should be filled by the information collected
     * from the hardware or the driver itself when configure is called.
     *
     * \sa IHeader
     */
    virtual IHeader* const getHeader() { return &m_header; };

    //@}
    /** \name Starting / Stopping acquisition */
    //@{

    /**
     * \brief Starts acquisition for this driver
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * This function is called by the acquistion server to notify the driver
     * that signal acquisiton should start... The driver should forward this
     * instruction to the hardware... The loop function is then called
     * repeatedly to receive data from the hardware and send it back to the
     * server.
     *
     * \sa loop
     * \sa stop
     */
    virtual bool start() = 0;
    /**
     * \brief Stops acquisition for this driver
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * This function is called by the acquistion server to notify the driver
     * that signal acquisiton should stop... The driver should forward this
     * instruction to the hardware... The loop function is then no more called
     * and the driver may be uninitialized to be re-configured.
     *
     * \sa start
     * \sa loop
     */
    virtual bool stop() = 0;
    /**
     * \brief Requests data acquisition to be done
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * This function is called repeatedly by the acquisition server when
     * the driver is correctly initialized and started. The driver's job
     * here is to receive measures from the hardware, build the buffers
     * according to the requested sampling count per channel (see \c initialize)
     * and send this built buffer to the provided callback object. The
     * server will then send these informations to the platform.
     *
     * During this loop function, the object passed as IDriverCallback at
     * the initialize phase should be ready to be notified of samples or
     * stimulations.
     *
     * \warning When implementing the driver, one should take care
     *         of configuring stimulation dates so that they are relative
     *         to the last buffer start time.
     *
     * \sa IDriverCallback::setSamples
     * \sa IDriverCallback::setStimulationSet
     */
    virtual bool loop() = 0;

    //@}

protected:
    IDriverContext& m_driverCtx; ///< The driver context
    CHeader m_header; ///< The header information for the driver
    OpenViBE::Parameters::Parameters m_parameters;

private:
    /**
     * \brief Default constructor can not be used because a context is needed
     */
    IDriver();
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
