#pragma once

#include "ovas_base.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class IHeader
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-04-01
 * \brief Base class for an OpenViBE header container
 *
 * IHeader objects are used by IDriver objects to give all the header
 * information to the acquisition server. The IDriver developer may
 * implement his own IHeader derived class or use the one provided
 * with the acquisition server. To get a standard header, refer to
 * the \c CHeader class.
 *
 * The IHeader objects mainly consist in get/set/isSet functions
 * that allow user code to modify, read back and check state of some
 * single header information.
 *
 * \sa IDriver
 * \sa IDriver::getHeader
 * \sa CHeader
 */
class IHeader
{
public:
    /** \name General purpose functions */
    //@{

    /**
     * \brief Resets this header
     *
     * When called, this function resets all the header content to its
     * default values. Most of the is*Set will return \e false after
     * this call.
     */
    virtual void reset() = 0;

    //@}
    /** \name Experiment information */
    //@{

    /**
     * \brief Sets the experiment identifier
     * \param experimentID [in] : the experiment identifier to send to
     *        the OpenViBE platform.
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * The experiment identifier may be used by the platform
     * to get details from a database, for example a description
     * of the experiment, what is done, where etc...
     */
    virtual bool setExperimentID(const size_t experimentID) = 0;
    /**
     * \brief Sets the subject age
     * \param subjectAge [in] : the subject age in years
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool setSubjectAge(const size_t subjectAge) = 0;
    /**
     * \brief Sets the subject gender
     * \param subjectGender [in] : the subject gender
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * The subject gender is given as an integer and should
     * be ISO 5218 conformant... Allowed values are :
     *  - 0 : unknown
     *  - 1 : male
     *  - 2 : female
     *  - 3 : not specified
     *
     * \note This values are defined in the OpenViBE toolkit.
     */
    virtual bool setSubjectGender(const size_t subjectGender) = 0;
    /**
     * \brief Gets the experiment identifier
     * \return the experiement identifier.
     * \sa setExperimentID
     */
    virtual size_t getExperimentID() const = 0;
    /**
     * \brief Gets the subject age
     * \return the subject age.
     * \sa setSubjectAge
     */
    virtual size_t getSubjectAge() const = 0;
    /**
     * \brief Gets the subject gender
     * \return the subject gender.
     * \sa setSubjectGender
     */
    virtual size_t getSubjectGender() const = 0;
    /**
     * \brief Tests if experiment identifier has been set
     * \return \e true if experiment identifier has been set since last \c reset.
     * \return \e false if experiment identifier has not been set.
     * \sa setExperimentID
     */
    virtual bool isExperimentIDSet() const = 0;
    /**
     * \brief Tests if subject age has been set
     * \return \e true if the subject age has been set since last \c reset.
     * \return \e false if the subject age has not been set.
     * \sa setSubjectAge
     */
    virtual bool isSubjectAgeSet() const = 0;
    /**
     * \brief Tests if subject gender has been set
     * \return \e true if the subject gender has been set since last \c reset.
     * \return \e false if the subject gender has not been set.
     * \sa setSubjectGender
     */
    virtual bool isSubjectGenderSet() const = 0;

    virtual void setImpedanceCheckRequested(const bool active) = 0;

    virtual bool isImpedanceCheckRequested() const = 0;

    /**
    * \brief Get impedance limit
    * \return \e the chosen impedance limit (ohms)
    */
    virtual size_t getImpedanceLimit() const = 0;
    /**
    * \brief Set impedance limit
     * \param limit [in] : the new value for impedance limit (ohms)
    */
    virtual void setImpedanceLimit(const size_t limit) = 0;

    //@}
    /** \name Chanel information */
    //@{

    /**
     * \brief Sets channel count for the recorded signal
     * \param nChannel [in] : the number of the channel for the recorder signal
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * The number of channels will be used by the IDriver and the acquisition server
     * to calculate the sample buffer size (that is \c nSamplesPerChannel x \c nChannel).
     */
    virtual bool setChannelCount(const size_t nChannel) = 0;
    /**
     * \brief Sets a channel' name
     * \param index [in] : the index of the channel which name should be set
     * \param name [in] : the new name for this channel
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note As soon as a channel name is set, all the yet-unset channel names are
     *       considered to be set to empty string.
     */
    virtual bool setChannelName(const size_t index, const char* name) = 0;
    /**
     * \brief Sets a channel' gain
     * \param index [in] : the index of the channel which gain should be set
     * \param gain [in] : the gain value for this channel
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note As soon as a channel gain is set, all the yet-unset channel gains are
     *       considered to be set to 1.
     *
     * Gains are multiplicator coefficients that are used by the OpenViBE platform to
     * to transform measured values into physical dimension.
     */
    virtual bool setChannelGain(const size_t index, const float gain) = 0;
    /**
     * \brief Sets a channel' measurement unit and its scaling factor
     * \param index [in] : the index of the channel which gain should be set
     * \param unit [in] : the unit
     * \param factor [in] : the scaling factor
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * Measurement units (e.g. Volts, Litres, ...) are specified by size_t enums defined in the openvibe toolkit.
     * Scaling factors (megas, millis, ...) are specified similarly. To specify that the channel is in millivolts, 
     * you set unit to volts and factor to millis. You get the list of supported enums from toolkit/ovtk_defines.h. 
     *
     * Default unit is 'Unspecified' and default factor is code translating 1e00. 
     */
    virtual bool setChannelUnits(const size_t index, const size_t unit, const size_t factor) = 0;

    /// \todo setChannelLocation
    // virtual bool setChannelLocation(const size_t index, const float channelLocationX, const float channelLocationY, const float channelLocationZ)=0;

    /**
     * \brief Gets the number of channels for this header
     * \return the number of channels.
     * \sa setChannelCount
     */
    virtual size_t getChannelCount() const = 0;
    /**
     * \brief Gets the name of a channel
     * \param index [in] : the index of the channel which name is wanted
     * \return the name of the \c index th channel if in the correct range
     *         and name has been specified.
     * \return an empty string if in the correct range but name has not been specified.
     * \return an empty string when \c index is out of range.
     * \sa setChannelName
     */
    virtual const char* getChannelName(const size_t index) const = 0;
    /**
     * \brief Gets the gain of a channel
     * \param index [in] : the index of the channel which gain is wanted
     * \return the gain of the \c index th channel if in the correct range
     *         and gain has been specified.
     * \return 1 if in the correct range but gain has not been specified.
     * \return 0 when \c index is out of range.
     * \sa setChannelGain
     */
    virtual float getChannelGain(const size_t index) const = 0;
    /**
     * \brief Gets a channel' measurement unit and its scaling factor
     * \param index [in] : the index of the channel which gain should be set
     * \param channelUnit [in] : the unit
     * \param channelFactor [in] : the scaling factor
     * \return \e true in case of success.
     * \return \e false in case of error. On false, the outputs will be set to default values.
     *
     * See setChannelUnits().
     */
    virtual bool getChannelUnits(const size_t index, size_t& channelUnit, size_t& channelFactor) const = 0;
    /// \todo getChannelLocation
    // virtual getChannelLocation(const size_t index) const=0;
    /**
     * \brief Tests if channel count has been set
     * \return \e true if channel count has been set since last \c reset.
     * \return \e false if channel count has not been set.
     * \sa setChannelCount
     */
    virtual bool isChannelCountSet() const = 0;
    /**
     * \brief Tests if channel name has been set at least once
     * \return \e true if channel name has been set at least once since last \c reset.
     * \return \e false if channel name has not been set.
     * \sa setChannelName
     */
    virtual bool isChannelNameSet() const = 0;
    /**
     * \brief Tests if channel gain has been set at least once
     * \return \e true if channel gain has been set at least once since last \c reset.
     * \return \e false if channel gain has not been set.
     * \sa setChannelGain
     */
    virtual bool isChannelGainSet() const = 0;
    /// \todo isChannelLocationSet
    // virtual bool isChannelLocationSet() const=0;
    /**
     * \brief Tests if channel unit has been set at least once
     * \return \e true if channel unit has been set at least once since last \c reset.
     * \return \e false if channel unit has not been set.
     * \sa setChannelGain
     */
    virtual bool isChannelUnitSet() const = 0;

    //@}
    /** \name Samples information */
    //@{

    /**
     * \brief Sets measured signal sampling rate
     * \param sampling [in] : the sampling rate for the measured signal
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note the sampling rate is a global value. It can not be specified per channel.
     */
    virtual bool setSamplingFrequency(const size_t sampling) = 0;
    /**
     * \brief Gets the sampling rate of the measured signal
     * \return the sampling rate of the measured signal
     * \sa setSamplingFrequency
     */
    virtual size_t getSamplingFrequency() const = 0;
    /**
     * \brief Tests if sampling frequency has been set
     * \return \e true if sampling frequency has been set since last \c reset.
     * \return \e false if sampling frequency has not been set.
     * \sa setSamplingFrequency
     */
    virtual bool isSamplingFrequencySet() const = 0;

    //@}

    /**
     * \brief Destructor
     */
    virtual ~IHeader() { }

    static void copy(IHeader& dst, const IHeader& src)
    {
        // Make sure that nothing lingers, this is mainly for channel units: we have no way to restore dst to an 'unset' state unless we reset
        dst.reset();

        const size_t nChannel = src.getChannelCount();
        dst.setExperimentID(src.getExperimentID());
        dst.setSubjectAge(src.getSubjectAge());
        dst.setSubjectGender(src.getSubjectGender());
        dst.setChannelCount(src.getChannelCount());
        dst.setSamplingFrequency(src.getSamplingFrequency());
        dst.setChannelCount(src.getChannelCount());
        for (size_t i = 0; i < nChannel; ++i) {
            dst.setChannelName(i, src.getChannelName(i));
            dst.setChannelGain(i, src.getChannelGain(i));
        }
        if (src.isChannelUnitSet()) {
            for (size_t i = 0; i < nChannel; ++i) {
                size_t unit = 0, factor = 0;
                src.getChannelUnits(i, unit, factor);           // No need to test for error, will set defaults on fail
                dst.setChannelUnits(i, unit, factor);
            }
        }
    }
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
