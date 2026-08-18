#pragma once

#ifdef TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include "../ovasCConfigurationBuilder.h"
#include "../brainproducts-base/CDriverBrainProductsBase.h"

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationBrainProductsLiveAmp
 * \author Ratko Petrovic (Brain Products GmbH)
 * \date Mon Nov 21 14:57:37 2016
 * \brief The CConfigurationBrainProductsLiveAmp handles the configuration dialog specific to the Brain Products LiveAmp device.
 *
 * TODO: details
 *
 * \sa CDriverBrainProductsLiveAmp
 */
class CConfigurationBrainProductsLiveAmp final : public CConfigurationBuilder
{
public:
    // you may have to add to your constructor some reference parameters
    // for example, a connection ID:
    CConfigurationBrainProductsLiveAmp(const char* gtkBuilderFilename, uint32_t& physicalSampling, uint32_t& rCountEEG,
                                       uint32_t& rCountBip, uint32_t& rCountAUX, uint32_t& rCountACC, bool& rUseAccChannels, uint32_t& goodImpedanceLimit,
                                       uint32_t& badImpedanceLimit, DeviceSelection& deviceSelection, bool& rUseBipolarChannels);

    bool preConfigure() override;
    bool postConfigure() override;

protected:

    uint32_t& m_physicalSampling;
    uint32_t& m_nEEG;
    uint32_t& m_nBip;
    uint32_t& m_nAUX;
    uint32_t& m_nACC;
    bool& m_useAccChannels;
    bool& m_useBipolarChannels;

    uint32_t& m_goodImpedanceLimit;
    uint32_t& m_badImpedanceLimit;
    uint32_t& m_triggers;

    DeviceSelection& m_deviceSelection;

    GtkComboBox* m_comboBoxPhysicalSampleRate = nullptr;
    GtkSpinButton* m_buttonGoodImpedanceLimit = nullptr;
    GtkSpinButton* m_buttonBadImpedanceLimit  = nullptr;

    GtkSpinButton* m_buttonNEEGChannels      = nullptr;
    GtkSpinButton* m_buttonNBipolar          = nullptr;
    GtkSpinButton* m_buttonNAUXChannels      = nullptr;
    GtkToggleButton* m_enableACCChannels     = nullptr;
    GtkToggleButton* m_enableBipolarChannels = nullptr;
    GtkComboBox* m_comboBoxSerialNumber      = nullptr;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
