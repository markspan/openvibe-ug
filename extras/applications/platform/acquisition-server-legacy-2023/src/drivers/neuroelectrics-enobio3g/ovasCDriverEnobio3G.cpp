#if defined(TARGET_HAS_ThirdPartyEnobioAPI)

#include "ovasCDriverEnobio3G.h"
#include "ovasCConfigurationEnobio3G.h"
#include <string.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverEnobio3G::CDriverEnobio3G(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_Enobio3G", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(_ENOBIO_SAMPLE_RATE_);
    m_header.setChannelCount(32);

    // The following class allows saving and loading driver settings from the acquisition server .conf file
    m_settings.add("Header", &m_header);
    // To save your custom driver settings, register each variable to the SettingsHelper
    //m_settings.add("SettingName", &variable);
    m_settings.load();

    // register consumers for enobio data and enobio status
    // we register for data and status
    m_enobioDevice.registerConsumer(Enobio3G::ENOBIO_DATA, *this);
    //  m_enobioDevice.registerConsumer(Enobio3G::STATUS, this);
    // DONT Get the data from the accelerometer HACK: should ask through config dialog
    m_enobioDevice.activateAccelerometer(false);
    // set sampling rate of enobio device
    m_sampleRate = _ENOBIO_SAMPLE_RATE_;
    // allocate space for m_macAddres
    m_macAddress = new unsigned char[6];
}

CDriverEnobio3G::~CDriverEnobio3G()
{
    // Note: Device itself is closed in uninitialize()

    delete m_sample;
    delete m_macAddress;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverEnobio3G::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) return false;

    // open the BT connection to the device
    if (m_enobioDevice.openDevice(m_macAddress)) { m_nChannels = m_enobioDevice.numOfChannels(); }
    else { return false; }

    m_header.setChannelCount(m_nChannels);
    if (!m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) return false;

    for (size_t c = 0; c < m_nChannels; ++c) { m_header.setChannelUnits(c, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); }

    // Builds up a buffer to store
    // acquired samples. This buffer
    // will be sent to the acquisition
    // server later...
    // number of cycling buffers we will use
    m_nBuffers = 32;
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Need " << m_nBuffers << " buffers of " << m_header.getChannelCount() * nSamplePerSentBlock << " size for "
            << m_nChannels << " channels\n";
    m_sample = new float*[m_nBuffers];
    for (uint32_t i = 0; i < m_nBuffers; ++i)
    {
        // each buffer will be of length samplecountpersentblock, defined by the configuration interface
        m_sample[i] = new float[m_header.getChannelCount() * nSamplePerSentBlock];
    }
    m_newData = false;

    if (!m_sample)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Memory allocation error\n";
        delete [] m_sample;
        m_sample = nullptr;
        return false;
    }
    m_currentBuffer    = 0;
    m_lastBufferFilled = 0;
    m_bufHead          = 0;

    // ...
    // initialize hardware and get
    // available header information
    // from it
    // Using for example the connection ID provided by the configuration (m_connectionID)
    // ...

    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverEnobio3G::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    m_enobioDevice.startStreaming();
    return true;
}

/**
    Running loop used by OV engine to query for new data from device
*/
bool CDriverEnobio3G::loop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return true; }

    // query new data flag state
    bool newData;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        newData = m_newData;
    }

    // if new data flag is raised it means there's a buffer with new data ready to be submitted
    if (newData)
    {
        // submit new data on the buffer pointed by the lastbufferfilled variable
        m_callback->setSamples(m_sample[m_lastBufferFilled]);
        // lower new data flag
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_newData = false;
        }
        // When your sample buffer is fully loaded, 
        // it is advised to ask the acquisition server 
        // to correct any drift in the acquisition automatically.
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }
    return true;
}

bool CDriverEnobio3G::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    // ...
    // request the hardware to stop
    // sending data
    // Tell Enobio device to stop streaming EEG. 
    m_enobioDevice.stopStreaming();

    return true;
}

bool CDriverEnobio3G::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_sample)
    {
        for (uint32_t i = 0; i < m_nBuffers; ++i) { delete m_sample[i]; }
        delete [] m_sample;
        m_sample = nullptr;
    }

    m_callback = nullptr;
    // close BT connection with the Enobio device
    m_enobioDevice.closeDevice();

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverEnobio3G::configure()
{
    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationEnobio3G config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-Enobio3G.ui");

    if (!config.configure(m_header)) { return false; }
    m_settings.save();
    unsigned char* macAddress = config.getMacAddress();
    for (int i = 0; i < 6; ++i) { m_macAddress[i] = (unsigned char)*(macAddress + i); }

    return true;
}
/**
    Callback function that will be called by the Enobio API for each sample received from the device. 
*/
void CDriverEnobio3G::receiveData(const PData& data)
{
    ChannelData* receivedData = (ChannelData*)data.getData();
    // We'll need to iterate through channels instead of memcpy because we need 
    // to cast from int to float. will also convert to microvolts

    int* samples = receivedData->data();
    for (uint32_t i = 0; i < m_nChannels; ++i)
    {
        double sample                                                    = samples[i] / 1000.0;
        m_sample[m_currentBuffer][i * m_nSamplePerSentBlock + m_bufHead] = float(sample);
    }

    // mutex for writing header and new data flag
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_bufHead++;
        // if we already filled the current buffer we need to raise the new data flag
        // and cycle to the next buffer
        if (m_bufHead >= m_nSamplePerSentBlock)
        {
            // update the pointer to the last buffer filled
            m_lastBufferFilled = m_currentBuffer;
            // reset the buffer writing head to the beginning. 
            m_bufHead = 0;
            // we update pointer to the current buffer
            m_currentBuffer++;
            // if we are at the end of the buffers set, cycle to the first one
            if (m_currentBuffer >= m_nBuffers) { m_currentBuffer = 0; }
            // raise the flag to mark existence of new data to be submittted
            m_newData = true;
        }
    }
}

/**
    Callback from EnobioAPI to receive status data. 
    CURRENTLY NOT USED
*/
void CDriverEnobio3G::newStatusFromDevice(const PData& data) { }

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
