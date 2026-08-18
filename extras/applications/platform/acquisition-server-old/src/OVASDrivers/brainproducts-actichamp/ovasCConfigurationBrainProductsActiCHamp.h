///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#pragma once

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK

#include "../ovasCConfigurationBuilder.h"
#include "../brainproducts-base/CDriverBrainProductsBase.h"

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {


struct BrainProductsAmp {
    std::string id;
    uint32_t availableModules;
};

class CConfigurationBrainProductsActiCHamp final : public CConfigurationBuilder
{
public:
    CConfigurationBrainProductsActiCHamp(const char* gtkBuilderFilename, DeviceSelection& deviceSelection,
                                         int32_t& activeShieldGain, uint32_t& nEEGChannels, bool& useAuxChannels,
                                         uint32_t& goodImpedanceLimit, uint32_t& badImpedanceLimit);

    bool preConfigure() override;
    bool postConfigure() override;

    void comboBoxDeviceChangedCB();
    void labelSamplingRateChangedCB();

protected:
    size_t m_selectedDeviceIndex;

    int32_t& m_activeShieldGain;
    uint32_t& m_nEEGChannels;
    bool& m_useAuxChannels;
    uint32_t& m_goodImpedanceLimit;
    uint32_t& m_badImpedanceLimit;
    DeviceSelection& m_deviceSelection;

    GtkComboBox* m_comboBoxDeviceId           = nullptr;
    GtkComboBox* m_comboBoxSampleRate         = nullptr;
    GtkComboBox* m_comboBoxSubSampleDivisor   = nullptr;
    GtkLabel* m_labelNominalSamplingFrequency = nullptr;
    GtkSpinButton* m_buttonActiveShieldGain   = nullptr;
    GtkSpinButton* m_buttonChannelsEnabled    = nullptr;
    GtkToggleButton* m_buttonUseAuxChannels   = nullptr;
    GtkSpinButton* m_buttonGoodImpedanceLimit = nullptr;
    GtkSpinButton* m_buttonBadImpedanceLimit  = nullptr;

    template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    void resetComboBox(GtkComboBox* comboBox, std::vector<T>& values);
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
