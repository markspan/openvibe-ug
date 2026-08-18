#include "ovasCHeaderBrainProductsVAmp.h"
#include "ovasCConfigurationBrainProductsVAmp.h"

#if defined TARGET_HAS_ThirdPartyUSBFirstAmpAPI

#include <windows.h>
#include <FirstAmp.h>

#include <map>
#include <string>

#define _NoValueI_ 0xffffffff

namespace OpenViBE {
namespace AcquisitionServer {

static size_t eegChannelCounts[]       = { 16, 8, 4 };
static size_t auxiliaryChannelCounts[] = { 2, 2, 0 };
static size_t triggerChannelCounts[]   = { 1, 1, 1 };

//___________________________________________________________________//
//                                                                   //

CHeaderBrainProductsVAmp::CHeaderBrainProductsVAmp()
{
    m_basicHeader = new CHeader();

    // additional information
    m_deviceID        = -1;
    m_acquisitionMode = VAmp16;

    // Pair information
    m_nPair = 0;
}

void CHeaderBrainProductsVAmp::reset()
{
    m_basicHeader->reset();
    m_deviceID = FA_ID_INVALID;

    // Pair information
    m_nPair = 0;
}

//___________________________________________________________________//
//                                                                   //

size_t CHeaderBrainProductsVAmp::getEEGChannelCount(const size_t acquisitionMode) { return eegChannelCounts[acquisitionMode]; }
size_t CHeaderBrainProductsVAmp::getAuxiliaryChannelCount(const size_t acquisitionMode) { return auxiliaryChannelCounts[acquisitionMode]; }
size_t CHeaderBrainProductsVAmp::getTriggerChannelCount(const size_t acquisitionMode) { return triggerChannelCounts[acquisitionMode]; }

// Pair information

bool CHeaderBrainProductsVAmp::setPairCount(const size_t count)
{
    m_nPair = count;
    m_names.clear();
    m_gains.clear();
    return m_nPair != size_t(-1);
}

bool CHeaderBrainProductsVAmp::setPairName(const size_t index, const char* name)
{
    m_names[index] = name;
    return index < m_nPair;
}

bool CHeaderBrainProductsVAmp::setPairGain(const size_t index, const float gain)
{
    m_gains[index] = gain;
    return index < m_nPair;
}

bool CHeaderBrainProductsVAmp::setPairUnits(const size_t index, const size_t unit, const size_t factor)
{
    m_units[index] = std::pair<size_t, size_t>(unit, factor);
    return index < m_nPair;
}

bool CHeaderBrainProductsVAmp::setFastModeSettings(t_faDataModeSettings settings)
{
    m_tFastModeSettings = settings;
    return isFastModeSettingsSet();
}

const char* CHeaderBrainProductsVAmp::getPairName(const size_t index) const
{
    const auto i = m_names.find(index);
    if (i == m_names.end()) { return ""; }
    return i->second.c_str();
}

float CHeaderBrainProductsVAmp::getPairGain(const size_t index) const
{
    const auto i = m_gains.find(index);
    if (i == m_gains.end()) { return (index < m_nPair ? 1.0f : 0.0f); }
    return i->second;
}

bool CHeaderBrainProductsVAmp::getPairUnits(const size_t index, size_t& unit, size_t& factor) const
{
    const auto i = m_units.find(index);
    if (i == m_units.end())
    {
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

bool CHeaderBrainProductsVAmp::setDeviceId(const int id)
{
    m_deviceID = id;
    return m_deviceID != _NoValueI_;
}

bool CHeaderBrainProductsVAmp::isDeviceIdSet() const { return m_deviceID != _NoValueI_; }

bool CHeaderBrainProductsVAmp::isFastModeSettingsSet() const
{
    return (m_tFastModeSettings.Mode20kHz4Channels.ChannelsPos[0] == _NoValueI_
            || m_tFastModeSettings.Mode20kHz4Channels.ChannelsNeg[0] == _NoValueI_
            || m_tFastModeSettings.Mode20kHz4Channels.ChannelsPos[1] == _NoValueI_
            || m_tFastModeSettings.Mode20kHz4Channels.ChannelsNeg[1] == _NoValueI_
            || m_tFastModeSettings.Mode20kHz4Channels.ChannelsPos[2] == _NoValueI_
            || m_tFastModeSettings.Mode20kHz4Channels.ChannelsNeg[2] == _NoValueI_
            || m_tFastModeSettings.Mode20kHz4Channels.ChannelsPos[3] == _NoValueI_
            || m_tFastModeSettings.Mode20kHz4Channels.ChannelsNeg[3] == _NoValueI_);
}

//___________________________________________________________________//
//                                                                   //

// Channel information

bool CHeaderBrainProductsVAmp::setChannelCount(const size_t nChannel)
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->setPairCount(nChannel);
    }
    return m_basicHeader->setChannelCount(nChannel);
}

bool CHeaderBrainProductsVAmp::setChannelName(const size_t index, const char* name)
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->setPairName(index, name);
    }
    return m_basicHeader->setChannelName(index, name);
}

bool CHeaderBrainProductsVAmp::setChannelGain(const size_t index, const float gain)
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->setPairGain(index, gain);
    }
    return m_basicHeader->setChannelGain(index, gain);
}

bool CHeaderBrainProductsVAmp::setChannelUnits(const size_t index, const size_t unit, const size_t factor)
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->setPairUnits(index, unit, factor);
    }
    return m_basicHeader->setChannelUnits(index, unit, factor);
}

size_t CHeaderBrainProductsVAmp::getChannelCount() const
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->getPairCount();
    }
    return m_basicHeader->getChannelCount();
}

const char* CHeaderBrainProductsVAmp::getChannelName(const size_t index) const
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->getPairName(index);
    }
    return m_basicHeader->getChannelName(index);
}

float CHeaderBrainProductsVAmp::getChannelGain(const size_t index) const
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->getPairGain(index);
    }
    return m_basicHeader->getChannelGain(index);
}

bool CHeaderBrainProductsVAmp::getChannelUnits(const size_t index, size_t& unit, size_t& factor) const
{
    if (m_acquisitionMode == VAmp4Fast) { return this->getPairUnits(index, unit, factor); }
    return m_basicHeader->getChannelUnits(index, unit, factor);
}

bool CHeaderBrainProductsVAmp::isChannelCountSet() const
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->isPairCountSet();
    }
    return m_basicHeader->isChannelCountSet();
}

bool CHeaderBrainProductsVAmp::isChannelNameSet() const
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->isPairNameSet();
    }
    return m_basicHeader->isChannelNameSet();
}

bool CHeaderBrainProductsVAmp::isChannelGainSet() const
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->isPairGainSet();
    }
    return m_basicHeader->isChannelGainSet();
}

bool CHeaderBrainProductsVAmp::isChannelUnitSet() const
{
    if (m_acquisitionMode == VAmp4Fast)
    {
        // in fast mode the channel count is the pair count (to display in the designer as a "channel")
        return this->isPairUnitSet();
    }
    return m_basicHeader->isChannelUnitSet();
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyUSBFirstAmpAPI
