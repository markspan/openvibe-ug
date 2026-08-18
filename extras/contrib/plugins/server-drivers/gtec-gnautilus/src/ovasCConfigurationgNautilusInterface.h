#pragma once

#if defined(TARGET_HAS_ThirdPartyGNEEDaccessAPI)

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>
#include <GDSClientAPI.h>
#include <GDSClientAPI_gNautilus.h>

#include <sstream>
#include <algorithm>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationgNautilusInterface
 * \author g.tec medical engineering GmbH (g.tec medical engineering GmbH)
 * \date Wed Aug 12 16:37:18 2015
 * \brief The CConfigurationgNautilusInterface handles the configuration dialog specific to the g.NEEDaccess device.
 *
 * TODO: details
 *
 * \sa CDrivergNautilusInterface
 */
class CConfigurationgNautilusInterface final : public CConfigurationBuilder
{
public:

    // you may have to add to your constructor some reference parameters
    // for example, a connection ID:
    //CConfigurationgNautilusInterface(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& rConnectionId);
    CConfigurationgNautilusInterface(IDriverContext& ctx, const char* gtkBuilderFilename, std::string& deviceSerial, int& inputSource, uint32_t& networkChannel,
                                     int& bandpassFilterIdx, int& notchFilterIdx, double& sensitivity, bool& digitalInputEnabled, bool& noiseReductionEnabled,
                                     bool& carEnabled, bool& accelerationDataEnabled, bool& counterEnabled, bool& linkQualityEnabled, bool& batteryLevelEnabled,
                                     bool& validationIndicatorEnabled, std::vector<uint16_t>& selectedChannels, std::vector<int>& bipolarChannels,
                                     std::vector<bool>& cars, std::vector<bool>& noiseReduction);

    bool preConfigure() override;
    bool postConfigure() override;

    //button callback functions
    void buttonChannelSettingsPressedCB();
    void buttonChannelSettingsApplyPressedCB();
    void buttonSensitivityFiltersPressedCB();
    void buttonSensitivityFiltersApplyPressedCB();
    bool getHardwareSettings();
    bool getChannelNames();
    bool getAvailableChannels();
    bool getFiltersForNewSamplingRate();
    void comboboxSampleRateChangedCB();
    void checkbuttonNoiseReductionChangedCB();
    void checkbuttonCARChangedCB();

protected:

    IDriverContext& m_driverCtx;
    std::string& m_deviceSerial;
    int& m_inputSource;
    uint32_t& m_networkChannel;
    int& m_bandpassFilterIdx;
    int& m_notchFilterIdx;
    double& m_sensitivity;
    bool& m_digitalInputEnabled;
    bool& m_noiseReductionEnabled;
    bool& m_carEnabled;
    bool& m_accelerationDataEnabled;
    bool& m_counterEnabled;
    bool& m_linkQualityEnabled;
    bool& m_batteryLevelEnabled;
    bool& m_validationIndicatorEnabled;
    std::vector<uint16_t>& m_selectedChannels;
    std::vector<int>& m_bipolarChannels;
    std::vector<bool>& m_cars;
    std::vector<bool>& m_noiseReduction;
    std::vector<int> m_comboBoxBandpassFilterIdx;
    std::vector<int> m_comboBoxNotchFilterIdx;
    std::vector<double> m_comboBoxSensitivityValues;
    std::vector<int> m_comboBoxInputSources;
    std::vector<size_t> m_comboBoxNetworkChannels;

private:

    /*
     * Insert here all specific attributes, such as a connection ID.
     * use references to directly modify the corresponding attribute of the driver
     * Example:
     */
    // uint32_t& m_connectionID;
    bool openDevice();
    bool closeDevice();
    GDS_HANDLE m_deviceHandle;
    GDS_RESULT m_gdsResult;
    std::vector<bool> m_availableChannels;
    char (*m_deviceNames)[DEVICE_NAME_LENGTH_MAX];
    bool m_connectionOpen = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGNEEDaccessAPI
