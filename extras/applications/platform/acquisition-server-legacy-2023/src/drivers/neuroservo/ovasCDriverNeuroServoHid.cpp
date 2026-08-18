/*
* NeuroServo driver for OpenViBE
*
* \author (NeuroServo)
* \date Wed Nov 23 00:24:00 2016
*
* \note This driver will not compile with VS2010 due to missing HID library. Use VS2013.
*
*/

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include "ovasCDriverNeuroServoHid.h"
#include "ovasCConfigurationNeuroServoHid.h"

#include <toolkit/ovtk_all.h>
#include <system/ovCTime.h>
#include <Windows.h>
#include <functional>

namespace OpenViBE {
namespace AcquisitionServer {

/*
  NeuroServo general infos
*/
#define NEUROSERVO_VID              uint16_t(0xC1C4)
#define NEUROSERVO_PID              uint16_t(0x8B25)
#define NEUROSERVO_DATA_SIZE        uint16_t(65)
#define NEUROSERVO_DRIVER_NAME      "NeuroServo"
#define NEUROSERVO_SENDDATA_BLOCK   uint32_t(1024)
#define ADC_TO_uVOLTS               0.0118

/*
General define
*/
#define DRIFTSTABILISATION_MAXNBSWITCH  10
#define DRIFTSTABILISATION_MINNBSWITCH  2
#define DRIFTSTABILISATION_MULTFACTOR   1.2

//___________________________________________________________________//
//                                                                   //

CDriverNeuroServoHid::CDriverNeuroServoHid(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_NeuroServoHid", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(2048);
    m_header.setChannelCount(1);
    m_header.setChannelUnits(0, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
    m_header.setChannelName(0, "FP1-FPz");

    // Set the Device basic infos
    m_vendorId          = NEUROSERVO_VID;
    m_productId         = NEUROSERVO_PID;
    m_dataSize          = NEUROSERVO_DATA_SIZE;
    m_driverName        = NEUROSERVO_DRIVER_NAME;
    m_isDeviceConnected = false;

    m_settings.add("Header", &m_header);
    m_settings.add("Vid", &m_vendorId);
    m_settings.add("Pid", &m_productId);
    m_settings.add("DataSize", &m_dataSize);
    m_settings.add("AutomaticShutdown", &m_automaticShutdown);
    m_settings.add("ShutdownOnDrvDisconnect", &m_bShutdownOnDriverDisconnect);
    m_settings.add("DeviceLightEnable", &m_bDeviceLightEnable);

    m_settings.load();
}

CDriverNeuroServoHid::~CDriverNeuroServoHid() {}

const char* CDriverNeuroServoHid::getName() { return m_driverName; }

//___________________________________________________________________//
//                                                                   //

bool CDriverNeuroServoHid::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    m_nSamplePerSentBlock = nSamplePerSentBlock;

    // Set the specific infos of the device
    m_oHidDevice.setHidDeviceInfos(m_vendorId, m_productId, m_dataSize);

    // Connect to the device
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_driverName << ": Connecting to the device.\n";
    if (!m_oHidDevice.connect()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_driverName << ": Connection failed.\n";
        return false;
    }

    // Device connection state
    m_isDeviceConnected = true;

    // Take into account the configuration of "Automatic Shutdown" and "Device Light Enable"
    deviceShutdownAndLightConfiguration();

    // Bind the callback methods
    m_oHidDevice.dataReceived   = std::bind(&CDriverNeuroServoHid::processDataReceived, this, std::placeholders::_1);
    m_oHidDevice.deviceDetached = std::bind(&CDriverNeuroServoHid::deviceDetached, this);
    m_oHidDevice.deviceAttached = std::bind(&CDriverNeuroServoHid::deviceAttached, this);

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_driverName << ": Connection succeded.\n";

    m_sendBlockRequiredTime  = CTime(m_header.getSamplingFrequency(), (nSamplePerSentBlock)).time();
    m_sendSampleRequiredTime = CTime(m_header.getSamplingFrequency(), 1).time();

    m_sample = new float[nSamplePerSentBlock];
    if (!m_sample) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not allocate memory for sample array\n";
        delete[] m_sample;
        m_sample = nullptr;
        return false;
    }
    m_bDeviceEpochDetected = false;
    m_isDeviceInitialized  = true;

    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverNeuroServoHid::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // Ensure that the device is connected
    if (!m_isDeviceConnected) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_driverName << ": Device is not connected.\n";
        return false;
    }

    m_bQueueOverflow  = false;
    m_bQueueUnderflow = false;

    // Build the data to be sent to the device
    BYTE data[65];
    data[0] = 0x00; // HID Report ID
    data[1] = 0x09; // Cmd
    data[2] = 0x01; // 0x01 (Reserved)
    data[3] = 0x01; // Start acquisition

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_driverName << ": Request acquisition to be started.\n";

    if (!m_oHidDevice.writeToDevice(data, m_dataSize)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_driverName << ": Failed to start acquisittion.\n";
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_driverName << ": Acquisiton started.\n";
    m_sampleIdxForSentBlock      = 0;
    m_timeStampLastSentBlock     = 0;
    m_nDriftSample               = 0;
    m_bDeviceEpochDetected       = false;
    m_driftAutoCorrectionDelay   = 0;
    m_nSwitchDrift               = 0;
    m_fDriftAutoCorrFactor       = 1.0;
    m_isDriftWasInEarlyDirection = false;
    m_sampleValue                = 0;

    // Set approximate lattency in regard to hardware implementation. Queue lattency 
    // should be adjusted but write_available() member is not accessible in current 
    // boost version.
    m_driverCtx.setInnerLatencySampleCount(-1024);

    return true;
}

bool CDriverNeuroServoHid::loop()
{
    if (!m_isDeviceInitialized || !m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    for (uint32_t i = 0; i < m_nSamplePerSentBlock; ++i) {
        if (!m_pBufferQueue.pop(m_sampleValue) && m_bQueueUnderflow == false && m_nSwitchDrift == DRIFTSTABILISATION_MAXNBSWITCH) {
            // We wait for stabilisation before warn
            m_bQueueUnderflow = true;
        }
        m_sample[i] = (m_sampleValue * float(ADC_TO_uVOLTS)); // N.B. Last sample value is sent if queue was empty
    }

    const int64_t currentDriftSampleCount = m_driverCtx.getDriftSampleCount();
    if (currentDriftSampleCount != 0) {
        // Drift in early direction
        if (currentDriftSampleCount > m_nDriftSample) {
            if (m_isDriftWasInEarlyDirection == false && m_nSwitchDrift < DRIFTSTABILISATION_MAXNBSWITCH) {
                m_nSwitchDrift++;
                m_isDriftWasInEarlyDirection = true;
                if (m_nSwitchDrift > DRIFTSTABILISATION_MINNBSWITCH) { m_fDriftAutoCorrFactor = m_fDriftAutoCorrFactor / float(DRIFTSTABILISATION_MULTFACTOR); }
            }
            m_driftAutoCorrectionDelay = m_driftAutoCorrectionDelay + int64_t(m_fDriftAutoCorrFactor * m_sendSampleRequiredTime);
        }
        // Drift in late direction
        if (currentDriftSampleCount < m_nDriftSample) {
            if (m_isDriftWasInEarlyDirection == true && m_nSwitchDrift < DRIFTSTABILISATION_MAXNBSWITCH) {
                m_nSwitchDrift++;
                m_isDriftWasInEarlyDirection = false;
                if (m_nSwitchDrift > DRIFTSTABILISATION_MINNBSWITCH) { m_fDriftAutoCorrFactor = m_fDriftAutoCorrFactor / float(DRIFTSTABILISATION_MULTFACTOR); }
            }
            m_driftAutoCorrectionDelay = m_driftAutoCorrectionDelay - int64_t(m_fDriftAutoCorrFactor * m_sendSampleRequiredTime);
        }
    }

    const uint64_t elapsedTime = System::Time::zgetTime() - m_timeStampLastSentBlock;
    if (m_sendBlockRequiredTime > elapsedTime) {
        // If we're early, sleep before sending. This code regulate the data drift
        // @fixme the code should not rely on zsleep() as its time precision can not be guaranteed; it can oversleep esp. on Windows + boost 1.58
        const uint64_t sleepTime = m_sendBlockRequiredTime - elapsedTime;
        System::Time::zsleep(uint64_t(sleepTime + m_driftAutoCorrectionDelay));
    }

    m_timeStampLastSentBlock = System::Time::zgetTime();
    m_callback->setSamples(m_sample);
    const int64_t autoCorrection = m_driverCtx.getSuggestedDriftCorrectionSampleCount();
    m_driverCtx.correctDriftSampleCount(autoCorrection);
    m_callback->setStimulationSet(m_stimSet);
    m_stimSet.clear();

    // We will apply driver time auto correction on next loop
    m_nDriftSample = currentDriftSampleCount;

    if (m_bQueueUnderflow) {
        m_bQueueUnderflow = false;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << m_driverName << ": Sample block has been skipped by driver!! Driver queue was empty\n";
    }
    return true;
}

bool CDriverNeuroServoHid::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    // Ensure that the device is connected
    if (!m_isDeviceConnected) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_driverName << ": Device is not connected.\n";
        return false;
    }

    // Build the data to be sent to the device
    BYTE data[65];
    data[0] = 0x00; // HID Report ID
    data[1] = 0x09; // Cmd
    data[2] = 0x01; // 0x01 (Reserved)
    data[3] = 0x00; // Stop acquisition

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_driverName << ": Request acquisition to be stopped.\n";

    if (!m_oHidDevice.writeToDevice(data, m_dataSize)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_driverName << ": Failed to stop acquisittion.\n";
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_driverName << ": Acquisiton stopped.\n";

    return true;
}

bool CDriverNeuroServoHid::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_isDeviceInitialized && m_isDeviceConnected) {
        if (m_bShutdownOnDriverDisconnect) {
            BYTE data[65];
            data[0] = 0x00; // HID Report ID
            data[1] = 0x16; // Cmd
            data[2] = 0x01; // 0x01 (Reserved)
            data[3] = 0x01; // Shutdown the device
            m_oHidDevice.writeToDevice(data, m_dataSize);
        }
        else {
            // Set device to normal user mode       

            BYTE data[65];
            data[0] = 0x00; // HID Report ID
            data[1] = 0x17; // Cmd
            data[2] = 0x01; // 0x01 (Reserved)
            data[3] = 0x01; // ask the device to switch off automatically
            m_oHidDevice.writeToDevice(data, m_dataSize);

            data[0] = 0x00; // HID Report ID
            data[1] = 0x18; // Cmd
            data[2] = 0x01; // 0x01 (Reserved)
            data[3] = 0x01; // Enable the device light
            m_oHidDevice.writeToDevice(data, m_dataSize);
        }
    }
    m_isDeviceConnected   = false;
    m_isDeviceInitialized = false;
    delete[] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverNeuroServoHid::configure()
{
    CConfigurationNeuroServoHid config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-NeuroServoHid.ui");

    // Set the current state of the "Automatic Shutdown" "Shutdown on driver disconnect" and "Device Light Enable"
    config.setRadioAutomaticShutdown(m_automaticShutdown);
    config.setRadioShutdownOnDriverDisconnect(m_bShutdownOnDriverDisconnect);
    config.setRadioDeviceLightEnable(m_bDeviceLightEnable);

    if (!config.configure(m_header)) { return false; }

    // Get the configuration from the ui
    m_automaticShutdown           = config.getAutomaticShutdownStatus();
    m_bShutdownOnDriverDisconnect = config.getShutdownOnDriverDisconnectStatus();
    m_bDeviceLightEnable          = config.getDeviceLightEnableStatus();

    // Save the settings
    m_settings.save();

    return true;
}

//___________________________________________________________________//
//                                                                   //
// NEUROSERVO SPECIFIC METHODS IMPLEMENTATION

void CDriverNeuroServoHid::processDataReceived(const BYTE data[])
{
    // Ensure the acquisition has started
    if (m_driverCtx.isStarted() && m_isDeviceInitialized) {
        if (data[1] == 0x03) {
            const uint32_t nElement = (data[3] + (data[4] << 8));
            const uint32_t index    = (data[5] + (data[6] << 8));

            if (index == 0) {
                // Queue lattency should be considered but write_available() member
                // is not accessible in current boost version.
                /*
                size_t NbItemInQueue = 1024 - m_pBufferQueue.write_available();
                if (uint32_t(NbItemInQueue) != m_nBufferQueue)
                {
                    m_driverCtx.setInnerLatencySampleCount(int64_t(m_nBufferQueue - uint32_t(NbItemInQueue)));
                    m_nBufferQueue = uint32_t(NbItemInQueue);
                }
                */
                m_nSamplesReceived     = 0;
                m_bDeviceEpochDetected = true;
            }
            if (m_bDeviceEpochDetected == true) {
                // Loop and process the data
                for (int i = (7); i < m_dataSize - 1; i = i + 2) {
                    if (m_nSamplesReceived < nElement) {
                        float pValue = float(float(data[i]) + float((data[i + 1]) << 8));
                        // Remove comp2
                        if (pValue > 32767) { pValue = pValue - 65536; }
                        if (!m_pBufferQueue.push(pValue)) { m_bQueueOverflow = true; }
                        m_sampleIdxForSentBlock++;

                        // If full epoch is received
                        if (m_sampleIdxForSentBlock == 1024) {
                            m_bDeviceEpochDetected  = false;
                            m_sampleIdxForSentBlock = 0;
                            if (m_bQueueOverflow && m_nSwitchDrift == DRIFTSTABILISATION_MAXNBSWITCH) // We wait for stabilisation before warn
                            {
                                m_bQueueOverflow = false;
                                m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << m_driverName <<
                                        ": Sample block has been skipped by driver!! Driver queue was full\n";
                            }
                        }
                        m_nSamplesReceived++;
                    }
                }
            }
        }
    }
}

void CDriverNeuroServoHid::deviceDetached()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_driverName << ": Device detached.\n";
    m_isDeviceConnected   = false;
    m_isDeviceInitialized = false;
}

void CDriverNeuroServoHid::deviceAttached()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_driverName << ": Device attached.\n";
    m_isDeviceConnected = true;
}

void CDriverNeuroServoHid::deviceShutdownAndLightConfiguration()
{
    BYTE data[65];
    data[0] = 0x00; // HID Report ID
    data[2] = 0x01; // 0x01 (Reserved)

    data[1] = 0x17; // Cmd
    if (m_automaticShutdown) {
        data[3] = 0x01; // ask the device to switch off automatically
        m_oHidDevice.writeToDevice(data, m_dataSize);
    }
    else {
        data[3] = 0x00; // ask the device not to switch off automatically
        m_oHidDevice.writeToDevice(data, m_dataSize);
    }

    data[1] = 0x18; // Cmd
    if (m_bDeviceLightEnable) {
        data[3] = 0x01; // Enable the device light
        m_oHidDevice.writeToDevice(data, m_dataSize);
    }
    else {
        data[3] = 0x00; // Disable the device light
        m_oHidDevice.writeToDevice(data, m_dataSize);
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
#endif // TARGET_OS_Windows
