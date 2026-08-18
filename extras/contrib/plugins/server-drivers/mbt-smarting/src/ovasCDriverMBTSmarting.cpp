#include "ovasCDriverMBTSmarting.h"
#include "ovasCConfigurationMBTSmarting.h"

#include <toolkit/ovtk_all.h>

#include <string>
#include <sstream>

namespace OpenViBE {
namespace AcquisitionServer {

//___________________________________________________________________//
//                                                                   //

CDriverMBTSmarting::CDriverMBTSmarting(IDriverContext& ctx)
    : IDriver(ctx)
      , m_settings("AcquisitionServer_Driver_MBTSmarting", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(500);
    m_header.setChannelCount(27);

    // The following class allows saving and loading driver settings from the acquisition server .conf file
    m_settings.add("Header", &m_header);
    // To save your custom driver settings, register each variable to the SettingsHelper
    m_settings.add("ConnectionID", &m_connectionID);
    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverMBTSmarting::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    // Builds up a buffer to store
    // acquired samples. This buffer
    // will be sent to the acquisition
    // server later...
    m_sample = new float[m_header.getChannelCount()];
    if (!m_sample)
    {
        delete [] m_sample;
        m_sample = nullptr;
        return false;
    }

    // ...
    // initialize hardware and get
    // available header information
    // from it
    // Using for example the connection ID provided by the configuration (m_connectionID)
    // ...

    m_pSmartingAmp.reset(new SmartingAmp);

    std::stringstream port_ss;
#ifdef TARGET_OS_Windows
    port_ss << "COM" << m_connectionID;
#elif defined TARGET_OS_Linux
        port_ss << "/dev/rfcomm" << m_connectionID;
#endif

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Attempting to Connect to Device at : " << port_ss.str() << "\n";

    std::string port(port_ss.str());
    const bool connected = m_pSmartingAmp->connect(port);
    if (connected)
    {
        // set sampling frequency
        switch (m_header.getSamplingFrequency())
        {
            case 250: m_pSmartingAmp->send_command(FREQUENCY_250);
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Setting the sampling frequency at " << 250 << "\n";
                break;
            case 500: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Setting the sampling frequency at " << 500 << "\n";
                m_pSmartingAmp->send_command(FREQUENCY_500);
                break;
            default: m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Only sampling frequencies 250 and 500 are supported\n";
                return false;
        }

        // Declare channel units
        for (uint32_t c = 0; c < 24; ++c)
        {
            m_header.setChannelUnits(c, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);         // signal channels
        }
        m_header.setChannelUnits(24, OVTK_UNIT_Degree_Per_Second, OVTK_FACTOR_Base); // gyroscope outputs
        m_header.setChannelUnits(25, OVTK_UNIT_Degree_Per_Second, OVTK_FACTOR_Base);
        m_header.setChannelUnits(26, OVTK_UNIT_Degree_Per_Second, OVTK_FACTOR_Base);

        m_header.setChannelName(24, "Gyro 1");
        m_header.setChannelName(25, "Gyro 2");
        m_header.setChannelName(26, "Gyro 3");

        // Saves parameters
        m_callback            = &callback;
        m_nSamplePerSentBlock = nSamplePerSentBlock;

        return true;
    }

    return false;
}

bool CDriverMBTSmarting::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // ...
    // request hardware to start
    // sending data
    // ...

    m_pSmartingAmp->send_command(ON);
    m_byteArray.clear();

    sample_number = 1;
    latency       = 1;

    return true;
}

bool CDriverMBTSmarting::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    const CStimulationSet stimSet;

    // ...
    // receive samples from hardware
    // put them the correct way in the sample array
    // whether the buffer is full, send it to the acquisition server
    //...

    unsigned char* receiveBuffer = new unsigned char[MAX_PACKAGE_SIZE];

    const int readed = m_pSmartingAmp->read(receiveBuffer, MAX_PACKAGE_SIZE);

    for (int i = 0; i < readed; ++i)
    {
        if (!m_byteArray.empty())
        {
            m_byteArray.push_back(receiveBuffer[i]);
            if (m_byteArray.size() == 83)
            {
                if (m_byteArray[82] == '<')
                {
                    if (sample_number % 5000 == 0)
                    {
                        sample_number = 1;

                        if (m_driverCtx.getDriftSampleCount() < 2)
                        {
                            m_sample = m_pSmartingAmp->convert_data(m_byteArray);
                            m_callback->setSamples(m_sample, 1);
                            m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
                        }
                    }
                    else
                    {
                        sample_number++;

                        m_sample = m_pSmartingAmp->convert_data(m_byteArray);
                        m_callback->setSamples(m_sample, 1);
                        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
                    }
                }

                m_byteArray.clear();
            }
        }

        if (m_byteArray.empty() && receiveBuffer[i] == '>') { m_byteArray.push_back(receiveBuffer[i]); }
    }

    if (latency == 300)
    {
        latency = 0;
        m_driverCtx.setInnerLatencySampleCount(-m_driverCtx.getDriftSampleCount());
    }
    else { if (latency != 0) { latency++; } }

    // ...
    // receive events from hardware
    // and put them the correct way in a CStimulationSet object
    //...
    m_callback->setStimulationSet(stimSet);

    return true;
}

bool CDriverMBTSmarting::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    // ...
    // request the hardware to stop
    // sending data
    // ...
    m_driverCtx.setInnerLatencySampleCount(0);
    m_pSmartingAmp->off();

    return true;
}

bool CDriverMBTSmarting::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // ...
    // uninitialize hardware here
    // ...

    m_pSmartingAmp->disconnect();

    delete [] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverMBTSmarting::configure()
{
    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationMBTSmarting config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-MBTSmarting.ui",
                                     m_connectionID);

    if (!config.configure(m_header)) { return false; }
    m_settings.save();

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
