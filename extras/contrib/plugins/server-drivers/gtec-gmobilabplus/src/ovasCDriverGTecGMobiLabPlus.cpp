/**
 * The gMobilab Linux driver was contributed by Lucie Daubigney from Supelec Metz
 *
 * Windows compatibility + gusbamp coexistence added by Jussi T. Lindgren / Inria
 *
 */

#include "ovasCDriverGTecGMobiLabPlus.h"
#include "ovasCConfigurationGTecGMobiLabPlus.h"

#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI

#include "ovasCDriverGTecGMobiLabPlusPrivate.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#if defined(TARGET_OS_Linux)
#include <dlfcn.h>
#endif


namespace OpenViBE {
namespace AcquisitionServer {

static const uint32_t N_ACQUIRED_CHANNEL = 8;

//constructor
CDriverGTecGMobiLabPlus::CDriverGTecGMobiLabPlus(IDriverContext& ctx)
    : IDriver(ctx)
      , m_settings("AcquisitionServer_Driver_GTecMobiLabPlus", m_driverCtx.getConfigurationManager())
      , m_library(nullptr)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::CDriverGTecGMobiLabPlus\n";

    m_gTec = new CDriverGTecGMobiLabPlusPrivate();

    m_header = new CHeader();
    m_header->setSamplingFrequency(256);
    m_header->setChannelCount(8);

    m_gTec->m_oBuffer.pBuffer     = nullptr;
    m_gTec->m_oBuffer.size        = 0;
    m_gTec->m_oBuffer.validPoints = 0;
#if defined(TARGET_OS_Windows)
    m_portName = "//./COM1";
#else
    m_portName="/dev/rfcomm0";
#endif

    //initialisation of the analog channels of the gTec module : by default no analog exchange are allowed
    m_gTec->m_analogIn.ain1 = false;
    m_gTec->m_analogIn.ain2 = false;
    m_gTec->m_analogIn.ain3 = false;
    m_gTec->m_analogIn.ain4 = false;
    m_gTec->m_analogIn.ain5 = false;
    m_gTec->m_analogIn.ain6 = false;
    m_gTec->m_analogIn.ain7 = false;
    m_gTec->m_analogIn.ain8 = false;

    m_settings.add("Header", m_header);
    m_settings.add("PortName", &m_portName);
    m_settings.add("TestMode", &m_testMode);
    m_settings.load();
}

CDriverGTecGMobiLabPlus::~CDriverGTecGMobiLabPlus()
{
    delete m_header;
    delete m_gTec;
}

void CDriverGTecGMobiLabPlus::release()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::release\n";
    delete this;
}

const char* CDriverGTecGMobiLabPlus::getName()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::getName\n";
    return "g.tec gMOBIlab+";
}

//___________________________________________________________________//
//                                                                   //

/*
 * configuration
 */

bool CDriverGTecGMobiLabPlus::isConfigurable()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::isConfigurable\n";
    return true;
}

bool CDriverGTecGMobiLabPlus::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::configure\n";

    // We use CConfigurationGTecMobilabPlus configuration which is a class that inheritate from the CConfigurationBuilder class
    // The difference between these two classes is the addition of a member of class. This member allows to change the port where is connected the device.
    CConfigurationGTecGMobiLabPlus config(Directories::getDataDir() + "/applications/acquisition-server/interface-GTec-GMobiLabPlus.ui", m_portName,
                                          m_testMode);

    // We configure the Header with it...
    if (!config.configure(*m_header)) { return false; }

    if (m_header->getChannelCount() > N_ACQUIRED_CHANNEL) { m_header->setChannelCount(N_ACQUIRED_CHANNEL); }

    m_settings.save();

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Port name after configuration " << m_portName << " \n";
    return true;
}

//___________________________________________________________________//
//                                                                   //

#if defined(TARGET_OS_Linux)
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#endif

bool CDriverGTecGMobiLabPlus::registerLibraryFunctions()
{
    // Lets open the DLL
#if defined(TARGET_OS_Windows)
    m_library = LoadLibrary("gMOBIlabplus.dll");
#else
    m_library = dlopen("libgmobilabplusapi.so", RTLD_LAZY);
#endif
    if (!m_library)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "CDriverGTecGMobiLabPlus:: Unable to open gMOBIlabplus.dll\n";
        return false;
    }

    m_gTec->m_fOpenDevice         = CDriverGTecGMobiLabPlusPrivate::OV_GT_OpenDevice(GetProcAddress(m_library, "GT_OpenDevice"));
    m_gTec->m_fCloseDevice        = CDriverGTecGMobiLabPlusPrivate::OV_GT_CloseDevice(GetProcAddress(m_library, "GT_CloseDevice"));
    m_gTec->m_fSetTestmode        = CDriverGTecGMobiLabPlusPrivate::OV_GT_SetTestmode(GetProcAddress(m_library, "GT_SetTestmode"));
    m_gTec->m_fStartAcquisition   = CDriverGTecGMobiLabPlusPrivate::OV_GT_StartAcquisition(GetProcAddress(m_library, "GT_StartAcquisition"));
    m_gTec->m_fGetData            = CDriverGTecGMobiLabPlusPrivate::OV_GT_GetData(GetProcAddress(m_library, "GT_GetData"));
    m_gTec->m_fInitChannels       = CDriverGTecGMobiLabPlusPrivate::OV_GT_InitChannels(GetProcAddress(m_library, "GT_InitChannels"));
    m_gTec->m_fStopAcquisition    = CDriverGTecGMobiLabPlusPrivate::OV_GT_StopAcquisition(GetProcAddress(m_library, "GT_StopAcquisition"));
    m_gTec->m_fGetLastError       = CDriverGTecGMobiLabPlusPrivate::OV_GT_GetLastError(GetProcAddress(m_library, "GT_GetLastError"));
    m_gTec->m_fTranslateErrorCode = CDriverGTecGMobiLabPlusPrivate::OV_GT_TranslateErrorCode(GetProcAddress(m_library, "GT_TranslateErrorCode"));

    if (!m_gTec->m_fOpenDevice || !m_gTec->m_fCloseDevice || !m_gTec->m_fSetTestmode
        || !m_gTec->m_fStartAcquisition || !m_gTec->m_fGetData || !m_gTec->m_fInitChannels
        || !m_gTec->m_fStopAcquisition || !m_gTec->m_fGetLastError || !m_gTec->m_fTranslateErrorCode)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "CDriverGTecGMobiLabPlus:: Unable to find all the required functions from the gMOBIlabplus.dll\n";
        return false;
    }

    return true;
}


/*
 * initialisation
 */
bool CDriverGTecGMobiLabPlus::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::initialize\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Port name after initialisation " << m_portName << "\n";

    if (m_driverCtx.isConnected()) { return false; }

    if (!m_header->isChannelCountSet() || !m_header->isSamplingFrequencySet())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Either channel count or sampling frequency is not set\n";
        return false;
    }

    if (!registerLibraryFunctions()) { return false; }

    const size_t nChannel = m_header->getChannelCount();

    // analog exchanges allowed on the first "nChannel" channels:
    for (uint32_t i = 1; i <= nChannel; ++i) { allowAnalogInputs(i); }

    // then buffer of type _BUFFER_ST built to store acquired samples.
    m_gTec->m_oBuffer.pBuffer = new short int[nChannel
    ];//allocate enough space for the buffer m_oBuffer.pBuffer ; only one set of mesures is acquired (channel 1 to 8) in a row
    m_gTec->m_oBuffer.size        = nChannel * sizeof(short int);
    m_gTec->m_oBuffer.validPoints = 0;

#if defined(TARGET_OS_Windows)
    m_gTec->m_oOverlap.hEvent     = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_gTec->m_oOverlap.Offset     = 0;
    m_gTec->m_oOverlap.OffsetHigh = 0;
#endif

    // allocates enough space for m_sample
    m_sample = new float[nSamplePerSentBlock * nChannel];

    // if there is a problem while creating the two arrays
    if (!m_gTec->m_oBuffer.pBuffer || !m_sample)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Memory allocation problem\n";
        delete [] m_gTec->m_oBuffer.pBuffer;
        delete [] m_sample;
        m_sample                  = nullptr;
        m_gTec->m_oBuffer.pBuffer = nullptr;
        return false;
    }

    // initializes hardware and get
    // available header information
    // from it
#if defined(TARGET_OS_Windows)
    m_gTec->m_device = m_gTec->m_fOpenDevice(LPSTR(m_portName.c_str()));
#else
    m_gTec->m_device = m_gTec->m_fOpenDevice(m_portName.c_str());
#endif
    if (m_gTec->m_device == nullptr)
    {
#if defined(TARGET_OS_Windows)
        UINT errorCode = 0;
#else
        uint32_t errorCode = 0;
#endif
        _ERRSTR error;
        m_gTec->m_fGetLastError(&errorCode);
        m_gTec->m_fTranslateErrorCode(&error, errorCode);

        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to connect to [" << m_portName << "], error code " << errorCode << ": '" << error.Error
                << "'\n";
        delete [] m_gTec->m_oBuffer.pBuffer;
        delete [] m_sample;
        return false;
    }

    // saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    return true;
}

bool CDriverGTecGMobiLabPlus::uninitialize()
{
    bool ok = true;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::uninitialize\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // uninitializes hardware here
    if (!m_gTec->m_fCloseDevice(m_gTec->m_device))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "GT_CloseDevice() failed\n";
        ok = false;
    }

    // frees memory
    delete [] m_sample;
    delete [] m_gTec->m_oBuffer.pBuffer;

    m_sample                  = nullptr;
    m_gTec->m_oBuffer.pBuffer = nullptr;
    m_callback                = nullptr;

    // uninitialisation of the analog channels : set valus to default ones
    m_gTec->m_analogIn.ain1 = false;
    m_gTec->m_analogIn.ain2 = false;
    m_gTec->m_analogIn.ain3 = false;
    m_gTec->m_analogIn.ain4 = false;
    m_gTec->m_analogIn.ain5 = false;
    m_gTec->m_analogIn.ain6 = false;
    m_gTec->m_analogIn.ain7 = false;
    m_gTec->m_analogIn.ain8 = false;

    if (m_library)
    {
        FreeLibrary(m_library);
        m_library = nullptr;
    }

    return ok;
}

const IHeader* CDriverGTecGMobiLabPlus::getHeader() { return m_header; }

//___________________________________________________________________//
//                                                                   //

/*
 * acquisition
 */

bool CDriverGTecGMobiLabPlus::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::start\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // we use none of the digital inputs/outputs
    _DIO digitalInOut;
    digitalInOut.dio1_enable = false;
    digitalInOut.dio2_enable = false;
    digitalInOut.dio3_enable = false;
    digitalInOut.dio4_enable = false;
    digitalInOut.dio5_enable = false;
    digitalInOut.dio6_enable = false;
    digitalInOut.dio7_enable = false;
    digitalInOut.dio8_enable = false;

    // channel initialisation
    if (!m_gTec->m_fInitChannels(m_gTec->m_device, m_gTec->m_analogIn, digitalInOut))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "GT_InitChannels failed\n";
        return false;
    }

    // are we interested in test signal?
    m_gTec->m_fSetTestmode(m_gTec->m_device, m_testMode);

    // requests hardware to start sending data
    if (!m_gTec->m_fStartAcquisition(m_gTec->m_device))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "GT_StartAcquisition failed\n";
        return false;
    }
    return true;
}

bool CDriverGTecGMobiLabPlus::loop()
{
    // m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::loop\n";

    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return true; }

    const size_t nChannel = m_header->getChannelCount();

    // only "l-nChannel" measures corresponding to one measure per channel are acquired in a row with the function GT_GetData()
    // these measures are stored in m_oBuffer.pBuffer[]
    // the acquisition is reapeted m_sampleCountPerSendBlock times to fill in the array "m_sample"
    for (uint32_t i = 0; i < m_nSamplePerSentBlock; ++i)
    {
#if defined(TARGET_OS_Windows)
        if (!m_gTec->m_fGetData(m_gTec->m_device, &m_gTec->m_oBuffer, &m_gTec->m_oOverlap))// receive samples from hardware (one per channel)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "GT_GetData failed\n";
            return false;
        }
        if (WaitForSingleObject(m_gTec->m_oOverlap.hEvent, 1000) == WAIT_TIMEOUT)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Timeout in reading from the device\n";
            return false;
        }
#else
        if (!m_gTec->m_fGetData(m_gTec->m_device, &m_gTec->m_oBuffer))// receive samples from hardware (one per channel)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "GT_GetData failed\n";
            return false;
        }
#endif

        // here the "nChannel" measures just acquired are stored in m_sample not to be deleted by the next acquisition
        // m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Here are the " << nChannel << " measures of the " << i << " th sample\n" << Kernel::LogLevel_Debug;
        for (uint32_t j = 0; j < nChannel; ++j)
        {
            // m_driverCtx.getLogManager() << (m_oBuffer.pBuffer[j]*0.5)/32768. << " ";
            //operation made to modify the short int in a number between 0 and 500mV (in Volt)
            m_sample[m_nSamplePerSentBlock * j + i] = float((m_gTec->m_oBuffer.pBuffer[j] * 0.5) / 32768.);
        }
        // m_driverCtx.getLogManager() << "\n";
    }

    // the buffer is full : it is send to the acquisition server
    m_callback->setSamples(m_sample, m_nSamplePerSentBlock);
    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

    return true;
}

bool CDriverGTecGMobiLabPlus::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGTecGMobiLabPlus::stop\n";

    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    // requests the hardware to stop sending data
    if (!m_gTec->m_fStopAcquisition(m_gTec->m_device))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "GT_StopAcquisition failed\n";
        return false;
    }

    return true;
}

// this function allows exchanges of data on the "index" channel
// function used to initialize the analog inputs according to the number "nChannel"
void CDriverGTecGMobiLabPlus::allowAnalogInputs(uint32_t index)
{
    switch (index)
    {
        case 8: m_gTec->m_analogIn.ain8 = true;
            break;
        case 7: m_gTec->m_analogIn.ain7 = true;
            break;
        case 6: m_gTec->m_analogIn.ain6 = true;
            break;
        case 5: m_gTec->m_analogIn.ain5 = true;
            break;
        case 4: m_gTec->m_analogIn.ain4 = true;
            break;
        case 3: m_gTec->m_analogIn.ain3 = true;
            break;
        case 2: m_gTec->m_analogIn.ain2 = true;
            break;
        case 1: m_gTec->m_analogIn.ain1 = true;
            break;
        default: m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Unexpected value " << index << " in CDriverGTecGMobiLabPlus::allowAnalogInputs\n";
            break;
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGMobiLabPlusAPI
