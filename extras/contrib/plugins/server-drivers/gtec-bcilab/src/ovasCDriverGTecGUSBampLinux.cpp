#if defined TARGET_HAS_ThirdPartyGUSBampCAPI_Linux

#include "ovasCDriverGTecGUSBampLinux.h"
#include "ovasCConfigurationGTecGUSBampLinux.h"

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverGTecGUSBampLinux::CDriverGTecGUSBampLinux(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_GTecGUSBampLinux", m_driverCtx.getConfigurationManager()),
      m_nSamplePerSentBlock(0), m_sampleSend(nullptr), m_sampleReceive(nullptr), m_sampleBuffer(nullptr), m_currentSample(0), m_currentChannel(0)
{
    // Default values
    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(16);

    m_config.ao_config = &m_analogOutConfig;

    // Configure some defaults so the settings are reasonable as soon as the driver loads and the user can tweak them from there

    // Configure the analog waveform to be created by the internal signal generator
    m_analogOutConfig.shape     = GT_ANALOGOUT_SINE;
    m_analogOutConfig.frequency = 1;
    m_analogOutConfig.amplitude = 0;
    m_analogOutConfig.offset    = 0;

    // This pretty much has to be GT_NOS_AUTOSET, don't know why, so says the documentation
    m_config.number_of_scans = GT_NOS_AUTOSET;
    // Disable the trigger line, digital io scan, slave mode and the shortcut
    m_config.enable_trigger_line = m_config.scan_dio = m_config.slave_mode = m_config.enable_sc = GT_FALSE;

    // Set the mode to just take readings
    m_config.mode = GT_MODE_NORMAL;

    // Set all the blocks A-D to use the common ground and reference voltages
    for (uint32_t i = 0; i < GT_USBAMP_NUM_GROUND; ++i)
    {
        m_config.common_ground[i]    = GT_TRUE;
        m_config.common_reference[i] = GT_TRUE;
    }

    // Configure each input
    for (unsigned char i = 0; i < GT_USBAMP_NUM_ANALOG_IN; ++i)
    {
        // Should be from 1 - 16, specifies which channel to observe as input i
        m_config.analog_in_channel[i] = i + 1;
        // Don't use any of the filters on channel i
        m_config.bandpass[i] = GT_FILTER_NONE;
        // Don't use any of the notch filters on channel i
        m_config.notch[i] = GT_FILTER_NONE;
        // Don't use any of the other channels for bi-polar derivation
        m_config.bipolar[i] = GT_BIPOLAR_DERIVATION_NONE;
    }

    // Now look for any connected devices. If any exist we'll set the name to the first one found
    char** devices = nullptr;
    size_t nDevice = 0;

    // Refresh and get the list of currently connnected devices
    GT_UpdateDevices();
    nDevice = GT_GetDeviceListSize();
    devices = GT_GetDeviceList();

    // If any devices were found at all, set the combo box to the first one listed
    if (nDevice) { m_deviceName = devices[0]; }

    GT_FreeDeviceList(devices, nDevice);

    // Now retrieve all those configs from the settings file if they are there to be found (don't need to worry about sample rate or channel number though since they're already in the header)
    m_settings.add("Header", &m_header);
    m_settings.add("DeviceName", static_cast<std::string*>(&m_deviceName));
    m_settings.add("Mode", static_cast<int*>(&m_config.mode));
    m_settings.add("EnableTrigger", static_cast<bool*>(&m_config.enable_trigger_line));
    m_settings.add("ScanDIO", static_cast<bool*>(&m_config.scan_dio));
    m_settings.add("SlaveMode", static_cast<bool*>(&m_config.slave_mode));
    m_settings.add("EnableShortcut", static_cast<bool*>(&m_config.enable_sc));
    m_settings.add("AnalogOutShape", static_cast<int*>(&m_analogOutConfig.shape));
    m_settings.add("AnalogOutFrequency", static_cast<int*>(&m_analogOutConfig.frequency));
    m_settings.add("AnalogOutAmplitude", static_cast<int*>(&m_analogOutConfig.amplitude));
    m_settings.add("AnalogOutOffset", static_cast<int*>(&m_analogOutConfig.offset));

    // Set all the blocks A-D to use the common ground and reference voltages
    for (uint32_t i = 0; i < GT_USBAMP_NUM_GROUND; ++i)
    {
        std::stringstream gndConfigName, configName;
        gndConfigName << "CommonGround" << i;
        configName << "CommonReference" << i;
        m_settings.add(gndConfigName.str().c_str(), static_cast<bool*>(&m_config.common_ground[i]));
        m_settings.add(configName.str().c_str(), static_cast<bool*>(&m_config.common_reference[i]));
    }

    // Configure each input
    for (uint32_t i = 0; i < GT_USBAMP_NUM_ANALOG_IN; ++i)
    {
        std::stringstream bandpassConfigName, notchConfigName, bipolarConfigName;
        bandpassConfigName << "Bandpass" << i;
        notchConfigName << "Notch" << i;
        bipolarConfigName << "Bipolar" << i;
        m_settings.add(bandpassConfigName.str().c_str(), static_cast<int*>(&m_config.bandpass[i]));
        m_settings.add(notchConfigName.str().c_str(), static_cast<int*>(&m_config.notch[i]));
        m_settings.add(bipolarConfigName.str().c_str(), static_cast<int*>(&m_config.bipolar[i]));
    }

    // This restores saved settings if any, such as sampling rate
    m_settings.load();

    // Set the sampling rate that may have been changed by load
    m_config.sample_rate = m_header.getSamplingFrequency();

    // Number of channels that may have been changed by load
    m_config.num_analog_in = m_header.getChannelCount();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGTecGUSBampLinux::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) return false;
    if (!m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) return false;

    // If the scan digital inputs flag is set, the API will return one extra channel outside of the analog data requested, so we need to match that on the header
    if (m_config.scan_dio == GT_TRUE)
    {
        m_header.setChannelCount(m_config.num_analog_in + 1);
        m_header.setChannelName(m_config.num_analog_in, "Digital");
    }

    // Allocate buffers for...

    // Sending to OpenViBE
    m_sampleSend = new float[m_header.getChannelCount() * nSamplePerSentBlock];
    // Receiving from the hardware,
    m_sampleReceive = new float[m_header.getChannelCount() * nSamplePerSentBlock];
    // Storing the data so we pass it between the two threads - we're using the recommended buffer size put out by gtec, which is enormous
    m_sampleBuffer = new float[GT_USBAMP_RECOMMENDED_BUFFER_SIZE / sizeof(float)];

    // Set up the queue to help pass the data out of the hardware thread
    m_sampleQueue.SetBuffer(m_sampleBuffer, m_header.getChannelCount() * m_header.getSamplingFrequency() / 8);

    // If any of that allocation fails then give up. Not sure what setting it all to NULL is for, but we'll go with it.
    if (!m_sampleSend || !m_sampleReceive || !m_sampleBuffer)
    {
        delete[] m_sampleSend;
        delete[] m_sampleReceive;
        delete[] m_sampleBuffer;
        m_sampleSend = m_sampleReceive = m_sampleBuffer = nullptr;

        return false;
    }

    // Apparently this causes the API to print debug info to the console, I'm yet to see any though
    GT_ShowDebugInformation(GT_TRUE);

    // Try to open the device with the configured name, let the user know how it goes
    if (!GT_OpenDevice(m_deviceName.c_str()))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open device: " << m_deviceName << "\n";
        return false;
    }

    if (!GT_SetConfiguration(m_deviceName.c_str(), &m_config))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not apply configuration to device: " << m_deviceName << "\n";
        return false;
    }

    GT_SetDataReadyCallBack(m_deviceName.c_str(), &OnDataReady, static_cast<void*>(this));

    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverGTecGUSBampLinux::start()
{
    if (!m_driverCtx.isConnected()) return false;
    if (m_driverCtx.isStarted()) return false;

    // ...
    // request hardware to start
    // sending data
    // ...

    // Need to reset these in case the device is stopped mid-sample and then started again
    m_currentChannel = m_currentSample = 0;

    GT_StartAcquisition(m_deviceName.c_str());

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Acquisition Started\n";

    return true;
}

// So when the gtec buffer grows larger than a send buffer, copy it all to a send buffer sized array, then copy it into the actual send buffer one by one.
bool CDriverGTecGUSBampLinux::loop()
{
    if (!m_driverCtx.isConnected()) return false;
    if (!m_driverCtx.isStarted()) return true;

    const CStimulationSet stimSet;

    // while there's new data available on the queue
    while (m_sampleQueue.Avail())
    {
        // take it off and put it in the appropriate element in the outgoing buffer
        m_sampleQueue.Get(m_sampleSend + m_currentChannel * m_nSamplePerSentBlock + m_currentSample, 1);

        // Increment the current channel
        m_currentChannel++;

        // If the current channel reaches the channel count then move to the next sample
        if (m_currentChannel == m_header.getChannelCount())
        {
            m_currentChannel = 0;
            m_currentSample++;
        }

        // If the sample count reaches the number per sent block, then send it and start again
        if (m_currentSample == m_nSamplePerSentBlock)
        {
            m_callback->setSamples(m_sampleSend); // it looks as if this copies the buffer, so we're free modify it as soon as it executes

            // When your sample buffer is fully loaded,
            // it is advised to ask the acquisition server
            // to correct any drift in the acquisition automatically.
            m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

            // ...
            // receive events from hardware
            // and put them the correct way in a CStimulationSet object
            //...
            m_callback->setStimulationSet(stimSet);

            m_currentSample = 0;
        }
    }

    return true;
}

bool CDriverGTecGUSBampLinux::stop()
{
    if (!m_driverCtx.isConnected()) return false;
    if (!m_driverCtx.isStarted()) return false;

    // ...
    // request the hardware to stop
    // sending data
    // ...
    GT_StopAcquisition(m_deviceName.c_str());
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Acquisition Stopped";

    return true;
}

bool CDriverGTecGUSBampLinux::uninitialize()
{
    if (!m_driverCtx.isConnected()) return false;
    if (m_driverCtx.isStarted()) return false;

    GT_CloseDevice(m_deviceName.c_str());

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Closed Device: " << m_deviceName << "\n";

    // ...
    // uninitialize hardware here
    // ...
    m_sampleQueue.SetBuffer(nullptr, 0);

    delete[] m_sampleSend;
    delete[] m_sampleBuffer;
    delete[] m_sampleReceive;

    m_sampleSend = m_sampleReceive = m_sampleBuffer = nullptr;

    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverGTecGUSBampLinux::isConfigurable()
{
    return true; // change to false if your device is not configurable
}

bool CDriverGTecGUSBampLinux::configure()
{
    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationGTecGUSBampLinux config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-GTecGUSBampLinux.ui", &m_deviceName, &m_config);

    if (!config.configure(m_header)) { return false; }

    m_header.setChannelCount(m_config.num_analog_in);
    m_header.setSamplingFrequency(m_config.sample_rate);

    m_settings.save();

    return true;
}

/*void AcquisitionServer::OnDataReady(void *param)
{
    // Like the 'this' pointer, but for a friend function
    CDriverGTecGUSBampLinux *that = (CDriverGTecGUSBampLinux*)param;

    // This is pretty tricky to know in advance, the API decides how many values to spit out depnding on a few factors it seems.
    // We'll allocate a reasonble buffer and call GT_GetData as many times as is necessary
    while(size_t nSamplesToRead = GT_GetSamplesAvailable(that->m_deviceName.c_str()))
    {
        // If there are more samples than will fit in the buffer, just get as many as possible and we can get the rest next iteration
        if(nSamplesToRead > CDriverGTecGUSBampLinux::ReceiveBufferSize * sizeof(float))
            nSamplesToRead = CDriverGTecGUSBampLinux::ReceiveBufferSize * sizeof(float);

        // Get the data -- TODO: rewrite this algorithm such that we can copy directly from GT_GetData into the buffer read in the loop() function - is this a bug?? Maybe, but probably not since the calibration mode was always perfect
        GT_GetData(that->m_deviceName.c_str(), reinterpret_cast<unsigned char*>(that->m_sampleReceive), nSamplesToRead);

        // Put it on the sample queue
        that->m_sampleQueue.Put(that->m_sampleReceive, nSamplesToRead / sizeof(float));
    }
}*/

void AcquisitionServer::OnDataReady(void* param)
{
    // Like the 'this' pointer, but for a friend function
    CDriverGTecGUSBampLinux* that = static_cast<CDriverGTecGUSBampLinux*>(param);

    // This is pretty tricky to know in advance, the API decides how many values to spit out depnding on a few factors it seems.
    // We'll allocate a reasonble buffer and call GT_GetData as many times as is necessary
    while (size_t samplesToRead = GT_GetSamplesAvailable(that->m_deviceName.c_str()))
    {
        // If there are more samples than will fit in the buffer, just get as many as possible and we can get the rest next iteration
        if (samplesToRead > that->m_sampleQueue.FreeContiguous() * sizeof(float)) samplesToRead = that->m_sampleQueue.FreeContiguous() * sizeof(float);

        // Get the data and put it directly onto the queue
        GT_GetData(that->m_deviceName.c_str(), reinterpret_cast<unsigned char*>(that->m_sampleQueue.NextFreeAddress()), samplesToRead);

        // Pad the queue so it recognises how much data was just added to it
        that->m_sampleQueue.Pad(samplesToRead / sizeof(float));
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGUSBampCAPI_Linux
