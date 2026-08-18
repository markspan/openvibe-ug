#pragma once

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"
#include "../brainproducts-base/CDriverBrainProductsBase.h"

#include <deque>

#ifndef BYTE
typedef unsigned char BYTE;
#endif

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverBrainProductsLiveAmp
 * \author Ratko Petrovic (Brain Products GmbH)
 * \date Mon Nov 21 14:57:37 2016
 * \brief The CDriverBrainProductsLiveAmp allows the acquisition server to acquire data from a Brain Products LiveAmp device.
 *
 * TODO: details
 *
 * \sa CConfigurationBrainProductsLiveAmp
 */
class CDriverBrainProductsLiveAmp final : public CDriverBrainProductsBase
{
public:
    explicit CDriverBrainProductsLiveAmp(IDriverContext& ctx);
    ~CDriverBrainProductsLiveAmp() override;

    const char* getName() override { return "Brain Products LiveAmp"; }

    bool isConfigurable() override;
    bool configure() override;

    bool initializeSpecific() override;

    bool isFlagSet(const EDriverFlag flag) const override
    {
        if (flag == EDriverFlag::IsUnstable) { return false; }  // switch to "Stable" on 3.5.2017 - RP
        return false; // No flags are set
    }

private:
    SettingsHelper m_settings;

    uint32_t m_physicalSampling  = 250;
    bool m_useAccChannels        = false;
    bool m_useBipolarChannels    = false;
    uint32_t m_nEnabledChannels  = 0;

    uint32_t m_nChannel           = 0;
    uint32_t m_nEEG               = 32;
    uint32_t m_nAux               = 0;
    uint32_t m_nACC               = 0;
    uint32_t m_nBipolar           = 0;
    uint32_t m_nTriggersIn        = 0;

    std::string m_sSerialNumber = "";

    std::vector<size_t> m_samplings;

    /**
    * \brief Check how many available channels has LiveAmp. Get count of EEG,AUX and ACC channels.
        Compares the count with the amplifier/driver settings.
    * \return True if checking is successful. 
    */
    bool checkAvailableChannels();

    /**
    * \brief Disable all available channels
    * \return True if function is successful. 
    */
    bool disableAllAvailableChannels();

    /**
    * \brief Get indexes of the channel that will be used for acquisition.
    * \return True if function is successful.  
    */
    bool getChannelIndices();

};
}  //namespace AcquisitionServer
}  //namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
