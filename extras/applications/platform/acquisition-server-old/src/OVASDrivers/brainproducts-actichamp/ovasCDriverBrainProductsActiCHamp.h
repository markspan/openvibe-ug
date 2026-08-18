///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#pragma once

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../brainproducts-base/CDriverBrainProductsBase.h"

#include <deque>



namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverBrainProductsActiCHamp
 * \author Mensia Technologies
 */
class CDriverBrainProductsActiCHamp final : public CDriverBrainProductsBase
{
public:
    explicit CDriverBrainProductsActiCHamp(IDriverContext& ctx);
    void release() { delete this; }
    const char* getName() override { return "Brain Products actiCHamp"; }

    bool initializeSpecific() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

protected:

    bool setEnabledChannels();
    bool setActiveShieldGain();

    SettingsHelper m_settings;

    int32_t m_activeShieldGain   = 0;
    uint32_t m_nEEGChannels     = 0;
    bool m_useAuxChannels        = false;

    // sampling Freq. is handled by a Base and a divisor.
    // This variable is only useful for setting saving/initialization purpose
    size_t m_samplingFrequencySetting = 0;


};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
