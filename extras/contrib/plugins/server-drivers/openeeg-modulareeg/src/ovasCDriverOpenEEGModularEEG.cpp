/*
 * \author Christoph Veigl, Yann Renard
 *
 * \copyright AGPL3
 *
 */
#include "ovasCDriverOpenEEGModularEEG.h"
#include "ovasCConfigurationOpenEEGModularEEG.h"

#include <toolkit/ovtk_all.h>

#include <iostream>

#if defined TARGET_OS_Windows
#include <windows.h>
#include <winbase.h>
#include <cstdio>
#include <commctrl.h>
#define TERM_SPEED 57600
#elif defined TARGET_OS_Linux
 #include <cstdio>
 #include <unistd.h>
 #include <fcntl.h>
 #include <termios.h>
 #include <sys/select.h>
 #define TERM_SPEED B57600
#else
#endif


namespace OpenViBE {
namespace AcquisitionServer {

CDriverOpenEEGModularEEG::CDriverOpenEEGModularEEG(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_OpenEEG-ModularEEG", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(256);
    m_header.setChannelCount(m_nChannel);

    m_settings.add("Header", &m_header);
    m_settings.add("DeviceIdentifier", &m_deviceID);
    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverOpenEEGModularEEG::initialize(const uint32_t /*nSamplePerSentBlock*/, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    m_readState  = 0;
    m_extractPos = 0;

    if (!this->initTTY(&m_fileDesc, m_deviceID != uint32_t(-1) ? m_deviceID : 1)) { return false; }

    m_sample = new float[m_header.getChannelCount()];
    if (!m_sample)
    {
        delete [] m_sample;
        m_sample = nullptr;
        return false;
    }
    m_channelBuffers2.resize(6);

    m_callback         = &callback;
    m_nChannel         = m_header.getChannelCount();
    m_lastPacketNumber = 0;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << this->getName() << " driver initialized.\n";
    return true;
}

bool CDriverOpenEEGModularEEG::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << this->getName() << " driver started.\n";
    return true;
}

bool CDriverOpenEEGModularEEG::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }

    if (this->readPacketFromTTY(m_fileDesc) < 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not receive data from " << m_ttyName << "\n";
        return false;
    }

    if (!m_channelBuffers.empty())
    {
        if (m_driverCtx.isStarted())
        {
            for (size_t i = 0; i < m_channelBuffers.size(); ++i)
            {
                for (uint32_t j = 0; j < m_nChannel; ++j) { m_sample[j] = float(m_channelBuffers[i][j]) - 512.0F; }
                m_callback->setSamples(m_sample, 1);
            }
            m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
        }
        m_channelBuffers.clear();
    }

    return true;
}

bool CDriverOpenEEGModularEEG::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << CString(this->getName()) << " driver stopped.\n";
    return true;
}

bool CDriverOpenEEGModularEEG::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    this->closeTTY(m_fileDesc);

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << CString(this->getName()) << " driver closed.\n";

    delete [] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverOpenEEGModularEEG::configure()
{
    CConfigurationOpenEEGModularEEG config(Directories::getDataDir() + "/applications/acquisition-server/interface-OpenEEG-ModularEEG.ui", m_deviceID);
    if (!config.configure(m_header)) { return false; }
    m_settings.save();
    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverOpenEEGModularEEG::parseByteP2(const uint8_t actbyte)
{
    switch (m_readState)
    {
        case 0:
            if (actbyte == 165) { m_readState++; }
            break;

        case 1:
            if (actbyte == 90) { m_readState++; }
            else { m_readState = 0; }
            break;

        case 2:
            m_readState++;
            break;

        case 3:
            m_packetNumber = actbyte;
            m_extractPos = 0;
            m_readState++;
            break;

        case 4:
            if (m_extractPos < 12)
            {
                if ((m_extractPos & 1) == 0) { if (uint32_t(m_extractPos >> 1) < m_nChannel) { m_channelBuffers2[m_extractPos >> 1] = int(actbyte) << 8; } }
                else { if (uint32_t(m_extractPos >> 1) < m_nChannel) { m_channelBuffers2[m_extractPos >> 1] += actbyte; } }
                m_extractPos++;
            }
            else
            {
                m_channelBuffers.push_back(m_channelBuffers2);
                m_switches  = actbyte;
                m_readState = 0;
                return true;
            }
            break;

        default: m_readState = 0;
            break;
    }
    return false;
}

bool CDriverOpenEEGModularEEG::initTTY(FD_TYPE* descriptor, const uint32_t ttyNumber)
{
    char ttyName[1024];

#if defined TARGET_OS_Windows

    sprintf(ttyName, "\\\\.\\COM%d", ttyNumber);
    DCB dcb     = { 0 };
    *descriptor = ::CreateFile(LPCSTR(ttyName), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (*descriptor == INVALID_HANDLE_VALUE)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open port [" << CString(ttyName) << "]\n";
        return false;
    }

    if (!GetCommState(*descriptor, &dcb))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not get comm state on port [" << CString(ttyName) << "]\n";
        return false;
    }

    // update DCB rate, byte size, parity, and stop bits size
    dcb.DCBlength = sizeof(dcb);
    dcb.BaudRate  = CBR_56000;
    dcb.ByteSize  = 8;
    dcb.Parity    = NOPARITY;
    dcb.StopBits  = ONESTOPBIT;
    dcb.EvtChar   = '\0';

    // update flow control settings
    dcb.fDtrControl       = DTR_CONTROL_ENABLE;
    dcb.fRtsControl       = RTS_CONTROL_ENABLE;
    dcb.fOutxCtsFlow      = FALSE;
    dcb.fOutxDsrFlow      = FALSE;
    dcb.fDsrSensitivity   = FALSE;
    dcb.fOutX             = FALSE;
    dcb.fInX              = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.XonChar           = 0;
    dcb.XoffChar          = 0;
    dcb.XonLim            = 0;
    dcb.XoffLim           = 0;
    dcb.fParity           = FALSE;

    SetCommState(*descriptor, &dcb);
    SetupComm(*descriptor, 64/*1024*/, 64/*1024*/);
    EscapeCommFunction(*descriptor, SETDTR);
    SetCommMask(*descriptor, EV_RXCHAR | EV_CTS | EV_DSR | EV_RLSD | EV_RING);

#elif defined TARGET_OS_Linux

    struct termios terminalAtt;

    // open ttyS<i> for i < 10, else open ttyUSB<i-10>
    if(ttyNumber<10) { ::sprintf(ttyName, "/dev/ttyS%d", ttyNumber); }
    else { ::sprintf(ttyName, "/dev/ttyUSB%d", ttyNumber-10); }

    if((*descriptor=::open(ttyName, O_RDWR))==-1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open port [" << CString(ttyName) << "]\n";
        return false;
    }

    if(tcgetattr(*descriptor, &terminalAtt)!=0)
    {
        ::close(*descriptor);
        *descriptor=-1;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "terminal: tcgetattr() failed - did you use the right port [" << CString(ttyName) << "] ?\n";
        return false;
    }

    /* terminalAtt.c_cflag = TERM_SPEED | CS8 | CRTSCTS | CLOCAL | CREAD; */
    terminalAtt.c_cflag = TERM_SPEED | CS8 | CLOCAL | CREAD;
    terminalAtt.c_iflag = 0;
    terminalAtt.c_oflag = OPOST | ONLCR;
    terminalAtt.c_lflag = 0;
    if(tcsetattr(*descriptor, TCSAFLUSH, &terminalAtt)!=0)
    {
        ::close(*descriptor);
        *descriptor=-1;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "terminal: tcsetattr() failed - did you use the right port [" << CString(ttyName) << "] ?\n";
        return false;
    }

#else

    return false;

#endif

    m_ttyName = ttyName;
    return true;
}

void CDriverOpenEEGModularEEG::closeTTY(const FD_TYPE fileDesc)
{
#if defined TARGET_OS_Windows
    CloseHandle(fileDesc);
#elif defined TARGET_OS_Linux
    ::close(fileDesc);
#else
#endif
}

int CDriverOpenEEGModularEEG::readPacketFromTTY(const FD_TYPE fileDesc)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Enters readPacketFromTTY\n";

    uint8_t readBuffer[100];
    int packetsProcessed = 0;

#if defined TARGET_OS_Windows

    uint32_t readLength = 0;
    uint32_t readOk     = 0;
    struct _COMSTAT status;
    DWORD state;

    if (ClearCommError(fileDesc, &state, &status)) { readLength = status.cbInQue; }

    for (uint32_t i = 0; i < readLength; i++)
    {
        ReadFile(fileDesc, readBuffer, 1, LPDWORD(&readOk), nullptr);
        if (readOk == 1) { if (this->parseByteP2(readBuffer[0])) { packetsProcessed++; } }
    }

#elif defined TARGET_OS_Linux

    fd_set  inputFileDescSet;
    struct timeval time;
    size_t readLength=0;
    bool finished=false;

    time.tv_sec=0;
    time.tv_usec=0;

    do
    {
        FD_ZERO(&inputFileDescSet);
        FD_SET(fileDesc, &inputFileDescSet);

        switch(select(fileDesc+1, &inputFileDescSet, nullptr, nullptr, &time))
        {
            case -1: // error or timeout
            case  0:
                finished=true;
                break;

            default:
                if(FD_ISSET(fileDesc, &inputFileDescSet))
                {
                    if((readLength=::read(fileDesc, readBuffer, 1)) > 0)
                    {
                        for (uint32_t i = 0; i<readLength; i++) { if(this->parseByteP2(readBuffer[i])) { packetsProcessed++; } }
                    }
                }
                else { finished = true; }
                break;
        }
    }
    while(!finished);

#else

#endif

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Leaves readPacketFromTTY\n";
    return packetsProcessed;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
