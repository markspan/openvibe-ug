#include "ovasCHeader.h"

#include <map>
#include <string>

namespace OpenViBE {
namespace AcquisitionServer {
namespace {
class CHeaderImpl final : public IHeader
{
public:
    CHeaderImpl() :
        m_experimentID("Identifier", "Identifier", 0, 0, 10000000),
        m_subjectAge("Age", "Age", 18, 0, 115),
        m_subjectGender("Gender", "the gender", {"Other", "Female", "Male"}),
        m_channelCount("ChannelCount", "the channel count", 0, 0, 1024),
        m_samplingFrequency("SamplingFrequency", "the sampling frequency", 0, 0, 1000000) {}
    ~CHeaderImpl() override {}
    void reset() override;

    // Experiment information

    OpenViBE::Parameters::ParameterInt& getExperimentIDParameter() override { return m_experimentID; }
    int getExperimentID() const override { return m_experimentID.value(); }
    void setExperimentID(const int id) override { m_experimentID.setValue(id); }
    OpenViBE::Parameters::ParameterInt& getSubjectAgeParameter() override { return m_subjectAge; }
    int getSubjectAge() const override { return m_subjectAge.value(); }
    void setSubjectAge(const int age) override { m_subjectAge.setValue(age); }
    OpenViBE::Parameters::ParameterListString& getSubjectGenderParameter() override { return m_subjectGender; }
    std::string getSubjectGender() const override { return m_subjectGender.value(); }
    void setSubjectGender(const std::string gender) override { m_subjectGender.setValue(gender); }
    OpenViBE::Parameters::ParameterInt& getChannelCountParameter() override { return m_channelCount; }
    int getChannelCount() const override { return m_channelCount.value(); }
    void setChannelCount(const int count) override { m_channelCount.setValue(count); }
    OpenViBE::Parameters::ParameterInt& getSamplingFrequencyParameter() override { return m_samplingFrequency; }
    int getSamplingFrequency() const override { return m_samplingFrequency.value(); }
    void setSamplingFrequency(const int frequency) override { m_samplingFrequency.setValue(frequency); }

    void setImpedanceCheckRequested(const bool active) override { m_isImpedanceCheckRequested = active; }
    bool isImpedanceCheckRequested() const override { return m_isImpedanceCheckRequested; }
    size_t getImpedanceLimit() const override { return m_impedanceLimit; }
    void setImpedanceLimit(const size_t limit) override { m_impedanceLimit = limit; }

    // Chanel information
    bool setChannelName(const size_t index, const char* name) override;
    bool setChannelUnits(const size_t index, const size_t unit, const size_t factor) override;
    // virtual bool setChannelLocation(const size_t index, const float channelLocationX, const float channelLocationY, const float channelLocationZ);

    const char* getChannelName(const size_t index) const override;
    bool getChannelUnits(const size_t index, size_t& unit, size_t& factor) const override;
    // virtual getChannelLocation(const size_t index) const;
    // virtual bool isChannelLocationSet() const;
    bool isChannelNameSet() const override { return !m_channelNames.empty(); }
    bool isChannelUnitSet() const override { return !m_channelUnits.empty(); }

protected:
    // Experiment information
    OpenViBE::Parameters::ParameterInt m_experimentID;
    OpenViBE::Parameters::ParameterInt m_subjectAge;
    OpenViBE::Parameters::ParameterListString m_subjectGender;
    OpenViBE::Parameters::ParameterInt m_channelCount;
    OpenViBE::Parameters::ParameterInt m_samplingFrequency;

    // Chanel information
    std::map<size_t, std::string> m_channelNames;
    std::map<size_t, std::pair<size_t, size_t>> m_channelUnits;

    // Impedance check
    bool m_isImpedanceCheckRequested = false;
    size_t m_impedanceLimit          = NO_VALUE_I;
};
}  // namespace
//___________________________________________________________________//
//                                                                   //

void CHeaderImpl::reset()
{
    m_experimentID  = 0;
    m_subjectAge    = 0;
    m_channelCount  = 0;
    m_samplingFrequency = 1;
    m_channelNames.clear();
    m_channelUnits.clear();
    m_isImpedanceCheckRequested = false;
}

//___________________________________________________________________//
//                                                                   //

bool CHeaderImpl::setChannelName(const size_t index, const char* name)
{
    m_channelNames[index] = name;
    return index < m_channelCount.value();
}

bool CHeaderImpl::setChannelUnits(const size_t index, const size_t unit, const size_t factor)
{
    m_channelUnits[index] = std::pair<size_t, size_t>(unit, factor);
    return index < m_channelCount.value();
}

// bool CHeaderImpl::setChannelLocation(const size_t index, const float channelLocationX, const float channelLocationY, const float channelLocationZ);

const char* CHeaderImpl::getChannelName(const size_t index) const
{
    const auto i = m_channelNames.find(index);
    if (i == m_channelNames.end()) { return ""; }
    return i->second.c_str();
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

CHeader::CHeader() { m_impl = new CHeaderImpl(); }
CHeader::~CHeader() { delete m_impl; }
void CHeader::reset() { m_impl->reset(); }

// Experiment information
OpenViBE::Parameters::ParameterInt& CHeader::getExperimentIDParameter() { return m_impl->getExperimentIDParameter(); }
int CHeader::getExperimentID() const { return m_impl->getExperimentID(); }
void CHeader::setExperimentID(const int id) { m_impl->setExperimentID(id); }
OpenViBE::Parameters::ParameterInt& CHeader::getSubjectAgeParameter() { return m_impl->getSubjectAgeParameter(); }
int CHeader::getSubjectAge() const { return m_impl->getSubjectAge(); }
void CHeader::setSubjectAge(const int age) { m_impl->setSubjectAge(age); }
OpenViBE::Parameters::ParameterListString& CHeader::getSubjectGenderParameter() { return m_impl->getSubjectGenderParameter(); }
std::string CHeader::getSubjectGender() const { return m_impl->getSubjectGender(); }
void CHeader::setSubjectGender(const std::string gender) { m_impl->setSubjectGender(gender); }
OpenViBE::Parameters::ParameterInt& CHeader::getChannelCountParameter() { return m_impl->getChannelCountParameter(); }
int CHeader::getChannelCount() const { return m_impl->getChannelCount(); }
void CHeader::setChannelCount(const int count) { m_impl->setChannelCount(count); }
OpenViBE::Parameters::ParameterInt& CHeader::getSamplingFrequencyParameter() { return m_impl->getSamplingFrequencyParameter(); }
int CHeader::getSamplingFrequency() const { return m_impl->getSamplingFrequency(); }
void CHeader::setSamplingFrequency(const int frequency) { m_impl->setSamplingFrequency(frequency); }

void CHeader::setImpedanceCheckRequested(const bool state) { m_impl->setImpedanceCheckRequested(state); }
bool CHeader::isImpedanceCheckRequested() const { return m_impl->isImpedanceCheckRequested(); }
size_t CHeader::getImpedanceLimit() const { return m_impl->getImpedanceLimit(); }
void CHeader::setImpedanceLimit(const size_t limit) { m_impl->setImpedanceLimit(limit); }

// Chanel information
bool CHeader::setChannelName(const size_t index, const char* name) { return m_impl->setChannelName(index, name); }
bool CHeader::setChannelUnits(const size_t index, const size_t unit, const size_t factor) { return m_impl->setChannelUnits(index, unit, factor); }
const char* CHeader::getChannelName(const size_t index) const { return m_impl->getChannelName(index); }
bool CHeader::getChannelUnits(const size_t index, size_t& unit, size_t& factor) const { return m_impl->getChannelUnits(index, unit, factor); }
bool CHeader::isChannelNameSet() const { return m_impl->isChannelNameSet(); }
bool CHeader::isChannelUnitSet() const { return m_impl->isChannelUnitSet(); }

}  // namespace AcquisitionServer
}  // namespace OpenViBE
