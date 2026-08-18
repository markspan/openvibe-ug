#pragma once

#if defined TARGET_HAS_ThirdPartyUSBFirstAmpAPI

#include "../ovasCHeader.h"

#include <windows.h>
#include <FirstAmp.h>

#include<map>

namespace OpenViBE {
namespace AcquisitionServer {

enum EAcquisitionModes { VAmp16 = 0, VAmp8 = 1, VAmp4Fast = 2 };

/**
 * \class CHeaderBrainProductsVAmp
 * \author Laurent Bonnet (INRIA)
 * \date 16 nov 2009
 * \erief The CHeaderBrainProductsVAmp is an Adaptator for the VAmp device.
 *
 * It contains basic functions using the basic header behaviour, and a set of specific functions to handle the Fast Mode data.
 *
 * \sa CDriverBrainProductsVAmp
 */
class CHeaderBrainProductsVAmp final : public IHeader
{
public:

    CHeaderBrainProductsVAmp();
    ~CHeaderBrainProductsVAmp() override { delete m_basicHeader; }
    void reset() override;

    // Experiment information
    bool setExperimentID(const size_t experimentID) override { return m_basicHeader->setExperimentID(experimentID); }
    bool setSubjectAge(const size_t subjectAge) override { return m_basicHeader->setSubjectAge(subjectAge); }
    bool setSubjectGender(const size_t subjectGender) override { return m_basicHeader->setSubjectGender(subjectGender); }

    size_t getExperimentID() const override { return m_basicHeader->getExperimentID(); }
    size_t getSubjectAge() const override { return m_basicHeader->getSubjectAge(); }
    size_t getSubjectGender() const override { return m_basicHeader->getSubjectGender(); }

    bool isExperimentIDSet() const override { return m_basicHeader->isExperimentIDSet(); }
    bool isSubjectAgeSet() const override { return m_basicHeader->isSubjectAgeSet(); }
    bool isSubjectGenderSet() const override { return m_basicHeader->isSubjectGenderSet(); }

    void setImpedanceCheckRequested(const bool state) override { m_basicHeader->setImpedanceCheckRequested(state); }
    bool isImpedanceCheckRequested() const override { return m_basicHeader->isImpedanceCheckRequested(); }
    size_t getImpedanceLimit() const override { return m_impedanceLimit; }

    void setImpedanceLimit(const size_t limit) override { m_impedanceLimit = limit; }

    // Channel information
    bool setChannelCount(const size_t nChannel) override;
    bool setChannelName(const size_t index, const char* name) override;
    bool setChannelGain(const size_t index, const float gain) override;
    bool setChannelUnits(const size_t index, const size_t unit, const size_t factor) override;

    size_t getChannelCount() const override;
    const char* getChannelName(const size_t index) const override;
    float getChannelGain(const size_t index) const override;
    bool getChannelUnits(const size_t index, size_t& unit, size_t& factor) const override;

    bool isChannelCountSet() const override;
    bool isChannelNameSet() const override;
    bool isChannelGainSet() const override;
    bool isChannelUnitSet() const override;

    // Samples information
    bool setSamplingFrequency(const size_t sampling) override { return m_basicHeader->setSamplingFrequency(sampling); }
    size_t getSamplingFrequency() const override { return m_basicHeader->getSamplingFrequency(); }
    bool isSamplingFrequencySet() const override { return m_basicHeader->isSamplingFrequencySet(); }

    //------------- SPECIFIC FUNCTIONS -------------

    EAcquisitionModes getAcquisitionMode() const { return m_acquisitionMode; }
    void setAcquisitionMode(const EAcquisitionModes mode) { m_acquisitionMode = mode; }

    static size_t getEEGChannelCount(size_t acquisitionMode);
    static size_t getAuxiliaryChannelCount(size_t acquisitionMode);
    static size_t getTriggerChannelCount(size_t acquisitionMode);

    // Pair information
    bool setPairCount(const size_t count);
    bool setPairName(const size_t index, const char* name);
    bool setPairGain(const size_t index, const float gain);
    bool setPairUnits(const size_t index, const size_t unit, const size_t factor);
    bool setDeviceId(int id);
    bool setFastModeSettings(t_faDataModeSettings settings);

    size_t getPairCount() const { return m_nPair; }
    const char* getPairName(const size_t index) const;
    float getPairGain(const size_t index) const;
    bool getPairUnits(const size_t index, size_t& unit, size_t& factor) const;
    int getDeviceId() const { return m_deviceID; }
    t_faDataModeSettings getFastModeSettings() const { return m_tFastModeSettings; }

    bool isPairCountSet() const { return m_nPair != size_t(-1) && m_nPair != 0; }
    bool isPairNameSet() const { return isPairCountSet(); }
    bool isPairGainSet() const { return isPairCountSet(); }
    bool isPairUnitSet() const { return isPairCountSet(); }
    bool isDeviceIdSet() const;
    bool isFastModeSettingsSet() const;

    CHeader* getBasicHeader() const { return m_basicHeader; }

protected:

    CHeader* m_basicHeader = nullptr; // the basic header

    // additional information
    int m_deviceID                      = 0;
    EAcquisitionModes m_acquisitionMode = VAmp16;
    t_faDataModeSettings m_tFastModeSettings;

    // Pair information
    size_t m_nPair = 0;
    std::map<size_t, std::string> m_names;
    std::map<size_t, float> m_gains;
    std::map<size_t, std::pair<size_t, size_t>> m_units;

    size_t m_impedanceLimit = 5000; // 5kOhm
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
