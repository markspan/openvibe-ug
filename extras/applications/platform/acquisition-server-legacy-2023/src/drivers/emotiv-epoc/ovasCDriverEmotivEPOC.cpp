#if defined TARGET_HAS_ThirdPartyEmotivAPI

#include "ovasCDriverEmotivEPOC.h"
#include "ovasCConfigurationEmotivEPOC.h"

#include <system/ovCTime.h>

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <vector>

#if defined(TARGET_OS_Windows)
#include <delayimp.h>
#endif

namespace OpenViBE {
namespace AcquisitionServer {


#if defined(TARGET_HAS_ThirdPartyEmotivResearchAPI3x)
static const IEE_DataChannel_t CHANNEL_LIST[] = 
{
    IED_AF3, IED_F7, IED_F3, IED_FC5, IED_T7, IED_P7, IED_O1, IED_O2, IED_P8, IED_T8, IED_FC6, IED_F4, IED_F8, IED_AF4, 
    IED_GYROX, IED_GYROY,
    IED_COUNTER,
    IED_TIMESTAMP, 
    IED_FUNC_ID, IED_FUNC_VALUE, 
    IED_MARKER, 
    IED_SYNC_SIGNAL
};
#else
// Old API
static const EE_DataChannel_t CHANNEL_LIST[] =
{
    ED_AF3, ED_F7, ED_F3, ED_FC5, ED_T7, ED_P7, ED_O1, ED_O2, ED_P8, ED_T8, ED_FC6, ED_F4, ED_F8, ED_AF4,
    ED_GYROX, ED_GYROY,
    ED_COUNTER,
    ED_TIMESTAMP,
    ED_FUNC_ID, ED_FUNC_VALUE,
    ED_MARKER,
    ED_SYNC_SIGNAL
};
#endif

CDriverEmotivEPOC::CDriverEmotivEPOC(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_EmotivEPOC", m_driverCtx.getConfigurationManager())
{
    m_useGyroscope           = false;
    m_pathToEmotivSDK        = "";
    m_cmdForPathModification = "";

    m_userID         = 0;
    m_readyToCollect = false;
    m_firstStart     = true;

    m_settings.add("Header", &m_header);
    m_settings.add("UseGyroscope", &m_useGyroscope);
    m_settings.add("PathToEmotivSDK", &m_pathToEmotivSDK);
    m_settings.add("UserID", &m_userID);
    m_settings.load();
}

CDriverEmotivEPOC::~CDriverEmotivEPOC() {}

const char* CDriverEmotivEPOC::getName() { return "Emotiv EPOC"; }

bool CDriverEmotivEPOC::restoreState()
{
    // Restore the previous path
    if (m_oldPath.length() > 0) { _putenv_s("PATH", m_oldPath); }

#if defined TARGET_OS_Windows
    __FUnloadDelayLoadedDLL2("edk.dll");
#endif

    return true;
}

bool CDriverEmotivEPOC::buildPath()
{
    char* path = getenv("PATH");
    if (path == nullptr) { return false; }

    m_oldPath = path;

    const std::string str = std::string(path);
    const size_t found    = str.find(m_pathToEmotivSDK.toASCIIString());

    if (found == std::string::npos)
    {
        // If the emotiv component is not part of the path, we add it.
        m_cmdForPathModification = path + CString(";") + m_pathToEmotivSDK;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] Emotiv Driver: Building new Windows PATH.\n";
    }
    else
    {
        // If we found the emotiv component in path already, keep the path as-is.
        m_cmdForPathModification = CString(path);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] Emotiv Driver: Using the existing PATH.\n";
    }


    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverEmotivEPOC::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_eventHandle = nullptr;

    //we need to add the path to Emotiv SDK to PATH: done in external function 
    //because SEH (__try/__except) does not allow the use of local variables with destructor.
    if (!this->buildPath())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Emotiv Driver: Failed to get the ENV variable PATH.\n";
        return false;
    }

#if defined TARGET_OS_Windows
    // m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "[INIT] Emotiv Driver: Setting PATH as " << m_cmdForPathModification << "\n";

    if (_putenv_s("PATH", m_cmdForPathModification) != 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Emotiv Driver: Failed to modify the environment PATH with the Emotiv SDK path.\n";
        return false;
    }
#endif

    m_header.setChannelCount(14);
    if (m_useGyroscope)
    {
        m_header.setChannelCount(16); // 14 + 2 REF + 2 Gyro 
    }

    m_header.setChannelName(0, "AF3");
    m_header.setChannelName(1, "F7");
    m_header.setChannelName(2, "F3");
    m_header.setChannelName(3, "FC5");
    m_header.setChannelName(4, "T7");
    m_header.setChannelName(5, "P7");
    m_header.setChannelName(6, "O1");
    m_header.setChannelName(7, "O2");
    m_header.setChannelName(8, "P8");
    m_header.setChannelName(9, "T8");
    m_header.setChannelName(10, "FC6");
    m_header.setChannelName(11, "F4");
    m_header.setChannelName(12, "F8");
    m_header.setChannelName(13, "AF4");

    if (m_useGyroscope)
    {
        m_header.setChannelName(14, "Gyro-X");
        m_header.setChannelName(15, "Gyro-Y");
    }

    m_header.setSamplingFrequency(128); // let's hope so...

    // Set channel units
    // Various sources (e.g. http://emotiv.com/forum/forum15/topic879/messages/?PAGEN_1=3
    // and http://www.bci2000.org/wiki/index.php/Contributions:Emotiv ) suggested
    // that the units from the device are in microvolts, but with a typical DC offset around 4000. 
    // Hard to find official source.
    for (uint32_t c = 0; c < 14; ++c) { m_header.setChannelUnits(c, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); }
    if (m_useGyroscope)
    {
        // Even less sure about the units of these, leaving as unspecified.
        m_header.setChannelUnits(14, OVTK_UNIT_Unspecified, OVTK_FACTOR_Base);
        m_header.setChannelUnits(15, OVTK_UNIT_Unspecified, OVTK_FACTOR_Base);
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "INIT called.\n";
    if (m_driverCtx.isConnected())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Emotiv Driver: Driver already initialized.\n";
        restoreState();
        return false;
    }

    if (!m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Emotiv Driver: Channel count or frequency not set.\n";
        restoreState();
        return false;
    }

    //---------------------------------------------------------
    // Builds up a buffer to store acquired samples. This buffer will be sent to the acquisition server later.

    m_sample = new float[m_header.getChannelCount()];
    //m_Buffer=new double[m_header.getChannelCount()*nSamplePerSentBlock];
    if (!m_sample /*|| !m_Buffer*/)
    {
        delete [] m_sample;
        //delete [] m_Buffer;
        m_sample = nullptr;
        //m_Buffer= nullptr;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Emotiv Driver: Samples allocation failed.\n";
        restoreState();
        return false;
    }

    //__________________________________
    // Hardware initialization

    m_readyToCollect = false;
#if defined TARGET_OS_Windows
    // First call to a function from EDK.DLL: guard the call with __try/__except clauses.
    __try
#elif defined TARGET_OS_Linux
    try
#endif
    {
        m_eventHandle   = IEE_EmoEngineEventCreate();
        m_lastErrorCode = IEE_EngineConnect();
    }
#if defined TARGET_OS_Windows
    __except (EXCEPTION_EXECUTE_HANDLER)
#elif defined TARGET_OS_Linux
    catch(...)
#endif
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Emotiv Driver: First call to 'edk.dll' failed.\n"
                << "\tThis driver needs Emotiv SDK Research Edition (or better)\n"
                << "\tinstalled on your computer.\n"
                << "\tYou have configured the driver to use path [" << m_pathToEmotivSDK << "].\n"
                << "\tIf there is an 'edk.dll' there, its version may be incompatible.\n"
                << "\tThis driver was built against 32-bit (x86) Emotiv SDK v "
#if defined(TARGET_HAS_ThirdPartyEmotivResearchAPI3x)
            << "3.x.x.\n";
#else
                << "1.x.x.\n";
#endif

        restoreState();
        return false;
    }

    if (m_lastErrorCode != EDK_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Emotiv Driver: Can't connect to EmoEngine. EDK Error Code [" << m_lastErrorCode << "]\n";
        restoreState();
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] Emotiv Driver: Connection to EmoEngine successful.\n";

#if defined(TARGET_HAS_ThirdPartyEmotivResearchAPI)
    unsigned long hwVersion = 0;
    unsigned long buildNum = 0;
    char version[16];

    IEE_HardwareGetVersion(m_userID, &hwVersion);
    IEE_SoftwareGetVersion(version, 16, &buildNum);

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Emotiv Driver: "
        << "headset version " << uint64_t(hwVersion && 0xFFFF) << " / "  << uint64_t(hwVersion >> 16) 
        << ", software " << version << ", build " << uint64_t(buildNum) << "\n";
#endif

    //__________________________________
    // Saves parameters

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverEmotivEPOC::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "START called.\n";
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_dataHandle = IEE_DataCreate();
    //float sec = (float)m_nSamplePerSentBlock/(float)m_header.getSamplingFrequency();
    float sec       = 1;
    m_lastErrorCode = IEE_DataSetBufferSizeInSec(sec);
    if (m_lastErrorCode != EDK_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[START] Emotiv Driver: Set buffer size to [" << sec << "] sec failed. EDK Error Code [" << m_lastErrorCode << "]\n";
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[START] Emotiv Driver: Data Handle created. Buffer size set to [" << sec << "] sec.\n";

    return true;
}

bool CDriverEmotivEPOC::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }

    if (m_driverCtx.isStarted())
    {
        // we enable the acquisiton for every new headset (user) ever added
        if (IEE_EngineGetNextEvent(m_eventHandle) == EDK_OK)
        {
            IEE_Event_t type = IEE_EmoEngineEventGetType(m_eventHandle);


            if (type == IEE_UserAdded)
            {
                uint32_t id;
                IEE_EmoEngineEventGetUserId(m_eventHandle, &id);
                m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[LOOP] Emotiv Driver: User #" << id << " registered.\n";
                m_lastErrorCode = IEE_DataAcquisitionEnable(id, true);
                if (m_lastErrorCode != EDK_OK)
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[LOOP] Emotiv Driver: Enabling acquisition failed. EDK Error Code [" << m_lastErrorCode << "]\n";
                    return false;
                }
                // but we are ready to acquire the samples only if the requested headset is detected
                m_readyToCollect = m_readyToCollect || (m_userID == id);
            }
        }

        if (m_readyToCollect)
        {
            uint32_t nSamplesTaken = 0;
            m_lastErrorCode        = IEE_DataUpdateHandle(m_userID, m_dataHandle);
            if (m_lastErrorCode != EDK_OK)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[LOOP] Emotiv Driver: An error occurred while updating the DataHandle. EDK Error Code [" << m_lastErrorCode << "]\n";
                return false;
            }
            m_lastErrorCode = IEE_DataGetNumberOfSample(m_dataHandle, &nSamplesTaken);
            if (m_lastErrorCode != EDK_OK)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[LOOP] Emotiv Driver: An error occurred while getting new samples from device. EDK Error Code [" << m_lastErrorCode << "]\n";
                return false;
            }
            // warning : if you connect/disconnect then reconnect, the internal buffer may be full of samples, thus maybe nSamplesTaken > m_nSamplePerSentBlock
            m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "EMOTIV EPOC >>> received [" << nSamplesTaken << "] samples per channel from device with user #" << m_userID << ".\n";

            /*
            for (size_t i=0; i<m_header.getChannelCount(); ++i)
            {
                for (uint32_t s=0;s<nSamplesTaken;s++)
                {
                    double* buffer = new double[nSamplesTaken];
                    m_lastErrorCode = IEE_DataGet(m_dataHandle, CHANNEL_LIST[i], buffer, nSamplesTaken);
                    if(m_lastErrorCode != EDK_OK)
                    {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[LOOP] Emotiv Driver: An error occurred while getting new samples from device. EDK Error Code [" << m_lastErrorCode << "]\n";
                        return false;
                    }
                    
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "EMOTIV EPOC >>> adding sample with value ["<< buffer[s] <<"]\n";
                    m_sample[i*m_nSamplePerSentBlock + s] = (float)buffer[s];
                    delete [] buffer;
                }
                m_callback->setSamples(m_sample);
            }
            m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
            */

            double* buffer = new double[nSamplesTaken];
            for (uint32_t s = 0; s < nSamplesTaken; ++s)
            {
                for (size_t i = 0; i < m_header.getChannelCount(); ++i)
                {
                    m_lastErrorCode = IEE_DataGet(m_dataHandle, CHANNEL_LIST[i], buffer, nSamplesTaken);
                    if (m_lastErrorCode != EDK_OK)
                    {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[LOOP] Emotiv Driver: An error occurred while getting new samples from device. EDK Error Code [" << m_lastErrorCode << "]\n";
                        return false;
                    }
                    m_sample[i] = float(buffer[s]);
                    if (s == 0) m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "EMOTIV EPOC >>> sample received has value [" << buffer[s] << "]\n";
                    //m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "EMOTIV EPOC >>> sample stored has value ["<< m_sample[i] <<"]\n";
                }
                m_callback->setSamples(m_sample, 1);
            }
            m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
            delete [] buffer;
        }
    }

    return true;
}

bool CDriverEmotivEPOC::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "STOP called.\n";
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    IEE_DataFree(m_dataHandle);

    return true;
}

bool CDriverEmotivEPOC::uninitialize()
{
    IEE_EngineDisconnect();

    if (m_eventHandle)
    {
        IEE_EmoEngineEventFree(m_eventHandle);
        m_eventHandle = nullptr;
    }

    restoreState();

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverEmotivEPOC::configure()
{
    CConfigurationEmotivEPOC config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-Emotiv-EPOC.ui", m_useGyroscope, m_pathToEmotivSDK, m_userID);

    if (!config.configure(m_header)) { return false; }
    m_settings.save();
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyEmotivAPI
