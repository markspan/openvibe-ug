#pragma once

#include "ovasIHeader.h"

#define NO_VALUE_I 0xffffffff

namespace OpenViBE {
namespace AcquisitionServer {
class CHeader : public IHeader
{
public:
    CHeader();
    ~CHeader() override;
    void reset() override;

    // Parameters

    OpenViBE::Parameters::ParameterInt& getExperimentIDParameter() override;
    int getExperimentID() const override;
    void setExperimentID(const int id) override;
    OpenViBE::Parameters::ParameterInt& getSubjectAgeParameter() override;
    int getSubjectAge() const override;
    void setSubjectAge(const int age) override;
    OpenViBE::Parameters::ParameterListString& getSubjectGenderParameter() override;
    std::string getSubjectGender() const override;
    void setSubjectGender(const std::string gender) override;
    OpenViBE::Parameters::ParameterInt& getSamplingFrequencyParameter() override;
    int getSamplingFrequency() const override;
    void setSamplingFrequency(const int frequency) override;
    OpenViBE::Parameters::ParameterInt& getChannelCountParameter() override;
    int getChannelCount() const override;
    void setChannelCount(const int count) override;

    void setImpedanceCheckRequested(const bool state) override;
    bool isImpedanceCheckRequested() const override;
    size_t getImpedanceLimit() const override;
    void setImpedanceLimit(const size_t limit) override;

    // Chanel information
    bool setChannelName(const size_t index, const char* name) override;
    bool setChannelUnits(const size_t index, const size_t unit, const size_t factor) override;
    // virtual bool setChannelLocation(const size_t index, const float channelLocationX, const float channelLocationY, const float channelLocationZ);

    const char* getChannelName(const size_t index) const override;
    bool getChannelUnits(const size_t index, size_t& unit, size_t& factor) const override;
    // virtual getChannelLocation(const size_t index) const;

    bool isChannelNameSet() const override;
    // virtual bool isChannelLocationSet() const;
    bool isChannelUnitSet() const override;

protected:
    IHeader* m_impl = nullptr;
};

/* Not sure this is needed anymore.
*
*class CHeaderAdapter : public IHeader
{
public:
    explicit CHeaderAdapter(IHeader& header) { }

    void reset() override { return m_header.reset(); }

    // Experiment information
    bool setExperimentID(const size_t id) override { return m_header.setExperimentID(id); }

    bool setSubjectAge(const size_t age) override { return m_header.setSubjectAge(age); }
    bool setSubjectGender(const size_t gender) override { return m_header.setSubjectGender(gender); }

    OpenViBE::Parameters::ParameterInt& getExperimentID() const override { return m_header.getExperimentID(); }
    OpenViBE::Parameters::ParameterInt& getSubjectAge() const override { return m_header.getSubjectAge(); }
    OpenViBE::Parameters::ParameterListString& getSubjectGender() const override { return m_header.getSubjectGender(); }

    void setImpedanceCheckRequested(const bool active) override { m_header.setImpedanceCheckRequested(active); }
    bool isImpedanceCheckRequested() const override { return m_header.isImpedanceCheckRequested(); }
    size_t getImpedanceLimit() const override { return m_header.getImpedanceLimit(); }
    void setImpedanceLimit(const size_t impedanceLimit) override { m_header.setImpedanceLimit(impedanceLimit); }

    // Channel information
    bool setChannelCount(const size_t nChannel) override { return m_header.setChannelCount(nChannel); }
    bool setChannelName(const size_t index, const char* name) override { return m_header.setChannelName(index, name); }
    bool setChannelGain(const size_t index, const float gain) override { return m_header.setChannelGain(index, gain); }

    bool setChannelUnits(const size_t index, const size_t unit, const size_t factor) override { return m_header.setChannelUnits(index, unit, factor); }

    size_t getChannelCount() const override { return m_header.getChannelCount(); }
    const char* getChannelName(const size_t index) const override { return m_header.getChannelName(index); }
    float getChannelGain(const size_t index) const override { return m_header.getChannelGain(index); }

    bool getChannelUnits(const size_t index, size_t& unit, size_t& factor) const override { return m_header.getChannelUnits(index, unit, factor); }

    bool isChannelCountSet() const override { return m_header.isChannelCountSet(); }
    bool isChannelNameSet() const override { return m_header.isChannelNameSet(); }
    bool isChannelGainSet() const override { return m_header.isChannelGainSet(); }
    bool isChannelUnitSet() const override { return m_header.isChannelUnitSet(); }

    // Samples information
    bool setSamplingFrequency(const size_t sampling) override { return m_header.setSamplingFrequency(sampling); }
    size_t getSamplingFrequency() const override { return m_header.getSamplingFrequency(); }
    bool isSamplingFrequencySet() const override { return m_header.isSamplingFrequencySet(); }

};*/
}  // namespace AcquisitionServer
}  // namespace OpenViBE
