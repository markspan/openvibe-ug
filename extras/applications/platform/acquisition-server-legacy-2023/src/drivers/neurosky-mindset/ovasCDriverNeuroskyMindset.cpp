#if defined TARGET_HAS_ThirdPartyThinkGearAPI

#include "ovasCDriverNeuroskyMindset.h"
#include "ovasCConfigurationNeuroskyMindset.h"

#include <sstream>
#include <system/ovCTime.h>

#include <thinkgear.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverNeuroskyMindset::CDriverNeuroskyMindset(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_NeuroSkyMindSet", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(512); // raw signal sampling frequency, from the official documentation.
    // CHANNEL COUNT
    m_header.setChannelCount(1); // one channel on the forhead
    m_header.setChannelName(0, "Electrode");

    m_connectionID = -1;

    m_comPort = OVAS_MINDSET_INVALID_COM_PORT;

    m_eSenseChannels       = false;
    m_bandPowerChannels    = false;
    m_blinkStimulations    = false;
    m_blinkStrengthChannel = false;

    m_settings.add("Header", &m_header);
    m_settings.add("ComPort", &m_comPort);
    m_settings.add("ESenseChannels", &m_eSenseChannels);
    m_settings.add("BandPowerChannels", &m_bandPowerChannels);
    m_settings.add("Stimulations", &m_blinkStimulations);
    m_settings.add("StrengthChannel", &m_blinkStrengthChannel);
    m_settings.load();

#if !defined(TG_DATA_BLINK_STRENGTH)
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Mindset Driver: This version does not have eyeblink detection support\n";
    m_blinkStimulations    = false;
    m_blinkStrengthChannel = false;
#endif
}

//___________________________________________________________________//
//                                                                   //

bool CDriverNeuroskyMindset::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Mindset Driver: INIT called.\n";
    if (m_driverCtx.isConnected())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Mindset Driver: Driver already initialized.\n";
        return false;
    }

    /*
    11 channels
    -----
    eSense values:
    TG_DATA_ATTENTION
    TG_DATA_MEDITATION
    
    Raw EEG data:
    TG_DATA_RAW     (512 Hz sampling frequency)
    
    Power in defined frequency bands:
    TG_DATA_DELTA   (0.5 - 2.75 Hz)
    TG_DATA_THETA   (3.5 - 6.75 Hz)
    TG_DATA_ALPHA1  (7.5 - 9.25 Hz)
    TG_DATA_ALPHA2  (10 - 11.75 Hz)
    TG_DATA_BETA1   (13 - 16.75 Hz)
    TG_DATA_BETA2   (18 - 29.75 Hz)
    TG_DATA_GAMMA1  (31 - 39.75 Hz)
    TG_DATA_GAMMA2  (41 - 49.75 Hz)

    Since MindSet Development Kit 2.1:
    TG_DATA_BLINK_STRENGTH (1-255)
    */

    // the tokens allow user to use all data coming from headset. 
    // Raw EEG is sampled at 512Hz, other data are sampled around 1Hz 
    // so the driver sends these data at 512 Hz, changing value every seconds (we obtain a square signal)
    // The Blink Strength can be viewed as spike signal, sampled at 512 Hz.
    // The Blinks can be viewed as an OpenViBE stimulation OVTK_GDF_Eye_Blink

    m_header.setChannelCount(1); // one channel on the forhead
    m_header.setChannelName(0, "Electrode");

    if (m_eSenseChannels) { m_header.setChannelCount(m_header.getChannelCount() + 2); }
    if (m_bandPowerChannels) { m_header.setChannelCount(m_header.getChannelCount() + 8); }
    if (m_blinkStrengthChannel) { m_header.setChannelCount(m_header.getChannelCount() + 1); }

    // NAMES
    uint32_t idx = 1;

    if (m_eSenseChannels)
    {
        m_header.setChannelName(idx++, "Attention");
        m_header.setChannelName(idx++, "Meditation");
    }
    if (m_bandPowerChannels)
    {
        m_header.setChannelName(idx++, "Delta");
        m_header.setChannelName(idx++, "Theta");
        m_header.setChannelName(idx++, "Low Alpha");
        m_header.setChannelName(idx++, "High Alpha");
        m_header.setChannelName(idx++, "Low Beta");
        m_header.setChannelName(idx++, "High Beta");
        m_header.setChannelName(idx++, "Low Gamma");
        m_header.setChannelName(idx++, "Mid Gamma");
    }
    // spikes for blink strength
    if (m_blinkStrengthChannel) { m_header.setChannelName(idx++, "Blink Strength"); }

    if (!m_header.isChannelCountSet()
        || !m_header.isSamplingFrequencySet())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Mindset Driver: Channel count or frequency not set.\n";
        return false;
    }

    // Builds up a buffer to store acquired samples. This buffer will be sent to the acquisition server later.
    m_sample = new float[m_header.getChannelCount() * nSamplePerSentBlock];
    if (!m_sample)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Mindset Driver: Samples allocation failed.\n";
        return false;
    }


    /* Print driver version number */
    const int dllVersion = TG_GetVersion();
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "ThinkGear DLL version: " << dllVersion << "\n";
    if (dllVersion > 7) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Eye blink detection is possible. \n"; }
    else { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Eye blink detection NOT possible. Please use MindSet Dev. Tools v2.1 or sup. \n"; }

    // if no com port was selected with the property dialog window
    if (m_comPort == OVAS_MINDSET_INVALID_COM_PORT)
    {
        // try the com ports. @NOTE almost duplicate code in CConfigurationMindSet
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Scanning COM ports 1 to 16...\n";
        for (uint32_t i = 1; i < 16 && m_comPort == OVAS_MINDSET_INVALID_COM_PORT; ++i)
        {
            /* Get a new connection ID handle to ThinkGear API */
            int connectionId = TG_GetNewConnectionId();
            if (connectionId < 0)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "The driver was unable to connect to the ThinkGear Communication Driver.\n";
                return false;
            }
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "ThinkGear Connection ID is: " << connectionId << ".\n";

            /* Attempt to connect the connection ID handle to serial port */
            std::stringstream ss;
            ss << "\\\\.\\COM" << i;
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Trying port [" << ss.str() << "]\n";
            int errCode = TG_Connect(connectionId, ss.str().c_str(), TG_BAUD_9600, TG_STREAM_PACKETS);
            if (errCode >= 0)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Connection available on port " << ss.str();

                const uint32_t startTime = System::Time::getTime();
                const uint32_t timeToTry = 3000; // ms

                // With e.g. MindWave Mobile, errors do not mean that the operation couldn't succeed in the future, so we ask for a packet optimistically for a while.
                bool comPortFound = false;
                while (!comPortFound && (System::Time::getTime() - startTime) < timeToTry)
                {
                    //we try to read one packet to check the connection.
                    errCode = TG_ReadPackets(connectionId, 1);
                    if (errCode >= 0)
                    {
                        m_driverCtx.getLogManager() << " - Status: OK\n";
                        m_comPort    = i;
                        comPortFound = true;
                    }
                    else { System::Time::sleep(1); }
                }
                if (!comPortFound)
                {
                    m_driverCtx.getLogManager() << " - Tried for " << timeToTry / 1000 << " seconds, gave up.\n";
                    if (errCode == -1) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Last TG_ReadPackets error: -1, Invalid connection ID\n"; }
                    else if (errCode == -2) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Last TG_ReadPackets error: -2, 0 bytes on the stream\n"; }
                    else if (errCode == -3) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Last TG_ReadPackets error: -3, I/O error occurred\n"; }
                    else { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Last TG_ReadPackets error: " << errCode << ", Unknown\n"; }
                }
            }
            else { m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "TG_Connect() returned error " << errCode << "\n"; }
            // free the connection to ThinkGear API
            // We use FreeConnection() only as doing a TG_Disconnect()+TG_FreeConnection() pair can cause first-chance exceptions on visual studio & MindWave Mobile for some reason.
            TG_FreeConnection(connectionId);
        }
    }

    if (m_comPort == OVAS_MINDSET_INVALID_COM_PORT)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "The driver was unable to find any valid device on serial port COM1 to COM16.\n";
        return false;
    }

    //__________________________________
    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverNeuroskyMindset::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Mindset Driver: START called.\n";
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_comPort == OVAS_MINDSET_INVALID_COM_PORT)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "No valid Serial COM port detected.\n";
        return false;
    }

    /* Get a connection ID handle to ThinkGear */
    m_connectionID = TG_GetNewConnectionId();
    if (m_connectionID < 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Can't connect to ThinkGear Communication Driver (error code " << m_connectionID << ").\n";
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "ThinkGear Communication ID is: " << m_connectionID << ".\n";

    /* Attempt to connect the connection ID handle to serial port */
    const std::string tmp = "\\\\.\\COM" + std::to_string(m_comPort);
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Trying to connect ThinkGear driver to Serial Port [" << tmp << "]\n";
    const int errCode = TG_Connect(m_connectionID, tmp.c_str(), TG_BAUD_9600, TG_STREAM_PACKETS);
    if (errCode < 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "The ThinkGear driver was unable to connect to serial port [" << tmp << "] (error code "
                << errCode << ").\n";
        return false;
    }

#if defined(TG_DATA_BLINK_STRENGTH)
    if(m_blinkStimulations || m_blinkStrengthChannel) { TG_EnableBlinkDetection(m_connectionID, 1); }
#endif

    return true;
}

bool CDriverNeuroskyMindset::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }

    if (m_driverCtx.isStarted())
    {
        uint32_t receivedSamples = 0;
        uint32_t errorCount      = 0;

        CStimulationSet stimSet;

        while (receivedSamples < m_nSamplePerSentBlock)
        {
            /* Attempt to read 1 Packet of data from the connection (numPackets = -1 means all packets) */
            const int errorCode = TG_ReadPackets(m_connectionID, 1);

            if (errorCode == 1)// we asked for 1 packet and received 1
            {
                // Got a packet, reset the error count
                errorCount = 0;

                /* If raw value has been updated by TG_ReadPackets()... */
                if (TG_GetValueStatus(m_connectionID, TG_DATA_RAW) != 0)
                {
                    float value               = float(TG_GetValue(m_connectionID, TG_DATA_RAW));
                    m_sample[receivedSamples] = value;
                    receivedSamples++;
                }

                //checking the signal quality
                //if it has been updated...
                if (TG_GetValueStatus(m_connectionID, TG_DATA_POOR_SIGNAL) != 0)
                {
                    const float quality = float(TG_GetValue(m_connectionID, TG_DATA_POOR_SIGNAL));

                    // Special warning for value 200 (no contact with electrode)
                    // Noise warning after 25% contamination.
                    if (quality == 200)
                    {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(" << ++m_nWarning
                                << ") Poor Signal detected (electrode not in contact with the forehead)\n";
                    }
                    else if (quality > 25)
                    {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(" << ++m_nWarning << ") Poor Signal detected (noise at "
                                << (1 - (quality / 200.0F)) * 100 << "%)\n";
                    }
                    else
                    {
                        if (m_nWarning != 0) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Signal Quality acceptable - noise < 12.5%\n"; }
                        m_nWarning = 0;
                    }
                }

                float value;
                uint32_t idx = m_nSamplePerSentBlock + receivedSamples - 1;
                if (m_eSenseChannels)
                {
                    // we don't check if the value has changed, we construct a square signal (1Hz --> 512Hz)

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_ATTENTION));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_MEDITATION));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;
                }
                if (m_bandPowerChannels)
                {
                    value         = float(TG_GetValue(m_connectionID, TG_DATA_DELTA));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_THETA));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_ALPHA1));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_ALPHA2));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_BETA1));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_BETA2));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_GAMMA1));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;

                    value         = float(TG_GetValue(m_connectionID, TG_DATA_GAMMA2));
                    m_sample[idx] = value;
                    idx += m_nSamplePerSentBlock;   // keep this line for the if def next
                }

#if defined(TG_DATA_BLINK_STRENGTH)
                bool blinkDetected = false;
                // We construct a "blink" spike signal if requested.
                if(m_blinkStrengthChannel)
                {
                    if(TG_GetValueStatus(m_connectionID,TG_DATA_BLINK_STRENGTH) != 0)
                    {
                        value = float(TG_GetValue(m_connectionID, TG_DATA_BLINK_STRENGTH));
                        blinkDetected = true;
                    }
                    else { value = 0; }
                    m_sample[idx] = value;
                }
                // We send a "blink" stimulation if requested.
                if(m_blinkStimulations)
                {
                    if(TG_GetValueStatus(m_connectionID,TG_DATA_BLINK_STRENGTH) != 0 || blinkDetected)
                    {
                        uint64_t time = CTime(m_header.getSamplingFrequency(),receivedSamples);
                        stimSet.push_back(OVTK_GDF_Eye_Blink, time,0);
                    }
                }
#endif
            }
            else
            {
                // Received something else than 1 packet. This is not necessarily an unrecoverable error. Try for a while.
                // @note this is using counting to avoid polling the clock constantly
                errorCount++;

                const uint32_t errorSleep     = 2; // In ms
                const uint32_t errorTolerance = 1000; // As each error sleeps 2 ms, 1000*2 = 2000ms
                if (errorCount > errorTolerance)
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "No valid packet from ThinkGear for a while, returning prematurely.\n";
                    return true;    // Let the acquisition server decide what to do, don't return a failure. It'll timeout after a bit.
                }

                System::Time::sleep(errorSleep);
            }
        }

        m_callback->setSamples(m_sample);
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
        m_callback->setStimulationSet(stimSet);
    }

    return true;
}

bool CDriverNeuroskyMindset::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Mindset Driver: STOP called.\n";
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    if (m_connectionID >= 0)
    {
        // We use FreeConnection() only as doing a TG_Disconnect()+TG_FreeConnection() pair can cause first-chance exceptions on visual studio & MindWave Mobile for some reason.
        TG_FreeConnection(m_connectionID);
        m_connectionID = -1;
    }

    return true;
}

bool CDriverNeuroskyMindset::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    if (m_sample)
    {
        delete [] m_sample;
        m_sample = nullptr;
    }
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverNeuroskyMindset::configure()
{
    CConfigurationNeuroskyMindset config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-Neurosky-Mindset.ui"
                                         , m_comPort, m_eSenseChannels, m_bandPowerChannels, m_blinkStimulations, m_blinkStrengthChannel);

    if (!config.configure(m_header)) { return false; }  // the basic configure will use the basic header
    m_settings.save();
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyThinkGearAPI
