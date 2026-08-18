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
    /** \name General parameters */
    //@{

    /**
     * \brief Resets this header
     *
     * When called, this function resets all the header content to its
     * default values. Most of the is*Set will return \e false after
     * this call.
     */
    virtual void reset() = 0;

    /**
     * \brief Gets the experiment identifier parameter
     * \return the experiement identifier parameter.
     */
    virtual OpenViBE::Parameters::ParameterInt& getExperimentIDParameter() = 0;
    /**
     * \brief Gets the experiment identifier value
     * \return the experiement identifier.
     * 
     * This is a convenience function that avoids getting the paramater object
    */
    virtual int getExperimentID() const = 0;
    /**
     * \brief Sets the experiment identifier value
     * \param id [in] : the new experiment identifier
     * 
    */
    virtual void setExperimentID(const int id) = 0;
    /**
     * \brief Gets the subject age parameter
     * \return the subject age parameter.
     */
    virtual OpenViBE::Parameters::ParameterInt& getSubjectAgeParameter() = 0;
    /**
     * \brief Gets the subject age value
     * \return the subject age.
     * 
     * This is a convenience function that avoids getting the paramater object
    */
    virtual int getSubjectAge() const = 0;
    /**
     * \brief sets the subject age value
     * \param age [in] the new subject age
     * 
     * This is a convenience function that avoids getting the paramater object
    */
    virtual void setSubjectAge(const int age) = 0;
    /**
     * \brief Gets the subject gender parameter
     * \return the subject gender parameter.
     */
    virtual OpenViBE::Parameters::ParameterListString& getSubjectGenderParameter() = 0;
    /**
     * \brief Gets the geneder value
     * \return the subject gender
     * 
     * This is a convenience function that avoids getting the paramater object
    */
    virtual std::string getSubjectGender() const = 0;
    /**
     * \brief Sets the subject gender value
     * \param the subject gender
    */
    virtual void setSubjectGender(const std::string gender) = 0;
    /**
     * \brief Gets the sampling frequency of the measured signal
     * \return the sampling frequency parameter
    */
    virtual OpenViBE::Parameters::ParameterInt& getSamplingFrequencyParameter() = 0;
    /**
     * \brief Gets the sampling frequency value
     * \return the sampling frequency
     *
     * This is a convenience function that avoids getting the paramater object
    */
    virtual int getSamplingFrequency() const = 0;
    /**
     * \brief Sets the sampling frequency value
     * \param freq [in] : the new sampling frequency
     * 
    */
    virtual void setSamplingFrequency(const int freq) = 0;
    /**
     * \brief Gets the number of channels parameter for this header
     * \return the number of channels.
     */
    virtual OpenViBE::Parameters::ParameterInt& getChannelCountParameter() = 0;
    /**
     * \brief Gets the number of channels value
     * \return the number of channels.
     * 
     * This is a convenience function that avoids getting the paramater object
    */
    virtual int getChannelCount() const = 0;
    /**
     * \brief Sets the number of channels value
     * \param count [in] : the new number of channels
    */
    virtual void setChannelCount(const int count) = 0;
    /**
     * \brief Sets whether impedance check is requested
    */
    virtual void setImpedanceCheckRequested(const bool active) = 0;
    /**
     * \brief Gets whether impedance check is requested
    */
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
     * \brief Tests if channel name has been set at least once
     * \return \e true if channel name has been set at least once since last \c reset.
     * \return \e false if channel name has not been set.
     * \sa setChannelName
     */
    virtual bool isChannelNameSet() const = 0;
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
    /**
     * \brief Destructor
     */
    virtual ~IHeader() { }

    void fromString(const std::string& str)
    {
        std::istringstream in(str);
        in >> *this;
    }

    std::string toString() const
    {
        std::ostringstream out;
        out << *this;
        return out.str();
    }

    friend std::ostream& operator<<(std::ostream& out, const IHeader& var);
    friend std::istream& operator>>(std::istream& in, IHeader& var);
};

// Writes fields of IHeader to a stream
inline std::ostream& operator<<(std::ostream& out, const IHeader& var)
{
    out << "ExperimentID " << var.getExperimentID() << " ";
    out << "SubjectAge " << var.getSubjectAge() << " ";
    out << "SubjectGender " << var.getSubjectGender() << " ";
    out << "ImpedanceCheck " << var.isImpedanceCheckRequested() << " ";
    out << "SamplingFrequency " << var.getSamplingFrequency() << " ";
    out << "Channels " << var.getChannelCount() << " ";

    if (var.isChannelNameSet()) {
        out << "Names ";
        for (size_t i = 0; i < var.getChannelCount(); ++i) { out << var.getChannelName(i) << ";"; }
        out << " ";
    }
    return out;
}

// Reads fields of IHeader from a stream
inline std::istream& operator>>(std::istream& in, IHeader& var)
{
    std::string token;

    while (std::getline(in, token, ' ')) {
        if (token.empty()) { continue; }

        // std::cout << "Got token [" << token << "]\n";

        if (token == "ExperimentID") {
            // std::cout << "Parsing experiment id\n";
            uint32_t tmp;
            in >> tmp;
            var.setExperimentID(tmp);
        }
        else if (token == "SubjectAge") {
            // std::cout << "Parsing age\n";
            uint32_t tmp;
            in >> tmp;
            var.setSubjectAge(tmp);
        }
        else if (token == "SubjectGender") {
            // std::cout << "Parsing gender\n";
            std::string tmp;
            in >> tmp;
            var.setSubjectGender(tmp);
        }
        else if (token == "ImpedanceCheck") {
            // std::cout << "Parsing impedance check\n";
            bool tmp;
            in >> tmp;
            var.setImpedanceCheckRequested(tmp);
        }
        else if (token == "SamplingFrequency") {
            // std::cout << "Parsing freq\n";

            uint32_t tmp;
            in >> tmp;
            var.setSamplingFrequency(tmp);

            // std::cout << "  Got " << sampling << "\n";
        }
        else if (token == "Channels") {
            // std::cout << "Parsing chn count\n";
            uint32_t tmp;
            in >> tmp;
            var.setChannelCount(tmp);

            // std::cout << "  Got " << nChannels << "\n";
        }
        else if (token == "Names") {
            // std::cout << "Parsing names\n";

            std::string tmp;
            for (size_t i = 0; i < var.getChannelCount(); ++i) {
                std::getline(in, tmp, ';');
                // std::cout << "  Parsed " << value << "\n";
                var.setChannelName(i, tmp.c_str());
            }
        }
        else { }// std::cout << "Unexpected token [" << token << "]\n";
    }

    return in;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
