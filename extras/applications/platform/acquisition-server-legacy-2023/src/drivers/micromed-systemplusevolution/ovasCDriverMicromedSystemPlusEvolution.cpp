#if defined(TARGET_HAS_ThirdPartyMicromed)

#include "ovasCDriverMicromedSystemPlusEvolution.h"
#include "../ovasCConfigurationNetworkBuilder.h"

#if defined TARGET_OS_Windows

#define MicromedDLL "dllMicromed.dll"

#include <system/ovCTime.h>

#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <windows.h>
#include <cstring>

#include <algorithm> // std::min, etc on VS2013

namespace OpenViBE {
namespace AcquisitionServer {

typedef char* ( __stdcall * STRUCTHEADER)();
typedef int ( __stdcall * STRUCTHEADERSIZE)();
typedef char* ( __stdcall * STRUCTHEADERINFO)();
typedef int ( __stdcall * STRUCTHEADERINFOSIZE)();
typedef unsigned short int* ( __stdcall * STRUCTBUFFDATA)();
typedef int ( __stdcall * STRUCTBUFFDATASIZE)();
typedef unsigned char* ( __stdcall * STRUCTBUFFNOTE)();
typedef int ( __stdcall * STRUCTBUFFNOTESIZE)();
typedef unsigned char* ( __stdcall * STRUCTBUFFTRIGGER)();
typedef int ( __stdcall * STRUCTBUFFTRIGGERSIZE)();
typedef bool ( __stdcall * HEADERVALID)();
typedef bool ( __stdcall * DATAHEADER)();
typedef bool ( __stdcall * NOTEHEADER)();
typedef bool ( __stdcall * TRIGGERHEADER)();
typedef bool ( __stdcall * INITHEADER)();
typedef uint32_t ( __stdcall * DATALENGTH)();
//typedef uint32_t        ( __stdcall * ADDRESSOFDATA)        ();
typedef uint32_t ( __stdcall * NBOFCHANNELS)();
typedef uint32_t ( __stdcall * MINSAMPLINGRATE)();
typedef uint32_t ( __stdcall * SIZEOFEACHDATAINBYTE)();
typedef float ( __stdcall * DATAVALUE)(int numChannel, int numSample);
typedef int ( __stdcall * TRIGGERCOUNT)();
typedef unsigned long int ( __stdcall * TRIGGERSAMPLE)(int indexTrigger);
typedef unsigned short int ( __stdcall * TRIGGERVALUE)(int indexTrigger);
typedef int ( __stdcall * NOTECOUNT)();
typedef unsigned long int ( __stdcall * NOTESAMPLE)(int indexNote);
typedef char* ( __stdcall * NOTECOMMENT)(int indexNote);
typedef void ( __stdcall * SHOWELECTRODE)(std::stringstream* info);
typedef void ( __stdcall * SHOWNOTE)(std::stringstream* info);
typedef void ( __stdcall * SHOWTRIGGER)(std::stringstream* trigger);
typedef void ( __stdcall * SHOWSIGNAL)(std::stringstream* signal);

// Header  Structure
//---------------------------
//pointer on the header structure. the header structure is send before any HeaderInfo or BuffData structure.
STRUCTHEADER fGetStructHeader;

//give the size of the structHeader
STRUCTHEADERSIZE fGetStructHeaderSize;

//say if the last structHeader is valid.
//must be call just after structHeader.
HEADERVALID fIsHeaderValid;

//say if the Header structure is following by a Buffer Data structure
//must be call after structHeader.
DATAHEADER fIsDataHeader;

//say if the Header structure is following by a Buffer Note structure
//must be call after structHeader.
NOTEHEADER fIsNoteHeader;

//say if the Header structure is following by a Buffer Trigger structure
//must be call after structHeader.
TRIGGERHEADER fIsTriggerHeader;

//say if the Header structure is following by a Header Info Structure
//must be call after structHeader.
INITHEADER fIsInitHeader;

//give the size of the following data receive
//must be call after structHeader.
DATALENGTH fGetDataLength;

//   Header Info Structure
//---------------------------

//give many information of the device configuration.
STRUCTHEADERINFO fGetStructHeaderInfo;

//give the size of the structHeaderInfo
STRUCTHEADERINFOSIZE fGetStructHeaderInfoSize;

//give address of the first data. the address count start to the beginning of this structure.
//it necessary to receive information between this structure and the first address data before beginning.
//must be call after structHeaderInfo.
//ADDRESSOFDATA fGetAddressOfData;

//give the number of channels connected to this device.
//must be call after structHeaderInfo.
NBOFCHANNELS fGetNbOfChannels;

//give the sampling rate of the device
//must be call after structHeaderInfo.
MINSAMPLINGRATE fGetMinimumSamplingRate;

//give the size in byte for one sample of one channels
//must be call after structHeaderInfo.
SIZEOFEACHDATAINBYTE fGetSizeOfEachDataInByte;

//Give the value of the samples and channel specify
DATAVALUE fGetDataValue;

SHOWELECTRODE fShowElectrode;
SHOWNOTE fShowNote;
SHOWTRIGGER fShowTrigger;
SHOWSIGNAL fShowSignal;

//   Buffer Data Structure
//---------------------------

//give sample of channels
STRUCTBUFFDATA fGetStructBuffData;
//give the size of the Data buffer
STRUCTBUFFDATASIZE fGetStructBuffDataSize;
//   Buffer Note Structure
//---------------------------

//give sample of channels
STRUCTBUFFNOTE fGetStructBuffNote;
//give the size of the Data buffer
STRUCTBUFFNOTESIZE fGetStructBuffNoteSize;

//give the number of Note received in the last data block
NOTECOUNT fGetNoteCount;

//give the number of the sample whose the time corresponding to the reception of the note specified by the parameter
NOTESAMPLE fGetNoteSample;

//give the comment corresponding to the note specified by the parameter
NOTECOMMENT fGetNoteComment;

//   Buffer Trigger Structure
//---------------------------

//give sample of channels
STRUCTBUFFTRIGGER fGetStructBuffTrigger;
//give the size of the Data buffer
STRUCTBUFFTRIGGERSIZE fGetStructBuffTriggerSize;

//give the number of Trigger received in the last data block
TRIGGERCOUNT fGetTriggerCount;

//give the number of the sample whose the time corresponding to the reception of the trigger specified by the parameter
TRIGGERSAMPLE fGetTriggerSample;

//give the value corresponding to the trigger specified by the parameter
TRIGGERVALUE fGetTriggerValue;

//lib
HINSTANCE libMicromed; //Library Handle

//reg key
static char tcpPortNumber[1024];
static char tcpSendAcq[1024];
static char tcpServerName[1024];
static HKEY registryKey = nullptr;
//  bool g_bInitializedFromRegistry=false;
static const char* registeryKeyName = "Software\\VB and VBA Program Settings\\Brain Quick - System 98\\EEG_Settings";

#define LOAD_DLL_FUNC(var, type, name) \
        var = (type)::GetProcAddress(libMicromed, name); \
        if(!var) \
        { \
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Load method " << name << "\n"; \
            m_valid=false; \
            return false; \
        }

CDriverMicromedSystemPlusEvolution::CDriverMicromedSystemPlusEvolution(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_SystemPlusEvolution", m_driverCtx.getConfigurationManager())
{
    libMicromed = nullptr;

    m_settings.add("Header", &m_header);
    m_settings.add("ServerHostPort", &m_ServerHostPort);
    m_settings.add("TimeOutMs", &m_timeOutMilliseconds);
    m_settings.load();
}

CDriverMicromedSystemPlusEvolution::~CDriverMicromedSystemPlusEvolution()
{
    if (m_ConnectionServer) {
        m_ConnectionServer->release();
        m_ConnectionServer = nullptr;
    }

    /**
     * Is it necessary to restore the
     * registry keys here ?
     */

#if 0
    if(ERROR_SUCCESS!=::RegOpenKeyEx(HKEY_CURRENT_USER, registeryKeyName, 0, KEY_WRITE, &registryKey)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key " << registeryKeyName << " not restored\n";
    }

    if(tcpPortNumber!=std::string("")) {
        if(ERROR_SUCCESS!=::RegSetValueEx(registryKey, "tcpPortNumber", 0, REG_SZ, (LPBYTE)tcpPortNumber, ::strlen(tcpPortNumber))) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpPortNumber not restored\n";
        }
    }
    else {
        if(ERROR_SUCCESS!=::RegDeleteValue(registryKey, "tcpPortNumber")) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpPortNumber not deleted\n";
        }
    }

    if(tcpSendAcq!=std::string("")) {
        if(ERROR_SUCCESS!=::RegSetValueEx(registryKey, "tcpSendAcq", 0, REG_SZ, (LPBYTE)tcpSendAcq, ::strlen(tcpSendAcq))) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpSendAcq not restored\n";
        }
    }
    else {
        if(ERROR_SUCCESS!=::RegDeleteValue(registryKey, "tcpSendAcq")) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpSendAcq not deleted\n";
        }
    }

    if(tcpServerName!=std::string("")) {
        if(ERROR_SUCCESS!=::RegSetValueEx(registryKey, "tcpServerName", 0, REG_SZ, (LPBYTE)tcpServerName, ::strlen(tcpServerName))) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpServerName not restored\n";
        }
    }
    else { 
        if(ERROR_SUCCESS!=::RegDeleteValue(registryKey, "tcpServerName")) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpServerName not deleted\n";
        }
    }

    ::RegCloseKey(registryKey);
    registryKey= nullptr;
#endif
}

// Load the ddl of the driver
bool CDriverMicromedSystemPlusEvolution::loadDLL()
{
    if (libMicromed) { return true; } // already loaded

    //Open library
    const CString path = m_driverCtx.getConfigurationManager().expand("${Path_Bin}") + "/" + MicromedDLL;
    libMicromed        = ::LoadLibrary(path.toASCIIString());

    //if it can't be open return FALSE;
    if (libMicromed == nullptr) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Couldn't load DLL: " << path << "\n";
        return false;
    }

    //load the method for initialized the driver
    LOAD_DLL_FUNC(fGetStructHeader, STRUCTHEADER, "getStructHeader");
    LOAD_DLL_FUNC(fGetStructHeaderSize, STRUCTHEADERSIZE, "getStructHeaderSize");
    LOAD_DLL_FUNC(fGetStructHeaderInfo, STRUCTHEADERINFO, "getStructHeaderInfo");
    LOAD_DLL_FUNC(fGetStructHeaderInfoSize, STRUCTHEADERINFOSIZE, "getStructHeaderInfoSize");
    LOAD_DLL_FUNC(fGetStructBuffData, STRUCTBUFFDATA, "getStructBuffData");
    LOAD_DLL_FUNC(fGetStructBuffDataSize, STRUCTBUFFDATASIZE, "getStructBuffDataSize");
    LOAD_DLL_FUNC(fGetStructBuffNote, STRUCTBUFFNOTE, "getStructBuffNote");
    LOAD_DLL_FUNC(fGetStructBuffNoteSize, STRUCTBUFFNOTESIZE, "getStructBuffNoteSize");
    LOAD_DLL_FUNC(fGetStructBuffTrigger, STRUCTBUFFTRIGGER, "getStructBuffTrigger");
    LOAD_DLL_FUNC(fGetStructBuffTriggerSize, STRUCTBUFFTRIGGERSIZE, "getStructBuffTriggerSize");
    LOAD_DLL_FUNC(fIsHeaderValid, HEADERVALID, "isHeaderValid");
    LOAD_DLL_FUNC(fIsDataHeader, DATAHEADER, "isDataHeader");
    LOAD_DLL_FUNC(fIsNoteHeader, NOTEHEADER, "isNoteHeader");
    LOAD_DLL_FUNC(fIsTriggerHeader, TRIGGERHEADER, "isTriggerHeader");
    LOAD_DLL_FUNC(fIsInitHeader, INITHEADER, "isInitHeader");
    LOAD_DLL_FUNC(fGetDataLength, DATALENGTH, "getDataLength");
    //LOAD_DLL_FUNC(fGetAddressOfData, ADDRESSOFDATA, "getAddressOfData");

    LOAD_DLL_FUNC(fGetNbOfChannels, NBOFCHANNELS, "getNbOfChannels");
    LOAD_DLL_FUNC(fGetMinimumSamplingRate, MINSAMPLINGRATE, "getMinimumSamplingRate");
    LOAD_DLL_FUNC(fGetSizeOfEachDataInByte, SIZEOFEACHDATAINBYTE, "getSizeOfEachDataInByte");
    LOAD_DLL_FUNC(fGetDataValue, DATAVALUE, "getDataValue");
    LOAD_DLL_FUNC(fGetTriggerCount, TRIGGERCOUNT, "getTriggerCount");
    LOAD_DLL_FUNC(fGetTriggerSample, TRIGGERSAMPLE, "getTriggerSample");
    LOAD_DLL_FUNC(fGetTriggerValue, TRIGGERVALUE, "getTriggerValue");
    LOAD_DLL_FUNC(fGetNoteCount, NOTECOUNT, "getNoteCount");
    LOAD_DLL_FUNC(fGetNoteSample, NOTESAMPLE, "getNoteSample");
    LOAD_DLL_FUNC(fGetNoteComment, NOTECOMMENT, "getNoteComment");
    LOAD_DLL_FUNC(fShowElectrode, SHOWELECTRODE, "show_Electrode");
    LOAD_DLL_FUNC(fShowNote, SHOWNOTE, "show_Note");
    LOAD_DLL_FUNC(fShowTrigger, SHOWTRIGGER, "show_Trigger");
    LOAD_DLL_FUNC(fShowSignal, SHOWSIGNAL, "showSignal");

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Succeeded in loading DLL: " << path << "\n";
    m_structHeader      = fGetStructHeader();
    m_structHeaderInfo  = fGetStructHeaderInfo();
    m_structBuffData    = fGetStructBuffData();
    m_structBuffNote    = fGetStructBuffNote();
    m_structBuffTrigger = fGetStructBuffTrigger();

#if 1
    if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_CURRENT_USER, registeryKeyName, 0, KEY_QUERY_VALUE, &registryKey)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key " << registeryKeyName << " is not initialized\n";
        strcpy(tcpPortNumber, "");
        strcpy(tcpSendAcq, "");
        strcpy(tcpServerName, "");
        return false;
    }

    DWORD taille = sizeof(tcpPortNumber);

    if (ERROR_SUCCESS != ::RegQueryValueEx(registryKey, "tcpPortNumber", nullptr, nullptr, reinterpret_cast<LPBYTE>(tcpPortNumber), &taille)) {
        strcpy(tcpPortNumber, "");
    }
    else { m_ServerHostPort = atoi(tcpPortNumber); }

    if (ERROR_SUCCESS != ::RegQueryValueEx(registryKey, "tcpSendAcq", nullptr, nullptr, reinterpret_cast<LPBYTE>(tcpSendAcq), &taille)) {
        strcpy(tcpSendAcq, "");
    }

    if (ERROR_SUCCESS != ::RegQueryValueEx(registryKey, "tcpServerName", nullptr, nullptr, reinterpret_cast<LPBYTE>(tcpServerName), &taille)) {
        strcpy(tcpServerName, "");
    }

    RegCloseKey(registryKey);
    registryKey = nullptr;
    //  g_bInitializedFromRegistry=true;
#endif

    return true;
}

short CDriverMicromedSystemPlusEvolution::myReceive(char* buf, const long dataLen)
{
    long nDati = 0;
    //get data contains in the temp buffer
    while (!m_tempBuffs.empty() && nDati < dataLen) {
        buf[nDati] = m_tempBuffs.front();
        m_tempBuffs.pop_front();
        nDati++;
    }

    while (nDati < dataLen) {
        const long recByte = m_Connection->receiveBuffer((&buf[nDati]), dataLen - nDati);
        if (recByte == 0) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "No data was received, check if the device is still connected\n";
            return -1;
        }

        nDati += recByte;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Received Data: = " << nDati << " /" << dataLen << "\n";
    }
    return 0;
}

bool CDriverMicromedSystemPlusEvolution::receiveAllHeader()
{
    do {
        // Receive Header
        if (this->myReceive(m_structHeader, fGetStructHeaderSize()) == -1) { return false; }
        if (fIsHeaderValid()) {
            char* header = new char[fGetStructHeaderSize()];
            memcpy(header, m_structHeader, fGetStructHeaderSize());
            m_headers.push_back(header);
        }
        else {
            //if no header was found, its impossible to find the next data block
            if (m_headers.empty()) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Header received not in correct form\n";
                return false;
            }

            //save data in the temp buffer
            for (int i = fGetStructHeaderSize(); i > 0; i--) { m_tempBuffs.push_front(m_structHeader[i - 1]); }
        }
    } while (fIsHeaderValid());

    return true;
}

bool CDriverMicromedSystemPlusEvolution::loadNextHeader()
{
    //get the next header
    memcpy(m_structHeader, m_headers.back(), fGetStructHeaderSize());

    //load next data
    char* temp = new char[fGetStructHeaderSize()];
    if (this->myReceive(temp, fGetStructHeaderSize()) == -1) { return false; }
    //check if the next data correspond to the current header
    while (m_headers.size() > 1 && ((fIsNoteHeader() && strncmp(&(temp[4]), "Note", 4) != 0) || (!fIsNoteHeader() && strncmp(&(temp[4]), "Note", 4) == 0))) {
        m_headers.push_front(m_headers.back());
        m_headers.pop_back();
        //load the last
        memcpy(m_structHeader, m_headers.back(), fGetStructHeaderSize());
    }

    //save data in the temp buffer
    for (int i = fGetStructHeaderSize(); i > 0; i--) { m_tempBuffs.push_front(temp[i - 1]); }
    delete[] temp;
    delete[] m_headers.back();
    m_headers.pop_back();
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverMicromedSystemPlusEvolution::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (!m_valid || m_driverCtx.isConnected()) { return false; }
    if (!libMicromed && !loadDLL()) { return false; }

    //update register key
    if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_CURRENT_USER, registeryKeyName, 0, KEY_WRITE, &registryKey)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key not initialized\n";
    }
    else {
        char hostPort[1024];
        sprintf(hostPort, "%i", m_ServerHostPort);
        //const std::string port = std::to_string(m_ServerHostPort);
        //if (ERROR_SUCCESS != ::RegSetValueEx(registryKey, "tcpPortNumber", 0, REG_SZ, reinterpret_cast<LPBYTE>(const_cast<char*>(port.c_str())), port.size()))

        if (ERROR_SUCCESS != ::RegSetValueEx(registryKey, "tcpPortNumber", 0, REG_SZ, reinterpret_cast<LPBYTE>(hostPort), strlen(hostPort))) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpPortNumber not initialized to '" << m_ServerHostPort << "'\n";
        }

        if (tcpSendAcq == std::string()) {
            char* acq = "1";
            if (ERROR_SUCCESS != ::RegSetValueEx(registryKey, "tcpSendAcq", 0, REG_SZ, reinterpret_cast<LPBYTE>(acq), 1)) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpSendAcq not initialized to '1'\n";
            }
        }

        if (tcpServerName == std::string()) {
            strcpy(tcpServerName, "localhost");
            if (ERROR_SUCCESS != ::RegSetValueEx(registryKey, "tcpServerName", 0, REG_SZ, reinterpret_cast<LPBYTE>(tcpServerName), strlen(tcpServerName))) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Registery key tcpServerName not initialized to '" << tcpServerName << "'\n";
            }
        }

        RegCloseKey(registryKey);
        registryKey = nullptr;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Configure Register key\n";

    // Builds up server connection
    m_ConnectionServer = Socket::createConnectionServer();

    if (!m_ConnectionServer) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "> Could not create server socket\n";
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Server is on \n";

    // Server start listening on defined port
    if (!m_ConnectionServer->listen(m_ServerHostPort)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "> Could not listen TCP port " << m_ServerHostPort << "\n";

        // Cleans up server connection
        m_ConnectionServer->close();
        m_ConnectionServer->release();
        m_ConnectionServer = nullptr;

        return false;
    }

    m_nSamplePerSentBlock = nSamplePerSentBlock;
    m_callback            = &callback;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Server is listening on port : " << m_ServerHostPort << "\n";

    if (m_ConnectionServer->isReadyToReceive(m_timeOutMilliseconds)) {
        // Accept new client
        m_Connection = m_ConnectionServer->accept();
    }
    else {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "> Time out after " << m_timeOutMilliseconds << " milliseconds\n";

        // Cleans up server connection
        m_ConnectionServer->close();
        m_ConnectionServer->release();
        m_ConnectionServer = nullptr;

        return false;
    }

    // Receive Header
    if (this->myReceive(m_structHeader, fGetStructHeaderSize()) == -1) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Receiving Header....\n";

    // Verify header validity
    if (!fIsHeaderValid()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Header received not in correct form : pb with fixCode\n";
        return false;
    }

    if (!fIsInitHeader()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Header received not in correct form : pb not receive Init information\n";
        return false;
    }

    if (fGetStructHeaderInfoSize() != fGetDataLength()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                "Header received not in correct form : pb the data header Info hasn't the good size\n the structure size:" << fGetStructHeaderInfoSize() <<
                "the size of data received:" << fGetDataLength() << "\n";
        return false;
    }

    // Receive Header
    if (this->myReceive(m_structHeaderInfo, fGetStructHeaderInfoSize()) == -1) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Header received\n";

    m_header.setChannelCount(fGetNbOfChannels());

    m_header.setSamplingFrequency(uint32_t(fGetMinimumSamplingRate()));

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "size for 1 channel, 1 block: " << m_nSamplePerSentBlock << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "number of channels: " << m_header.getChannelCount() << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Maximum sample rate =" << m_header.getSamplingFrequency() << " Hz" << "\n";
    m_sample = new float[m_header.getChannelCount() * m_nSamplePerSentBlock];
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "size of m_sample="
            << (m_header.getChannelCount() * m_nSamplePerSentBlock * sizeof(float)) << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Maximum Buffer size =" << (
        m_header.getChannelCount() * m_nSamplePerSentBlock * sizeof(signed short int)) << " Samples" << "\n";

    if (!m_sample) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not allocate sample buffer\n";
        uninitialize();
        return false;
    }
    m_buffDataIdx                  = 0;
    m_posFirstSampleOfCurrentBlock = 0;
    if (m_driverCtx.getLogManager().isActive(Kernel::LogLevel_Debug)) {
        std::stringstream info;
        fShowElectrode(&info);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << info.str();
        fShowNote(&info);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << info.str();
        fShowTrigger(&info);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << info.str();
    }
    return true;
}

bool CDriverMicromedSystemPlusEvolution::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "start device\n";
    if (!m_valid || !m_driverCtx.isConnected() || m_driverCtx.isStarted() || !m_Connection) { return false; }
    m_nSamplesBlock = m_header.getChannelCount() * m_nSamplePerSentBlock;
    m_sizeInByte    = fGetSizeOfEachDataInByte();
    //calculate the number max of complete samples can be contains in the buffer.
    m_buffSize = fGetStructBuffDataSize() / (m_sizeInByte * m_header.getChannelCount());
    m_buffSize = m_buffSize * (m_sizeInByte * m_header.getChannelCount());
    return true;
}

bool CDriverMicromedSystemPlusEvolution::dropData()
{
    //drop data
    uint32_t totalReceived = 0;

    do {
        const uint32_t maxByteRecv = std::min(uint32_t(fGetStructBuffDataSize()), uint32_t(fGetDataLength() - totalReceived));
        if (this->myReceive((char*)m_structBuffData, maxByteRecv) == -1) { return false; }
        totalReceived += maxByteRecv;
    } while (totalReceived < fGetDataLength());
    return true;
}

bool CDriverMicromedSystemPlusEvolution::loop()
{
    if (!m_valid || !m_driverCtx.isConnected()) { return false; }

    if (m_Connection) {
        // Receive All consecutive Header
        //this->myReceive(m_structHeader, fGetStructHeaderSize());
        if (!this->receiveAllHeader()) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "No Header received, an error was occurred during the data acquisition!\n";
            return false;
        }
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Number header received: " << m_headers.size() << "\n";

        while (!m_headers.empty()) {
            if (!loadNextHeader()) { return false; }
            // Verify header validity
            if (!fIsHeaderValid()) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Header received not in correct form\n";
                return false;
            }
            if (!fIsDataHeader() && !fIsNoteHeader() && !fIsTriggerHeader()) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Header received not in correct form : problem with infoType\n";
                return false;
            }

            //if(m_driverCtx.isStarted())
            //{
            if (fIsDataHeader()) {
                //if the device is not start or the first block after start haven't been received, data will be dropped
                if (!m_driverCtx.isStarted()) {
                    dropData();
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Device not started, dropped data: data.len = " << fGetDataLength() << "\n";
                    return true;
                }
                // Receive Data
                uint32_t maxByteRecv         = 0;
                uint32_t totalReceived       = 0;
                uint32_t receivedSampleCount = 0;
                do {
                    maxByteRecv = std::min(m_buffSize, std::min(fGetDataLength() - totalReceived,
                                                                uint32_t(m_nSamplesBlock - m_buffDataIdx * m_header.getChannelCount()) * m_sizeInByte));
                    if (this->myReceive((char*)m_structBuffData, maxByteRecv) == -1) { return false; }
                    receivedSampleCount = maxByteRecv / (m_sizeInByte * m_header.getChannelCount());
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Number of Samples Received:" << receivedSampleCount << "\n";

                    for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
                        for (uint32_t j = 0; j < receivedSampleCount; ++j) {
                            m_sample[m_buffDataIdx + j + i * m_nSamplePerSentBlock] = float(fGetDataValue(i, j));
                        }
                    }

                    totalReceived += maxByteRecv;
                    m_buffDataIdx += receivedSampleCount;
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Convert Data: dataConvert = " << totalReceived << "/" << fGetDataLength() << "\n";

                    if (m_nSamplesBlock < m_buffDataIdx) {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Data not received in correct form : problem with lenData\n";
                        return false;
                    }

                    if (m_nSamplesBlock == m_buffDataIdx * m_header.getChannelCount()) {
                        m_callback->setSamples(m_sample);
                        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Send samples back to CAcquisitionServer: samples.len = "
                                << m_buffDataIdx << "\n";
                        if (m_stimSet.size() > 0) {
                            m_callback->setStimulationSet(m_stimSet);
                            m_stimSet.clear();
                        }
                        m_posFirstSampleOfCurrentBlock += m_nSamplesBlock;
                        m_buffDataIdx = 0;
                    }
                } while (totalReceived < fGetDataLength());
                if (m_driverCtx.getLogManager().isActive(Kernel::LogLevel_Debug)) {
                    std::stringstream signal;
                    fShowSignal(&signal);
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << signal.str();
                }
            }
            else if (fIsNoteHeader()) {
                if (this->myReceive((char*)m_structBuffNote, long(fGetDataLength())) == -1) { return false; }
                std::stringstream note;
                fShowNote(&note);
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << note.str();
            }
            else if (fIsTriggerHeader()) {
                if (this->myReceive((char*)m_structBuffTrigger, long(fGetDataLength())) == -1) { return false; }

                for (int i = 0; i < fGetTriggerCount(); ++i) {
                    uint32_t sample = fGetTriggerSample(i);
                    if (sample < m_posFirstSampleOfCurrentBlock) {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning
                                << " A trigger was received too late! this trigger will not be send to the acquisition server.";
                    }
                    else {
                        uint32_t pos  = uint32_t(sample - m_posFirstSampleOfCurrentBlock);
                        uint64_t time = CTime(m_header.getSamplingFrequency(), uint64_t(pos)).time();
                        m_stimSet.push_back(fGetTriggerValue(i), time, 0);
                    }
                }

                if (m_driverCtx.getLogManager().isActive(Kernel::LogLevel_Trace)) {
                    std::stringstream trigger;
                    fShowTrigger(&trigger);
                    //m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "A Trigger was received but this function is not implemented. Please submit a bug report (including the acquisition server log file in debug mode)";
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << trigger.str();
                }
            }
        }
    }
    return true;
}

bool CDriverMicromedSystemPlusEvolution::stop()
{
    if (!m_valid) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Server stopped\n";

    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverMicromedSystemPlusEvolution::uninitialize()
{
    if (!m_valid || !m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_sample) {
        delete [] m_sample;
        m_sample   = nullptr;
        m_callback = nullptr;
    }

    // Cleans up client connection
    if (m_Connection) {
        m_Connection->close();
        m_Connection->release();
        m_Connection = nullptr;
    }

    // Cleans up server connection
    if (m_ConnectionServer) {
        m_ConnectionServer->close();
        m_ConnectionServer->release();
        m_ConnectionServer = nullptr;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Server disconnected\n";

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverMicromedSystemPlusEvolution::configure()
{
    if (!libMicromed) { loadDLL(); }

    CConfigurationNetworkBuilder config(Directories::getDataDir() + "/applications/acquisition-server/interface-Micromed-SystemPlusEvolution.ui");
    config.setHostPort(m_ServerHostPort);

    if (config.configure(m_header)) {
        m_ServerHostPort = config.getHostPort();
        m_settings.save();
        return true;
    }

    return false;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif  // #if defined TARGET_OS_Windows
#endif  // #if defined(TARGET_HAS_ThirdPartyMicromed)
