#pragma once

#include "ovasIHeader.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CHeader : public IHeader
{
public:
    CHeader();
    ~CHeader() override;
    void reset() override;

    // Experiment information
    bool setExperimentID(const size_t id) override;
    bool setSubjectAge(const size_t age) override;
    bool setSubjectGender(const size_t gender) override;

    size_t getExperimentID() const override;
    size_t getSubjectAge() const override;
    size_t getSubjectGender() const override;

    bool isExperimentIDSet() const override;
    bool isSubjectAgeSet() const override;
    bool isSubjectGenderSet() const override;

    void setImpedanceCheckRequested(const bool state) override;
    bool isImpedanceCheckRequested() const override;
    size_t getImpedanceLimit() const override;
    void setImpedanceLimit(const size_t limit) override;

    // Chanel information
    bool setChannelCount(const size_t nChannel) override;
    bool setChannelName(const size_t index, const char* name) override;
    bool setChannelGain(const size_t index, const float gain) override;
    bool setChannelUnits(const size_t index, const size_t unit, const size_t factor) override;
    // virtual bool setChannelLocation(const size_t index, const float channelLocationX, const float channelLocationY, const float channelLocationZ);

    size_t getChannelCount() const override;
    const char* getChannelName(const size_t index) const override;
    float getChannelGain(const size_t index) const override;
    bool getChannelUnits(const size_t index, size_t& unit, size_t& factor) const override;
    // virtual getChannelLocation(const size_t index) const;

    bool isChannelCountSet() const override;
    bool isChannelNameSet() const override;
    bool isChannelGainSet() const override;
    // virtual bool isChannelLocationSet() const;
    bool isChannelUnitSet() const override;

    // Samples information
    bool setSamplingFrequency(const size_t sampling) override;

    size_t getSamplingFrequency() const override;

    bool isSamplingFrequencySet() const override;

protected:
    IHeader* m_impl = nullptr;
};

class CHeaderAdapter : public IHeader
{
public:
    explicit CHeaderAdapter(IHeader& header) : m_header(header) { }

    void reset() override { return m_header.reset(); }

    // Experiment information
    bool setExperimentID(const size_t id) override { return m_header.setExperimentID(id); }

    bool setSubjectAge(const size_t age) override { return m_header.setSubjectAge(age); }
    bool setSubjectGender(const size_t gender) override { return m_header.setSubjectGender(gender); }

    size_t getExperimentID() const override { return m_header.getExperimentID(); }
    size_t getSubjectAge() const override { return m_header.getSubjectAge(); }
    size_t getSubjectGender() const override { return m_header.getSubjectGender(); }

    bool isExperimentIDSet() const override { return m_header.isExperimentIDSet(); }
    bool isSubjectAgeSet() const override { return m_header.isSubjectAgeSet(); }
    bool isSubjectGenderSet() const override { return m_header.isSubjectGenderSet(); }
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

protected:
    IHeader& m_header;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
