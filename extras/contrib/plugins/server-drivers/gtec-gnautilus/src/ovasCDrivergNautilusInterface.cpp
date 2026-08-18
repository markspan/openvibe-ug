#if defined(TARGET_HAS_ThirdPartyGNEEDaccessAPI)

#include "ovasCDrivergNautilusInterface.h"
#include "ovasCConfigurationgNautilusInterface.h"

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace AcquisitionServer {

// global functions, event handles and variables
void OnDataReadyEventHandler(GDS_HANDLE handle, void* data);
HANDLE dataReadyEventHandle;

CDrivergNautilusInterface::CDrivergNautilusInterface(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_gNautilusInterface", m_driverCtx.getConfigurationManager()), m_Device(NULL)
{
    m_header.setSamplingFrequency(250);
    m_header.setChannelCount(32);

    // The following class allows saving and loading driver settings from the acquisition server .conf file
    m_settings.add("Header", &m_header);
    // To save your custom driver settings, register each variable to the SettingsHelper
    //m_settings.add("SettingName", &variable);
    m_settings.add("DeviceIndex", &m_deviceIdx);
    m_settings.add("InputSource", &m_inputSource);
    m_settings.add("NetworkChannel", &m_networkChannel);
    m_settings.add("Sensitivity", &m_sensitivity);
    m_settings.add("NotchFilterIndex", &m_notchFilterIdx);
    m_settings.add("BandPassFilterIndex", &m_bandPassFilterIdx);
    m_settings.add("DigitalInputEnabled", &m_digitalInputEnabled);
    m_settings.add("NoiseReductionEnabled", &m_noiseReductionEnabled);
    m_settings.add("CAREnabled", &m_carEnabled);
    m_settings.add("AccelerationEnabled", &m_accelerationDataEnabled);
    m_settings.add("CounterEnabled", &m_counterEnabled);
    m_settings.add("LinqQualityEnabled", &m_linkQualityEnabled);
    m_settings.add("BatteryLevelEnabled", &m_batteryLevelEnabled);
    m_settings.add("ValidationIndicatorEnabled", &m_validationIndicatorEnabled);

    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //

bool CDrivergNautilusInterface::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    if (nSamplePerSentBlock > 250)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Sample count per sent block cannot be higher than 250 samples for g.Nautilus\n";
        return false;
    }

    // initialize basic GDS functions before the first GDS function itself is called
    GDS_Initialize();

    // get number of channels actually acquired
    m_nAcquiredChannel = 0;
    for (size_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        if (find(m_selectedChannels.begin(), m_selectedChannels.end(), (i + 1)) != m_selectedChannels.end())
        {
            m_header.setChannelUnits(m_nAcquiredChannel, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
            m_nAcquiredChannel += 1;
        }
    }

    m_header.setChannelCount(m_nAcquiredChannel);

    // g.Nautilus provides some extra channel, add them to the channels count
    // and provide corresponding names
    uint32_t nAcquiredChannel = m_nAcquiredChannel;
    if (m_accelerationDataEnabled)
    {
        m_header.setChannelCount(nAcquiredChannel + 3);
        m_header.setChannelName(nAcquiredChannel, "CH_Accel_x");
        m_header.setChannelName(nAcquiredChannel + 1, "CH_Accel_y");
        m_header.setChannelName(nAcquiredChannel + 2, "CH_Accel_z");
        m_header.setChannelUnits(nAcquiredChannel, OVTK_UNIT_Meter_Per_Second_Squared, OVTK_FACTOR_Base);
        m_header.setChannelUnits(nAcquiredChannel + 1, OVTK_UNIT_Meter_Per_Second_Squared, OVTK_FACTOR_Base);
        m_header.setChannelUnits(nAcquiredChannel + 2, OVTK_UNIT_Meter_Per_Second_Squared, OVTK_FACTOR_Base);
        nAcquiredChannel = m_header.getChannelCount();
    }
    if (m_counterEnabled)
    {
        m_header.setChannelCount(nAcquiredChannel + 1);
        m_header.setChannelName(nAcquiredChannel, "CH_Counter");
        m_header.setChannelUnits(nAcquiredChannel,OVTK_UNIT_Dimensionless,OVTK_FACTOR_Base);
        nAcquiredChannel = m_header.getChannelCount();
    }
    if (m_linkQualityEnabled)
    {
        m_header.setChannelCount(nAcquiredChannel + 1);
        m_header.setChannelName(nAcquiredChannel, "CH_LQ");
        m_header.setChannelUnits(nAcquiredChannel,OVTK_UNIT_10_2_Percent,OVTK_FACTOR_Base);
        nAcquiredChannel = m_header.getChannelCount();
    }
    if (m_batteryLevelEnabled)
    {
        m_header.setChannelCount(nAcquiredChannel + 1);
        m_header.setChannelName(nAcquiredChannel, "CH_Battery");
        m_header.setChannelUnits(nAcquiredChannel,OVTK_UNIT_10_2_Percent,OVTK_FACTOR_Base);
        nAcquiredChannel = m_header.getChannelCount();
    }
    if (m_digitalInputEnabled)
    {
        m_header.setChannelCount(nAcquiredChannel + 1);
        m_header.setChannelName(nAcquiredChannel, "CH_Event");
        m_header.setChannelUnits(nAcquiredChannel,OVTK_UNIT_Dimensionless,OVTK_FACTOR_Base);
        nAcquiredChannel = m_header.getChannelCount();
    }
    if (m_validationIndicatorEnabled)
    {
        m_header.setChannelCount(nAcquiredChannel + 1);
        m_header.setChannelName(nAcquiredChannel, "CH_Valid");
        m_header.setChannelUnits(nAcquiredChannel,OVTK_UNIT_Dimensionless,OVTK_FACTOR_Base);
        nAcquiredChannel = m_header.getChannelCount();
    }

    // initialize connection and open connection handle and get serial of connected g.Nautilus
    GDS_DEVICE_CONNECTION_INFO* devicesInfo;
    size_t nDevices = 0;
    GDS_ENDPOINT hostEp;
    GDS_ENDPOINT localEp;
    uint8_t byteIP[4];
    char tempIP[16];

    byteIP[0] = 1;
    byteIP[1] = 0;
    byteIP[2] = 0;
    byteIP[3] = 127;

    _snprintf_s(tempIP, IP_ADDRESS_LENGTH_MAX, "%d.%d.%d.%d", byteIP[3], byteIP[2], byteIP[1], byteIP[0]);

    for (size_t i = 0; i < IP_ADDRESS_LENGTH_MAX; ++i)
    {
        hostEp.IpAddress[i]  = tempIP[i];
        localEp.IpAddress[i] = tempIP[i];
    }

    hostEp.Port  = 50223;
    localEp.Port = 50224;

    bool found  = false;
    m_gdsResult = GDS_GetConnectedDevices(hostEp, localEp, &devicesInfo, &nDevices);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    for (size_t i = 0; i < nDevices; ++i)
    {
        // if devices are in use they cannot be used for a new acquisition
        if ((devicesInfo[i].InUse) && (i < nDevices)) { continue; }
        // only one device can be used for data acquisition, as g.Nautilus cannot be synchronized
        if ((devicesInfo[i].InUse) && (devicesInfo[i].ConnectedDevicesLength > 1)) { continue; }
        GDS_DEVICE_INFO* info = devicesInfo[i].ConnectedDevices;
        if (info[0].DeviceType == GDS_DEVICE_TYPE_GNAUTILUS)
        {
            // check if device used for configuration is still available
            if (!strcmp(info[0].Name, m_deviceSerial.c_str()))
            {
                m_nDevice = 1;
                found     = true;
                break;
            }
        }
    }
    if (found == false)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "No g.Nautilus device found\n";
        return false;
    }

    char (*names)[DEVICE_NAME_LENGTH_MAX] = new char[1][DEVICE_NAME_LENGTH_MAX];
    bool openExclusively                  = true;
    BOOL isCreator;

    strncpy_s(names[0], m_deviceSerial.c_str(), DEVICE_NAME_LENGTH_MAX);

    // connect to device
    m_gdsResult = GDS_Connect(hostEp, localEp, names, m_nDevice, openExclusively, &m_Device, &isCreator);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Connected to device : " << m_deviceSerial << "\n";

    m_deviceCfg.SamplingRate           = m_header.getSamplingFrequency();
    m_deviceCfg.NumberOfScans          = nSamplePerSentBlock;
    m_deviceCfg.NetworkChannel         = m_networkChannel;
    m_deviceCfg.AccelerationData       = m_accelerationDataEnabled;
    m_deviceCfg.BatteryLevel           = m_batteryLevelEnabled;
    m_deviceCfg.CAR                    = m_carEnabled;
    m_deviceCfg.Counter                = m_counterEnabled;
    m_deviceCfg.DigitalIOs             = m_digitalInputEnabled;
    m_deviceCfg.LinkQualityInformation = m_linkQualityEnabled;
    m_deviceCfg.NoiseReduction         = m_noiseReductionEnabled;
    m_deviceCfg.Slave                  = 0;
    m_deviceCfg.ValidationIndicator    = m_validationIndicatorEnabled;
    m_deviceCfg.InputSignal            = GDS_GNAUTILUS_INPUT_SIGNAL(m_inputSource);

    for (size_t i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; ++i)
    {
        if (find(m_selectedChannels.begin(), m_selectedChannels.end(), (i + 1)) != m_selectedChannels.end()) { m_deviceCfg.Channels[i].Enabled = true; }
        else { m_deviceCfg.Channels[i].Enabled = false; }
        m_deviceCfg.Channels[i].Sensitivity = m_sensitivity;
        if (i < m_cars.size()) { m_deviceCfg.Channels[i].UsedForCar = m_cars[i]; }
        else { m_deviceCfg.Channels[i].UsedForCar = false; }
        if (i < m_noiseReduction.size()) { m_deviceCfg.Channels[i].UsedForNoiseReduction = m_noiseReduction[i]; }
        else { m_deviceCfg.Channels[i].UsedForNoiseReduction = false; }
        m_deviceCfg.Channels[i].BandpassFilterIndex = m_bandPassFilterIdx;
        m_deviceCfg.Channels[i].NotchFilterIndex    = m_notchFilterIdx;
        if (i < m_bipolarChannels.size()) { m_deviceCfg.Channels[i].BipolarChannel = m_bipolarChannels[i]; }
        else { m_deviceCfg.Channels[i].BipolarChannel = false; }
    }

    // Free memory allocated for list of connected devices by GDS_GetConnectedDevices
    m_gdsResult = GDS_FreeConnectedDevicesList(&devicesInfo, nDevices);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // Builds up a buffer to store acquired samples. This buffer
    // will be sent to the acquisition server later...
    m_sample = new float[nAcquiredChannel * m_deviceCfg.NumberOfScans];
    if (!m_sample)
    {
        delete [] m_sample;
        m_sample = nullptr;
        return false;
    }
    // set up data buffer for gds getdata function
    m_Buffer = new float[nAcquiredChannel * m_deviceCfg.NumberOfScans];
    if (!m_Buffer)
    {
        delete [] m_Buffer;
        m_Buffer = nullptr;
        return false;
    }
    m_bufferSize = nAcquiredChannel * m_deviceCfg.NumberOfScans;

    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    return true;
}

bool CDrivergNautilusInterface::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // if no device was found return
    if (m_nDevice != 1) { return false; }

    GDS_CONFIGURATION_BASE* config  = new GDS_CONFIGURATION_BASE[m_nDevice];
    config[0].Configuration         = &m_deviceCfg;
    config[0].DeviceInfo.DeviceType = GDS_DEVICE_TYPE_GNAUTILUS;
    strcpy_s(config[0].DeviceInfo.Name, m_deviceSerial.c_str());
    size_t nScan               = 0;
    size_t channelsPerDevice   = 0;
    size_t bufferSizeInSamples = 0;

    // set device configuration
    m_gdsResult = GDS_SetConfiguration(m_Device, config, m_nDevice);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // initialize data ready event and set data ready event handle
    dataReadyEventHandle = nullptr;
    dataReadyEventHandle = CreateEvent(nullptr, false, false, nullptr);
    m_gdsResult          = GDS_SetDataReadyCallback(m_Device, (GDS_Callback)OnDataReadyEventHandler, m_deviceCfg.NumberOfScans,NULL);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // set number of scans getdata function will return
    m_availableScans = m_deviceCfg.NumberOfScans;

    // ...
    // request hardware to start
    // sending data
    // ...
    // start acquisition
    m_gdsResult = GDS_StartAcquisition(m_Device);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }
    // start data stream from server
    m_gdsResult = GDS_StartStreaming(m_Device);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    m_gdsResult = GDS_GetDataInfo(m_Device, &nScan, nullptr, &channelsPerDevice, &bufferSizeInSamples);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    return true;
}

bool CDrivergNautilusInterface::loop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return true; }

    const CStimulationSet stimSet;

    if (m_driverCtx.isStarted())
    {
        const DWORD ret = WaitForSingleObject(dataReadyEventHandle, 5000);
        if (ret == WAIT_TIMEOUT)
        {
            // if data ready event is not triggered in 5000ms add a timeout handler
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "No data received in 5 seconds\n";
            return false;
        }
        // when data is ready call get data function with amount of scans to return
        m_gdsResult = GDS_GetData(m_Device, &m_availableScans, m_Buffer, m_bufferSize);
        if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
            return false;
        }

        // put data from receiving buffer to application buffer
        for (size_t i = 0; i < m_header.getChannelCount(); ++i)
        {
            for (size_t j = 0; j < m_nSamplePerSentBlock; ++j) { m_sample[i * m_nSamplePerSentBlock + j] = m_Buffer[j * (m_header.getChannelCount()) + i]; }
        }
    }

    // ...
    // receive samples from hardware
    // put them the correct way in the sample array
    // whether the buffer is full, send it to the acquisition server
    //...
    m_callback->setSamples(m_sample);

    // When your sample buffer is fully loaded,
    // it is advised to ask the acquisition server
    // to correct any drift in the acquisition automatically.
    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

    // ...
    // receive events from hardware
    // and put them the correct way in a CStimulationSet object
    //...
    m_callback->setStimulationSet(stimSet);

    return true;
}

bool CDrivergNautilusInterface::stop()
{
    if (!m_driverCtx.isConnected()) return false;
    if (!m_driverCtx.isStarted()) return false;

    // ...
    // request the hardware to stop
    // sending data
    // ...


    // stop streaming
    m_gdsResult = GDS_StopStreaming(m_Device);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    // stop data acquisiton
    m_gdsResult = GDS_StopAcquisition(m_Device);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    return true;
}

bool CDrivergNautilusInterface::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // ...
    // uninitialize hardware here
    // ...

    m_gdsResult = GDS_Disconnect(&m_Device);
    if (m_gdsResult.ErrorCode != GDS_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << m_gdsResult.ErrorMessage << "\n";
        return false;
    }

    delete [] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;

    // uninitialize basic GDS functions after last GDS function is called
    GDS_Uninitialize();

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Disconnected from device : " << m_deviceSerial << "\n";

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDrivergNautilusInterface::isConfigurable()
{
    return true; // change to false if your device is not configurable
}

bool CDrivergNautilusInterface::configure()
{
    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationgNautilusInterface config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-gNautilusInterface.ui",
                                            m_deviceSerial, m_inputSource, m_networkChannel, m_bandPassFilterIdx, m_notchFilterIdx, m_sensitivity,
                                            m_digitalInputEnabled, m_noiseReductionEnabled, m_carEnabled, m_accelerationDataEnabled, m_counterEnabled,
                                            m_linkQualityEnabled, m_batteryLevelEnabled, m_validationIndicatorEnabled, m_selectedChannels, m_bipolarChannels,
                                            m_cars, m_noiseReduction);

    if (!config.configure(m_header)) { return false; }
    m_settings.save();

    return true;
}

void OnDataReadyEventHandler(GDS_HANDLE handle, void* data)
{
    // signals data is ready on GDS acquisition buffer
    if (!SetEvent(dataReadyEventHandle))
    {
        // insert error handling if necessary
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGNEEDaccessAPI
