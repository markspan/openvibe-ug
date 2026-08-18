/*
* NeuroServo driver for OpenViBE
*
* \author (NeuroServo)
* \date Wed Nov 23 00:24:00 2016
*
*/

#pragma once

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

// Provicde necessary methods to allow connection with HID device
#include "HidDeviceApi/HidDevice.h"

#include <boost/lockfree/spsc_queue.hpp>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverNeuroServoHid
 * \author  (NeuroServo)
 * \date Wed Nov 23 00:24:00 2016
 * \brief The CDriverNeuroServoHid allows the acquisition server to acquire data from a NeuroServo device.
 *
 * \sa CConfigurationNeuroServoHid
 */
class CDriverNeuroServoHid final : public IDriver
{
public:
    explicit CDriverNeuroServoHid(IDriverContext& ctx);
    ~CDriverNeuroServoHid() override;
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

    /* Methods implemented for the specific needs of NeuroServo.
       Execute the callback from the HID device services
    */
    void processDataReceived(const BYTE data[]);
    void deviceDetached();
    void deviceAttached();

private:
    // Control "Automatic Shutdown" and "Device Light Enable" based on user configuration
    void deviceShutdownAndLightConfiguration();


protected:
    SettingsHelper m_settings;
    IDriverCallback* m_callback = nullptr;
    CHeader m_header;
    float* m_sample = nullptr;
    CStimulationSet m_stimSet;

private:
    // Create a buffer queue for 1 sec data
    boost::lockfree::spsc_queue<float, boost::lockfree::capacity<2048>> m_pBufferQueue;

    // Device related infos
    HidDevice m_oHidDevice;
    uint16_t m_productId = 0;
    uint16_t m_vendorId  = 0;
    uint16_t m_dataSize  = 0;
    CString m_driverName;

    // Data processing related infos
    uint32_t m_nSamplePerSentBlock   = 0;
    uint32_t m_sampleIdxForSentBlock = 0;
    uint32_t m_nSamplesReceived      = 0;

    uint64_t m_timeStampLastSentBlock = 0;
    uint64_t m_sendBlockRequiredTime  = 0;
    uint64_t m_sendSampleRequiredTime = 0;
    uint64_t m_nSwitchDrift           = 0;

    int64_t m_nDriftSample             = 0;
    int64_t m_driftAutoCorrectionDelay = 0;

    float m_sampleValue          = 0;
    float m_fDriftAutoCorrFactor = 0;

    bool m_isDriftWasInEarlyDirection = false;
    bool m_bQueueOverflow             = false;
    bool m_bQueueUnderflow            = false;
    bool m_bDeviceEpochDetected       = false;

    // Configuration
    bool m_automaticShutdown           = false;
    bool m_bShutdownOnDriverDisconnect = true;
    bool m_bDeviceLightEnable          = false;
    bool m_isDeviceInitialized         = false;

    // Device connection state
    bool m_isDeviceConnected = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
#endif // TARGET_OS_Windows
