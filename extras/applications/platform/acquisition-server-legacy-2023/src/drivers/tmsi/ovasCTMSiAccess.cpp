///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#if defined TARGET_HAS_ThirdPartyTMSi

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include "ovasCTMSiAccess.h"

#include <iostream>
#include <string>
#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {

static const std::string TMSI_DLL = "TMSiSDK.dll";
//static const unsigned long s_ulLengthOfBufferInSeconds = 10;
static const int CALIBRATION_VOLTAGE = IC_VOLT_050;
//static const int s_iImpedanceLimit                     = IC_OHM_005;

HINSTANCE libTMSi; // Library Handle

template <typename T>
void CTMSiAccess::loadDLLFunct(T* functionPointer, const char* functionName)
{
    *functionPointer = T(GetProcAddress(libTMSi, functionName));
    if (!*functionPointer) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "(TMSi) Load method " << functionName << "\n";
        m_valid = false;
    }
}

CTMSiAccess::CTMSiAccess(IDriverContext& ctx)
    : m_driverCtx(ctx)
{
    // Create a map of available protocols, each protocol has an Enum value coming from TMSi and an index (used for the dropbown box)
    m_connectionProtocols["USB"]       = std::make_pair(TMSiConnectionUSB, 0);
    m_connectionProtocols["WiFi"]      = std::make_pair(TMSiConnectionWifi, 1);
    m_connectionProtocols["Network"]   = std::make_pair(TMSiConnectionNetwork, 2);
    m_connectionProtocols["Bluetooth"] = std::make_pair(TMSiConnectionBluetooth, 3);

    m_libraryHandle = nullptr;
    m_sampleBuffer  = nullptr;
    m_signalFormat  = nullptr;

    // Load the Mensia Acquisition Library
    const std::string path = std::string(m_driverCtx.getConfigurationManager().expand("${Path_Bin}").toASCIIString()) + "/" + TMSI_DLL;
    libTMSi                = ::LoadLibrary(path.c_str());

    if (libTMSi == nullptr) {
        const DWORD windowsError = GetLastError();
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "(TMSi) Can not load library, windows error = " << uint64_t(windowsError) << "\n";
    }

    m_valid = true;

    loadDLLFunct<POPEN>(&m_fpOpen, "Open");
    loadDLLFunct<PCLOSE>(&m_fpClose, "Close");
    loadDLLFunct<PSTART>(&m_fpStart, "Start");
    loadDLLFunct<PSTOP>(&m_fpStop, "Stop");
    loadDLLFunct<PSETSIGNALBUFFER>(&m_fpSetSignalBuffer, "SetSignalBuffer");
    loadDLLFunct<PGETBUFFERINFO>(&m_fpGetBufferInfo, "GetBufferInfo");
    loadDLLFunct<PGETSAMPLES>(&m_fpGetSamples, "GetSamples");
    loadDLLFunct<PGETSIGNALFORMAT>(&m_fpGetSignalFormat, "GetSignalFormat");
    loadDLLFunct<PFREE>(&m_fpFree, "Free");
    loadDLLFunct<PLIBRARYINIT>(&m_fpLibraryInit, "LibraryInit");
    loadDLLFunct<PLIBRARYEXIT>(&m_fpLibraryExit, "LibraryExit");
    loadDLLFunct<PGETFRONTENDINFO>(&m_fpGetFrontEndInfo, "GetFrontEndInfo");

    loadDLLFunct<PSETRTCTIME>(&m_fpSetRtcTime, "SetRtcTime");
    loadDLLFunct<PGETRTCTIME>(&m_fpGetRtcTime, "GetRtcTime");
    loadDLLFunct<PGETERRORCODE>(&m_fpGetErrorCode, "GetErrorCode");
    loadDLLFunct<PGETERRORCODEMESSAGE>(&m_fpGetErrorCodeMessage, "GetErrorCodeMessage");

    loadDLLFunct<PGETDEVICELIST>(&m_fpGetDeviceList, "GetDeviceList");
    loadDLLFunct<PFREEDEVICELIST>(&m_fpFreeDeviceList, "FreeDeviceList");

    loadDLLFunct<PSETREFCALCULATION>(&m_fpSetRefCalculation, "SetRefCalculation");
    loadDLLFunct<PGETCONNECTIONPROPERTIES>(&m_fpGetConnectionProperties, "GetConnectionProperties");
    loadDLLFunct<PSETMEASURINGMODE>(&m_fpSetMeasuringMode, "SetMeasuringMode");
    loadDLLFunct<PGETEXTFRONTENDINFO>(&m_fpGetExtFrontEndInfo, "GetExtFrontEndInfo");

    /*
    // NeXus10MkII functionality
    loadDLLFunct<PGETRANDOMKEY>(&m_fpGetRandomKey, "GetRandomKey");
    loadDLLFunct<PUNLOCKFRONTEND>(&m_fpUnlockFrontEnd, "UnlockFrontEnd");
    loadDLLFunct<PGETOEMSIZE>(&m_fpGetOEMSize, "GetOEMSize");
    loadDLLFunct<PSETOEMDATA>(&m_fpSetOEMData, "SetOEMData");
    loadDLLFunct<PGETOEMDATA>(&m_fpGetOEMData, "GetOEMData");
    loadDLLFunct<POPENFIRSTDEVICE>(&m_oFopenFirstDevice, "OpenFirstDevice");
    loadDLLFunct<PSETSTORAGEMODE>(&m_fpSetStorageMode, "SetStorageMode");
    loadDLLFunct<PGETFLASHSTATUS>(&m_fpGetFlashStatus, "GetFlashStatus");
    loadDLLFunct<PSTARTFLASHDATA>(&m_fpStartFlashData, "StartFlashData");
    loadDLLFunct<PGETFLASHSAMPLES>(&m_fpGetFlashSamples, "GetFlashSamples");
    loadDLLFunct<PSTOPFLASHDATA>(&m_fpStopFlashData, "StopFlashData");
    loadDLLFunct<PFLASHERASEMEMORY>(&m_fpFlashEraseMemory, "FlashEraseMemory");
    loadDLLFunct<PSETFLASHDATA>(&m_fpSetFlashData, "SetFlashData");
*/
}

CTMSiAccess::~CTMSiAccess()
{
    // Close the intefrace if it is still open, should prevent device locking in some cases
    if (m_opened) { closeFrontEnd(); }
    if (m_libraryHandle != nullptr) { m_fpLibraryExit(m_libraryHandle); }
    FreeLibrary(libTMSi);
}

bool CTMSiAccess::initializeTMSiLibrary(const char* connectionProtocol)
{
    if (!m_valid) { return false; }

    // TODO_JL block this function if the driver is in use

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "(TMSi) Initializing TMSi library on [" << connectionProtocol << "] protocol \n";
    const ETmSiConnection connection = m_connectionProtocols[connectionProtocol].first;

    int error;

    if (m_libraryHandle != nullptr) {
        // if the library is already in use then exit it first
        m_fpLibraryExit(m_libraryHandle);
        m_libraryHandle = nullptr;
    }

    m_libraryHandle = m_fpLibraryInit(connection, &error);

    if (error != 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "(TMSi) Can not initialize TMSi library, errorcode = " << error << "\n";
        m_libraryHandle = nullptr;
        return false;
    }

    if (m_libraryHandle == INVALID_HANDLE_VALUE) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "(TMSi) Can not initialize TMSi library, INVALID_ERROR_HANDLE\n";
        m_libraryHandle = nullptr;
        return false;
    }

    int nDevices;
    // Once the library is initialized, get the list of devices on the frontend
    char** deviceList = m_fpGetDeviceList(m_libraryHandle, &nDevices);
    m_devices.clear();

    if (nDevices == 0) {
        error = m_fpGetErrorCode(m_libraryHandle);
        if (error == 0) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "(TMSi) No TMSi devices connected\n"; }
        else {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Could not list TMSi devices, errorcode = " << error
                    << " message : \"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        }

        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "(TMSi) Found [" << nDevices << "] TMSi devices\n";

    for (int i = 0; i < nDevices; ++i) { m_devices.push_back(CString(deviceList[i])); }

    // the device list allocated in the library has to be freed
    m_fpFreeDeviceList(m_libraryHandle, nDevices, deviceList);

    m_isInitialized = true;
    return true;
}

bool CTMSiAccess::openFrontEnd(const char* deviceID)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Opening FrontEnd [" << deviceID << "]\n";
    if (m_libraryHandle == nullptr) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) TMSi library not accessible\n";
        return false;
    }

    if (m_fpOpen(m_libraryHandle, deviceID)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend opened\n"; }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Frontend NOT available, errorcode = " << error << ", message=\"" <<
                m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";

        // If we receive "Access Refused (10061)" error we show a dialog prompting the user to restart his device
        GtkWidget* gtkMessageDialog = error == 10061
                                          ? gtk_message_dialog_new_with_markup(nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
                                                                               "The device <b>%s</b> is present but refuses access. It might be connected to another instance of OpenViBE. Restarting the device might help",
                                                                               deviceID)
                                          : gtk_message_dialog_new_with_markup(nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
                                                                               "The device <b>%s</b> is present but can not be opened. Restarting the device might help. Error reported by driver : %s",
                                                                               deviceID, m_fpGetErrorCodeMessage(m_libraryHandle, error));

        gtk_dialog_run(GTK_DIALOG(gtkMessageDialog));
        gtk_widget_destroy(gtkMessageDialog);

        // if we fail to open the frontend we close the library immediately
        //m_fpLibraryExit(m_libraryHandle);
        //m_libraryHandle = nullptr;
        return false;
    }

    m_opened = true;
    return true;
}

bool CTMSiAccess::closeFrontEnd()
{
    if (m_libraryHandle == nullptr) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) TMSi library not accessible\n";
        return false;
    }

    if (!m_opened) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not close unopened FrontEnd\n";
        return false;
    }

    if (m_fpClose(m_libraryHandle)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend closed\n"; }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not close frontend, errorcode = " << error << ", message=\"" <<
                m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        return false;
    }

    m_opened              = false;
    m_hasChannelStructure = false;
    m_hasBufferSet        = false;
    return true;
}

std::vector<unsigned long> CTMSiAccess::discoverDeviceSamplingFrequencies()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Checking for sampling frequencies\n";

    std::vector<unsigned long> samplings;

    m_maxBufferSize = MAX_BUFFER_SIZE;
    samplings.push_back(128 * 1000);
    samplings.push_back(256 * 1000);
    samplings.push_back(512 * 1000);
    samplings.push_back(1024 * 1000);
    samplings.push_back(2048 * 1000);

    if (!m_opened) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) No FrontEnd opened\n";
        samplings.clear();
        return samplings;
    }

    for (size_t i = 0; i < samplings.size(); ++i) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Trying frequency [" << uint64_t(samplings[i]) << "]\n";

        unsigned long sampling = samplings[i];

        // Max bufer size will be set to the right value in this step
        if (m_fpSetSignalBuffer(m_libraryHandle, &sampling, &m_maxBufferSize)) {
            samplings[i] = sampling;
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Sample rate: " << uint64_t(sampling) << ", Buffer Size: " <<
                    uint64_t(m_maxBufferSize) << "\n";
        }
        else {
            const int error = m_fpGetErrorCode(m_libraryHandle);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "(TMSi) Failed to set sampling frequency, errorcode = " << error
                    << ", message = \"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
            samplings.clear();
            return samplings;
        }
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Sampling frequencies OK\n";
    return samplings;
}

bool CTMSiAccess::runDiagnostics()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Running diagnostics\n";

    if (!m_opened) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) No FrontEnd opened\n";
        return false;
    }

    front_end_info_t frontEndInfo;

    if (m_fpGetFrontEndInfo(m_libraryHandle, &frontEndInfo)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend has Serial " << size_t(frontEndInfo.Serial) << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend has HwVersion " << frontEndInfo.HwVersion << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend has SwVersion " << frontEndInfo.SwVersion << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend has BaseSf " << frontEndInfo.BaseSf << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend has maxRS232 " << frontEndInfo.maxRS232 << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend has " << frontEndInfo.NrOfChannels << " channels\n";
    }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) FrontendInfo NOT available, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        // return false;
    }

    tmsi_bat_report_type_t TMSiBatReport;
    tmsi_storage_report_type_t TMSiStorageReport;
    tmsi_device_report_type_t TMSiDeviceReport;
    tmsi_ext_frontend_info_type_t TMSiExtFrontEndInfo;

    if (m_fpGetExtFrontEndInfo(m_libraryHandle, &TMSiExtFrontEndInfo, &TMSiBatReport, &TMSiStorageReport, &TMSiDeviceReport)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) CurrentSamplerate " << TMSiExtFrontEndInfo.CurrentSamplerate << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) CurrentBlockType " << TMSiExtFrontEndInfo.CurrentBlockType << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) CurrentInterface " << TMSiExtFrontEndInfo.CurrentInterface << "\n";

        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) MemoryStatus.TotalSize " << TMSiStorageReport.TotalSize << " MByte\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) MemoryStatus.UsedSpace " << TMSiStorageReport.UsedSpace << " MByte\n";

        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) BatteryStatus.AccumCurrent " << TMSiBatReport.AccumCurrent << " mA\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) BatteryStatus.Current " << TMSiBatReport.Current << " mA\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) BatteryStatus.Temp " << TMSiBatReport.Temp << " C\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) BatteryStatus.Voltage " << TMSiBatReport.Voltage << " mV\n";

        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) STmSiDeviceReport.AdapterSerial " << TMSiDeviceReport.AdapterSN << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) STmSiDeviceReport.AdapterCycles " << TMSiDeviceReport.AdapterCycles << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) STmSiDeviceReport.AdapterStatus " << TMSiDeviceReport.AdapterStatus << "\n";

        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) STmSiDeviceReport.MobitaCycles " << TMSiDeviceReport.MobitaCycles << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) STmSiDeviceReport.MobitaStatus " << TMSiDeviceReport.MobitaStatus << "\n";
    }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        // Most of the devices do not have the extended frontend info so this message will popup quite often
        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "(TMSi) Extended FrontendInfo NOT available, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        // return false;
    }

    // only fails if the device is not opened, as not all devices support the getFrontEndInfo and getExtFrontEndInfo functions
    //m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Diagnostics OK\n";
    return true;
}

bool CTMSiAccess::getImpedanceTestingCapability(bool* hasImpedanceTestingAbility) const
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Getting impedance capability\n";

    if (!m_opened) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) No FrontEnd opened\n";
        return false;
    }

    front_end_info_t frontEndInfo;

    if (m_fpGetFrontEndInfo(m_libraryHandle, &frontEndInfo)) {
        // get the first three numbers from the serial number
        // TMSi devices that can measure impedance have their serial numbers starting by a number between 107 and 128
        char serialNumber[1024];
        sprintf(serialNumber, "%lu", frontEndInfo.Serial);

        char deviceIdentifier[4];
        strncpy(deviceIdentifier, serialNumber, 3);
        deviceIdentifier[3] = 0;

        const int deviceID = atoi(deviceIdentifier);

        *hasImpedanceTestingAbility = (deviceID >= 107 && deviceID <= 128);
    }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) FrontendInfo NOT available, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        // return false;
    }
    return true;
}

bool CTMSiAccess::calculateSignalFormat(const char* deviceID)
{
    if (!m_opened) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) No FrontEnd opened\n";
        return false;
    }

    m_hasChannelStructure = false;
    m_nActualChannel      = 0;
    m_nMaxEEGChannel      = 0;

    char deviceIdentifier[1024];
    strcpy(deviceIdentifier, deviceID);
    m_signalFormat = m_fpGetSignalFormat(m_libraryHandle, deviceIdentifier);

    if (m_signalFormat == nullptr) {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi)Error getting channel format, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        return false;
    }

    // Count the EEG channels
    m_nMaxEEGChannel = 0;

    m_nActualChannel = m_signalFormat->Elements;
    // go through the channels and find the number of EEG channels and additional channels
    // NOTE that this ONLY work if all EEG channels are listed first
    for (size_t i = 0; i < size_t(m_signalFormat->Elements); ++i) {
        if (m_signalFormat[i].Type == CHANNELTYPE_EXG) { m_nMaxEEGChannel++; }
        else if (m_signalFormat[i].Type == CHANNELTYPE_BIP) { m_nMaxEEGChannel++; }
        else if (m_signalFormat[i].Type == CHANNELTYPE_AUX) { m_nMaxEEGChannel++; }
    }

    m_hasChannelStructure = true;

    // the pointer is initialized and held by the library so we can store it
    return true;
}

bool CTMSiAccess::printSignalFormat()
{
    if (m_signalFormat != nullptr) {
        for (int i = 0; i < int(m_signalFormat->Elements); ++i) {
            char channelInfo[1024];
            CString channelName = getChannelName(i);

            sprintf(channelInfo, "%3d: %s Format %d Type %d Bytes %d Subtype %d UnitId %d UnitExponent %d", i, channelName.toASCIIString(),
                    int(m_signalFormat[i].Format), int(m_signalFormat[i].Type), int(m_signalFormat[i].Bytes), int(m_signalFormat[i].SubType),
                    int(m_signalFormat[i].UnitId), int(m_signalFormat[i].UnitExponent));

            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) " << channelInfo << "\n";
        }
    }
    else { return false; }

    return true;
}

CString CTMSiAccess::getChannelName(const size_t index) const
{
    if (m_signalFormat == nullptr || index >= m_signalFormat->Elements) { return ""; }

    char channelName[SIGNAL_NAME + 1];
    for (int i = 0; i < SIGNAL_NAME; ++i) { channelName[i] = char(m_signalFormat[index].Name[i]); }
    channelName[SIGNAL_NAME] = '\0';

    return CString(channelName);
}

CString CTMSiAccess::getChannelType(const size_t index) const
{
    if (m_signalFormat == nullptr || index >= m_signalFormat->Elements) { return ""; }

    switch (m_signalFormat[index].Type) {
        case CHANNELTYPE_UNKNOWN: return "Unknown";
        case CHANNELTYPE_EXG: return "EXG";
        case CHANNELTYPE_BIP: return "BIP";
        case CHANNELTYPE_AUX: return "AUX";
        case CHANNELTYPE_DIG: return "DIG";
        case CHANNELTYPE_TIME: return "TIME";
        case CHANNELTYPE_LEAK: return "LEAK";
        case CHANNELTYPE_PRESSURE: return "PRESSURE";
        case CHANNELTYPE_ENVELOPE: return "ENVELOPE";
        case CHANNELTYPE_MARKER: return "MARKER";
        case CHANNELTYPE_SAW: return "RAMP";
        case CHANNELTYPE_SAO2: return "SAO2";
        default: return "Unknown";
    }
}

void CTMSiAccess::freeSignalFormat()
{
    if (m_signalFormat != nullptr) {
        m_fpFree(m_signalFormat);
        m_signalFormat = nullptr;
    }
}

bool CTMSiAccess::setCommonModeRejection(const bool isCommonModeRejectionEnabled)
{
    const int statusCAR = isCommonModeRejectionEnabled ? 1 : 0;

    if (m_fpSetRefCalculation(m_libraryHandle, statusCAR)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Common mode rejection set to " << statusCAR << "\n";
    }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not set common mode rejection, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        return false;
    }

    return true;
}

bool CTMSiAccess::setActiveChannels(CHeader* header, const CString& additionalChannels)
{
    const size_t activeChannels = header->getChannelCount();
    bool hasRenamedAChannel     = false;

    m_isChannelsActivated.clear();
    m_isChannelsActivated.resize(m_nActualChannel, false);

    size_t nActiveAdditionalChannel = 0;

    for (size_t i = m_nMaxEEGChannel; i < m_nActualChannel; ++i) {
        if (std::string(additionalChannels.toASCIIString()).find(std::string(";") + std::string(getChannelName(i).toASCIIString()) + std::string(";"))
            != std::string::npos) {
            m_isChannelsActivated[i] = true;
            nActiveAdditionalChannel++;
        }
        else { m_isChannelsActivated[i] = false; }
    }

    const size_t activeEEGChannelIdx = activeChannels - nActiveAdditionalChannel;

    for (size_t i = 0; i < activeEEGChannelIdx; ++i) {
        m_isChannelsActivated[i] = true;
        if (strcmp(header->getChannelName(i), "") != 0) { hasRenamedAChannel = true; }
    }
    for (size_t i = activeEEGChannelIdx; i < m_nMaxEEGChannel; ++i) { m_isChannelsActivated[i] = false; }

    m_nActiveChannel = activeChannels;

    // set names of EEG channels if none were renamed
    if (!hasRenamedAChannel) { for (size_t i = 0; i < activeEEGChannelIdx; i++) { header->setChannelName(i, getChannelName(i)); } }

    // set names of additional channels
    size_t currentChannel = activeEEGChannelIdx;
    for (size_t i = m_nMaxEEGChannel; i < m_nActualChannel; i++) {
        if (m_isChannelsActivated[i]) {
            header->setChannelName(currentChannel, getChannelName(i));
            currentChannel++;
        }
    }

    return true;
}

bool CTMSiAccess::setSignalBuffer(const unsigned long sampling, unsigned long bufferSizeInSamples)
{
    if (!m_opened) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) No FrontEnd opened\n";
        return false;
    }

    if (!m_hasChannelStructure) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) The SignalFormat structure was not initialized\n";
        return false;
    }

    // if the buffer size in samples is too big, change it automatically to the MAX for the current device
    if (bufferSizeInSamples > m_maxBufferSize) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) The desired buffer size is too large, setting to maximum for the device instead"
                << "\n";
        bufferSizeInSamples = m_maxBufferSize;
    }

    ULONG samplingFrequency = sampling;
    ULONG size              = bufferSizeInSamples;

    if (m_fpSetSignalBuffer(m_libraryHandle, &samplingFrequency, &size)) {
        if (samplingFrequency == sampling && size == bufferSizeInSamples) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) SignalBuffer set to " << size_t(samplingFrequency) << " " << size_t(size) << "\n";
        }
        else {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Frontend does not support this Sampling Rate/Buffer Size combination\n";
            return false;
        }
    }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not setSignalBuffer, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        return false;
    }

    if (m_sampleBuffer != nullptr) {
        delete[] m_sampleBuffer;
        m_sampleBuffer = nullptr;
    }


    m_signalBufferSizeInBytes = size * m_nActualChannel * sizeof(m_sampleBuffer[0]);
    m_sampleBuffer            = new unsigned long[m_signalBufferSizeInBytes];
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Allocating sample buffer with " << size_t(m_signalBufferSizeInBytes) << "\n";

    m_hasBufferSet = true;

    return true;
}

bool CTMSiAccess::setSignalMeasuringModeToCalibration() { return setSignalMeasuringMode(MEASURE_MODE_CALIBRATION_EX); }

bool CTMSiAccess::setSignalMeasuringModeToImpedanceCheck(const int limit) { return setSignalMeasuringMode(MEASURE_MODE_IMPEDANCE_EX, limit); }

bool CTMSiAccess::setSignalMeasuringModeToNormal() { return setSignalMeasuringMode(MEASURE_MODE_NORMAL); }

bool CTMSiAccess::startAcquisition()
{
    if (m_isStarted) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Acquisition has already been started\n";
        return true;
    }

    if (m_libraryHandle == nullptr) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) TMSi library not accessible\n";
        return false;
    }

    if (!m_hasBufferSet) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not start acquisition without setting buffer first\n";
        return false;
    }

    if (m_fpStart(m_libraryHandle)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Starting Acquisition\n"; }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not start acqusition, errorcode = " << error << ", message=\"" <<
                m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        return false;
    }

    m_lastSampleIndexInBuffer = 0;
    m_isStarted               = true;

    return true;
}

bool CTMSiAccess::stopAcquisition()
{
    if (m_libraryHandle == nullptr) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) TMSi library not accessible\n";
        return false;
    }

    if (!m_isStarted) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Acquisition has not been started yet\n";
        return true;
    }

    if (m_fpStop(m_libraryHandle)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Stopping Acquisition\n"; }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not stop acqusition, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        return false;
    }

    getConnectionProperties();
    m_lastSampleIndexInBuffer = 0;
    m_isStarted               = false;
    return true;
}

int CTMSiAccess::getSamples(float* samples, IDriverCallback* driverCB, const uint64_t nSamplePerSentBlock, const uint32_t sampling)
{
    // since this function is called all the time, we do not do safety checks
    const long nBytesReceived = m_fpGetSamples(m_libraryHandle, m_sampleBuffer, m_signalBufferSizeInBytes);

    if (nBytesReceived > 0) {
        const uint32_t samplesReceived = nBytesReceived / m_nActualChannel / sizeof(m_sampleBuffer[0]);

        for (uint32_t i = 0; i < samplesReceived; ++i) {
            uint32_t virtualChannel = 0;
            for (uint32_t c = 0; c < m_nActualChannel; ++c) {
                if (m_isChannelsActivated[c]) {
                    // pointer to the current sample inside the signal sample buffer
                    float* sampleValueInFloat = &samples[virtualChannel * nSamplePerSentBlock + m_lastSampleIndexInBuffer];
                    //                      std::cout << virtualChannel * m_nActiveChannel + m_sampleIdx << std::endl;

                    const psignal_format_t currentSampleSignalFormat = &m_signalFormat[c];

                    // Calculate the floating value from the received integer value
                    // For overflow of a analog channel, set the value to zero
                    if (m_sampleBuffer[c + i * m_nActualChannel] == OVERFLOW_32BITS &&
                        (currentSampleSignalFormat->Type == CHANNELTYPE_EXG ||
                         currentSampleSignalFormat->Type == CHANNELTYPE_BIP ||
                         currentSampleSignalFormat->Type == CHANNELTYPE_AUX)) {
                        *sampleValueInFloat = 0; // Set it to a value you find a good sign of a overflow
                    }

                    else {
                        switch (currentSampleSignalFormat->Format) {
                            case SF_UNSIGNED: // unsigned integer
                                *sampleValueInFloat = float(m_sampleBuffer[c + i * m_nActualChannel] * currentSampleSignalFormat->UnitGain
                                                            + currentSampleSignalFormat->UnitOffSet);
                                break ;
                            case SF_INTEGER: // signed integer
                                *sampleValueInFloat = float(int(m_sampleBuffer[c + i * m_nActualChannel]) * currentSampleSignalFormat->UnitGain
                                                            + currentSampleSignalFormat->UnitOffSet);
                                break ;
                            default:
                                *sampleValueInFloat = 0; // For unknown types, set the value to zero
                                break ;
                        }
                    }

                    virtualChannel++;
                }

                // process DIGI channel, to receive hardware stimulations
                if (m_signalFormat[c].Type == CHANNELTYPE_DIG) {
                    uint32_t trigger = m_sampleBuffer[i * m_nActualChannel + c];

                    // invert bits
                    trigger = ~trigger;

                    // mask for bits 0-7 (see Refa manual)
                    trigger &= 255;

                    // add 0x8100 to match OVTK_StimulationId_Label_00 stimulation code
                    trigger += 0x8100U;

                    // add stimulation to SimulationSet
                    if (m_lastTriggerValue != trigger) {
                        const uint64_t time = CTime(sampling, uint64_t(m_lastSampleIndexInBuffer)).time();
                        m_stimSet.push_back(trigger, time, 0);
                        m_lastTriggerValue = trigger;
                    }
                }
            }
            m_lastSampleIndexInBuffer++;

            if (m_lastSampleIndexInBuffer == nSamplePerSentBlock) {
                driverCB->setSamples(samples);
                driverCB->setStimulationSet(m_stimSet);
                m_stimSet.clear();
                m_lastSampleIndexInBuffer = 0;
                m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
            }
        }
    }
    else if (nBytesReceived == 0) {
        ULONG overflow, percentFull;
        const int status = m_fpGetBufferInfo(m_libraryHandle, &overflow, &percentFull);

        if (status != 0 && overflow > 0 && percentFull > 0) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Overflow " << int(overflow) << " PercentFull " << int(percentFull) << "\n";
        }
    }
    else {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not read data, errorcode = " << nBytesReceived
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, nBytesReceived) << "\"\n";

        const int code = m_fpGetErrorCode(m_libraryHandle);
        if (code != 0) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Additional error, errorcode = " << code
                    << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, code) << "\"\n";
        }
        return -1;
    }

    return nBytesReceived;
}

bool CTMSiAccess::getImpedanceValues(std::vector<double>* impedanceValues)
{
    impedanceValues->resize(m_nActualChannel);

    // since this function is called all the time, we do not do safety checks
    const long nBytesReceived = m_fpGetSamples(m_libraryHandle, m_sampleBuffer, m_signalBufferSizeInBytes);

    if (nBytesReceived > 0) {
        // We only look at the first sample in the whole buffer, no need to look at all values
        for (uint32_t i = 0; i < 1; ++i) {
            uint32_t virtualChannel = 0;
            for (uint32_t c = 0; c < m_nActualChannel; ++c) {
                if (m_isChannelsActivated[c]) {
                    // Impedance values are returned in MOhm it seems
                    (*impedanceValues)[virtualChannel] = m_sampleBuffer[c + i * m_nActualChannel] * 1000.0;
                    virtualChannel++;
                }
            }
        }
    }
    else if (nBytesReceived == 0) { }
    else {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Can not read impedance data, errorcode = " << nBytesReceived
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, nBytesReceived) << "\"\n";

        const int error = m_fpGetErrorCode(m_libraryHandle);
        if (error != 0) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Additional error, errorcode = " << error
                    << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        }
        return false;
    }

    return true;
}

bool CTMSiAccess::getConnectionProperties() const
{
    uint32_t signalStrength;
    uint32_t nCrcErrors;
    uint32_t nSampleBlocks;

    if (m_fpGetConnectionProperties(m_libraryHandle, &signalStrength, &nCrcErrors, &nSampleBlocks)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) fpGetConnectionProperties SignalStrength " << signalStrength
                << " NrOfCRCErrors " << nCrcErrors << " NrOfSampleBlocks " << nSampleBlocks << "\n";
    }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Error getting channel format, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        return false;
    }

    return true;
}

// PRIVATE METHODS

bool CTMSiAccess::setSignalMeasuringMode(const ULONG measuringMode, const int value)
{
    if (!m_opened) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) No FrontEnd opened\n";
        return false;
    }

    if (!m_isStarted) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) This method must be called after the frontend is started\n";
        return false;
    }

    int modeValue = 0;
    CString modeName("Normal");

    if (measuringMode == MEASURE_MODE_CALIBRATION_EX) {
        modeValue = CALIBRATION_VOLTAGE;
        modeName  = "Calibration";
    }
    else if (measuringMode == MEASURE_MODE_IMPEDANCE_EX) {
        modeValue = value;
        modeName  = "Impedance";
    }

    if (m_fpSetMeasuringMode(m_libraryHandle, measuringMode, modeValue)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) Device set to " << modeName << " mode [" << modeValue << "]\n";
    }
    else {
        const int error = m_fpGetErrorCode(m_libraryHandle);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) Failed setting the device to " << modeName << " mode, errorcode = " << error
                << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, error) << "\"\n";
        return false;
    }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
