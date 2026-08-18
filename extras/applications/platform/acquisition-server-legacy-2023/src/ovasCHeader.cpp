#include "ovasCHeader.h"

#include <map>
#include <string>

#define NO_VALUE_I 0xffffffff

namespace OpenViBE {
namespace AcquisitionServer {
namespace {
class CHeaderImpl final : public IHeader
{
public:
    CHeaderImpl() {}
    ~CHeaderImpl() override {}
    void reset() override;

    // Experiment information
    bool setExperimentID(const size_t experimentID) override;
    bool setSubjectAge(const size_t subjectAge) override;
    bool setSubjectGender(const size_t subjectGender) override;

    size_t getExperimentID() const override { return m_experimentID; }
    size_t getSubjectAge() const override { return m_subjectAge; }
    size_t getSubjectGender() const override { return m_subjectGender; }

    bool isExperimentIDSet() const override { return m_experimentID != NO_VALUE_I; }
    bool isSubjectAgeSet() const override { return m_subjectAge != NO_VALUE_I; }
    bool isSubjectGenderSet() const override { return m_subjectGender != NO_VALUE_I; }

    void setImpedanceCheckRequested(const bool active) override { m_isImpedanceCheckRequested = active; }
    bool isImpedanceCheckRequested() const override { return m_isImpedanceCheckRequested; }
    size_t getImpedanceLimit() const override { return m_impedanceLimit; }
    void setImpedanceLimit(const size_t limit) override { m_impedanceLimit = limit; }

    // Chanel information
    bool setChannelCount(const size_t nChannel) override;
    bool setChannelName(const size_t index, const char* name) override;
    bool setChannelGain(const size_t index, const float gain) override;
    bool setChannelUnits(const size_t index, const size_t unit, const size_t factor) override;
    // virtual bool setChannelLocation(const size_t index, const float channelLocationX, const float channelLocationY, const float channelLocationZ);

    size_t getChannelCount() const override { return m_nChannel; }
    const char* getChannelName(const size_t index) const override;
    float getChannelGain(const size_t index) const override;
    bool getChannelUnits(const size_t index, size_t& unit, size_t& factor) const override;
    // virtual getChannelLocation(const size_t index) const;

    bool isChannelCountSet() const override { return m_nChannel != NO_VALUE_I && m_nChannel != 0; }
    bool isChannelNameSet() const override { return isChannelCountSet(); }
    bool isChannelGainSet() const override { return isChannelCountSet(); }
    // virtual bool isChannelLocationSet() const;
    bool isChannelUnitSet() const override { return !m_channelUnits.empty(); }

    // Samples information
    bool setSamplingFrequency(const size_t sampling) override;

    size_t getSamplingFrequency() const override { return m_sampling; }

    bool isSamplingFrequencySet() const override { return m_sampling != NO_VALUE_I; }

protected:
    // Experiment information
    size_t m_experimentID  = NO_VALUE_I;
    size_t m_subjectAge    = 18;
    size_t m_subjectGender = OVTK_Value_Gender_NotSpecified;

    // Chanel information
    size_t m_nChannel = NO_VALUE_I;
    std::map<size_t, std::string> m_channelNames;
    std::map<size_t, float> m_channelsGain;
    std::map<size_t, std::pair<size_t, size_t>> m_channelUnits;

    // Samples information
    size_t m_sampling = NO_VALUE_I;

    // Impedance check
    bool m_isImpedanceCheckRequested = false;
    size_t m_impedanceLimit          = NO_VALUE_I;
};
}  // namespace
//___________________________________________________________________//
//                                                                   //

void CHeaderImpl::reset()
{
    m_experimentID  = NO_VALUE_I;
    m_subjectAge    = NO_VALUE_I;
    m_subjectGender = NO_VALUE_I;
    m_nChannel      = NO_VALUE_I;
    m_channelNames.clear();
    m_channelsGain.clear();
    m_channelUnits.clear();
    m_sampling                  = NO_VALUE_I;
    m_isImpedanceCheckRequested = false;
    m_nChannel                  = NO_VALUE_I;
}

//___________________________________________________________________//
//                                                                   //

// Experiment information
bool CHeaderImpl::setExperimentID(const size_t experimentID)
{
    m_experimentID = experimentID;
    return m_experimentID != NO_VALUE_I;
}

bool CHeaderImpl::setSubjectAge(const size_t subjectAge)
{
    m_subjectAge = subjectAge;
    return m_subjectAge != NO_VALUE_I;
}

bool CHeaderImpl::setSubjectGender(const size_t subjectGender)
{
    m_subjectGender = subjectGender;
    return m_subjectGender != NO_VALUE_I;
}

//___________________________________________________________________//
//                                                                   //

// Chanel information

bool CHeaderImpl::setChannelCount(const size_t nChannel)
{
    m_nChannel = nChannel;
    // m_channelNames.clear();
    // m_channelsGain.clear();
    return m_nChannel != NO_VALUE_I;
}

bool CHeaderImpl::setChannelName(const size_t index, const char* name)
{
    m_channelNames[index] = name;
    return index < m_nChannel;
}

bool CHeaderImpl::setChannelGain(const size_t index, const float gain)
{
    m_channelsGain[index] = gain;
    return index < m_nChannel;
}

bool CHeaderImpl::setChannelUnits(const size_t index, const size_t unit, const size_t factor)
{
    m_channelUnits[index] = std::pair<size_t, size_t>(unit, factor);
    return index < m_nChannel;
}

// bool CHeaderImpl::setChannelLocation(const size_t index, const float channelLocationX, const float channelLocationY, const float channelLocationZ);

const char* CHeaderImpl::getChannelName(const size_t index) const
{
    const auto i = m_channelNames.find(index);
    if (i == m_channelNames.end()) { return ""; }
    return i->second.c_str();
}

float CHeaderImpl::getChannelGain(const size_t index) const
{
    const auto i = m_channelsGain.find(index);
    if (i == m_channelsGain.end()) { return (index < m_nChannel ? 1.0F : 0.0F); }
    return i->second;
}

bool CHeaderImpl::getChannelUnits(const size_t index, size_t& unit, size_t& factor) const
{
    const auto i = m_channelUnits.find(index);
    if (i == m_channelUnits.end()) {
        unit   = OVTK_UNIT_Unspecified;
        factor = OVTK_FACTOR_Base;

        return false;
    }

    unit   = (i->second).first;
    factor = (i->second).second;

    return true;
}

//___________________________________________________________________//
//                                                                   //

// Samples information
bool CHeaderImpl::setSamplingFrequency(const size_t sampling)
{
    m_sampling = sampling;
    return m_sampling != NO_VALUE_I;
}

//___________________________________________________________________//
//                                                                   //

CHeader::CHeader() { m_impl = new CHeaderImpl(); }
CHeader::~CHeader() { delete m_impl; }
void CHeader::reset() { m_impl->reset(); }

// Experiment information
bool CHeader::setExperimentID(const size_t id) { return m_impl->setExperimentID(id); }
bool CHeader::setSubjectAge(const size_t age) { return m_impl->setSubjectAge(age); }
bool CHeader::setSubjectGender(const size_t gender) { return m_impl->setSubjectGender(gender); }
size_t CHeader::getExperimentID() const { return m_impl->getExperimentID(); }
size_t CHeader::getSubjectAge() const { return m_impl->getSubjectAge(); }
size_t CHeader::getSubjectGender() const { return m_impl->getSubjectGender(); }
bool CHeader::isExperimentIDSet() const { return m_impl->isExperimentIDSet(); }
bool CHeader::isSubjectAgeSet() const { return m_impl->isSubjectAgeSet(); }
bool CHeader::isSubjectGenderSet() const { return m_impl->isSubjectGenderSet(); }

void CHeader::setImpedanceCheckRequested(const bool state) { m_impl->setImpedanceCheckRequested(state); }
bool CHeader::isImpedanceCheckRequested() const { return m_impl->isImpedanceCheckRequested(); }
size_t CHeader::getImpedanceLimit() const { return m_impl->getImpedanceLimit(); }
void CHeader::setImpedanceLimit(const size_t limit) { m_impl->setImpedanceLimit(limit); }

// Chanel information
bool CHeader::setChannelCount(const size_t nChannel) { return m_impl->setChannelCount(nChannel); }
bool CHeader::setChannelName(const size_t index, const char* name) { return m_impl->setChannelName(index, name); }
bool CHeader::setChannelGain(const size_t index, const float gain) { return m_impl->setChannelGain(index, gain); }
bool CHeader::setChannelUnits(const size_t index, const size_t unit, const size_t factor) { return m_impl->setChannelUnits(index, unit, factor); }
size_t CHeader::getChannelCount() const { return m_impl->getChannelCount(); }
const char* CHeader::getChannelName(const size_t index) const { return m_impl->getChannelName(index); }
float CHeader::getChannelGain(const size_t index) const { return m_impl->getChannelGain(index); }
bool CHeader::getChannelUnits(const size_t index, size_t& unit, size_t& factor) const { return m_impl->getChannelUnits(index, unit, factor); }
bool CHeader::isChannelCountSet() const { return m_impl->isChannelCountSet(); }
bool CHeader::isChannelNameSet() const { return m_impl->isChannelNameSet(); }
bool CHeader::isChannelGainSet() const { return m_impl->isChannelGainSet(); }
bool CHeader::isChannelUnitSet() const { return m_impl->isChannelUnitSet(); }

// Samples information
bool CHeader::setSamplingFrequency(const size_t sampling) { return m_impl->setSamplingFrequency(sampling); }
size_t CHeader::getSamplingFrequency() const { return m_impl->getSamplingFrequency(); }
bool CHeader::isSamplingFrequencySet() const { return m_impl->isSamplingFrequencySet(); }

}  // namespace AcquisitionServer
}  // namespace OpenViBE
