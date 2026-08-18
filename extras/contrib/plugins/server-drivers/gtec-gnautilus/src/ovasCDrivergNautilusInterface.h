#pragma once

#if defined(TARGET_HAS_ThirdPartyGNEEDaccessAPI)

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <GDSClientAPI.h>
#include <GDSClientAPI_gNautilus.h>
#include <vector>
#include <string>
#include <algorithm>
#include <Windows.h>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDrivergNautilusInterface
 * \author g.tec medical engineering GmbH (g.tec medical engineering GmbH)
 * \date Wed Aug 12 16:37:18 2015
 * \brief The CDrivergNautilusInterface allows the acquisition server to acquire data from a g.NEEDaccess device.
 *
 * TODO: details
 *
 * \sa CConfigurationgNautilusInterface
 */
class CDrivergNautilusInterface final : public IDriver
{
public:

    explicit CDrivergNautilusInterface(IDriverContext& ctx);
    ~CDrivergNautilusInterface() override { }
    const char* getName() override { return "g.tec g.Nautilus using g.NEEDaccess"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override;
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;

    // Replace this generic Header with any specific header you might have written
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 4;
    float* m_sample                = nullptr;
    uint32_t m_deviceIdx           = uint32_t(-1);
    uint32_t m_actualDeviceIdx     = 0;
    uint32_t m_bufferSize          = 0;
    size_t m_availableScans        = 0;
    float* m_Buffer                = nullptr;

    int m_notchFilterIdx    = -1;
    int m_bandPassFilterIdx = -1;

    double m_sensitivity              = 0;
    int m_inputSource                 = 0;
    uint32_t m_networkChannel         = 11;
    bool m_digitalInputEnabled        = true;
    bool m_noiseReductionEnabled      = false;
    bool m_carEnabled                 = false;
    bool m_accelerationDataEnabled    = true;
    bool m_counterEnabled             = true;
    bool m_linkQualityEnabled         = true;
    bool m_batteryLevelEnabled        = true;
    bool m_validationIndicatorEnabled = true;
    std::vector<uint16_t> m_selectedChannels;
    std::vector<int> m_bipolarChannels;
    std::vector<bool> m_noiseReduction;
    std::vector<bool> m_cars;
    uint32_t m_nAcquiredChannel = 32;

    GDS_HANDLE m_Device;
    GDS_RESULT m_gdsResult;
    GDS_GNAUTILUS_CONFIGURATION m_deviceCfg;
    std::string m_deviceSerial;
    uint32_t m_nDevice = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGNEEDaccessAPI
