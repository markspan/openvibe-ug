/**-------------------------------------------------------------------------------------------------
 * \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
 * Rights transferred to Inria, contract signed 21.11.2014
 *-------------------------------------------------------------------------------------------------
 */

#include "CDriverTMSi.h"

#include <CharmeleonMontage.h>

#include "toolkit/ovtk_defines.h"
#include "toolkit/ovtk_stimulations.h"

#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

//___________________________________________________________________//
// TMSi SDK declarations (TMSiSDK.dll is loaded at runtime)          //

namespace {
// Signal formats
constexpr ULONG SF_UNSIGNED = 0x0;
constexpr ULONG SF_INTEGER  = 0x1;

// Integer overflow value for analog channels
constexpr ULONG OVERFLOW_32BITS = 0x80000000UL;

constexpr size_t SIGNAL_NAME = 40;

constexpr ULONG CHANNELTYPE_UNKNOWN  = 0;
constexpr ULONG CHANNELTYPE_EXG      = 1;
constexpr ULONG CHANNELTYPE_BIP      = 2;
constexpr ULONG CHANNELTYPE_AUX      = 3;
constexpr ULONG CHANNELTYPE_DIG      = 4;
constexpr ULONG CHANNELTYPE_TIME     = 5;
constexpr ULONG CHANNELTYPE_LEAK     = 6;
constexpr ULONG CHANNELTYPE_PRESSURE = 7;
constexpr ULONG CHANNELTYPE_ENVELOPE = 8;
constexpr ULONG CHANNELTYPE_MARKER   = 9;
constexpr ULONG CHANNELTYPE_SAW      = 10;
constexpr ULONG CHANNELTYPE_SAO2     = 11;

constexpr ULONG MAX_BUFFER_SIZE = 0xFFFFFFFF;

typedef struct SSignalFormat
{
    ULONG Size;     // Size of this structure
    ULONG Elements; // Number of elements in list

    ULONG Type;    // One of the signal types above
    ULONG SubType; // One of the signal sub-types above
    ULONG Format;  // Float / Integer / Asci / Ect..
    ULONG Bytes;   // Number of bytes per sample including subsignals

    FLOAT UnitGain;
    FLOAT UnitOffSet;
    ULONG UnitId;
    LONG UnitExponent;

    WCHAR Name[SIGNAL_NAME];

    ULONG Port;
    WCHAR PortName[SIGNAL_NAME];
    ULONG SerialNumber;
} signal_format_t, *psignal_format_t;

typedef enum ETmSiConnection
{
    TMSiConnectionUndefined = 0,
    TMSiConnectionFiber,
    TMSiConnectionBluetooth,
    TMSiConnectionUSB,
    TMSiConnectionWifi,
    TMSiConnectionNetwork
} tmsi_connection_type_t;

typedef BOOLEAN (__stdcall * POPEN)(void* handle, const char* deviceLocator);
typedef BOOLEAN (__stdcall * PCLOSE)(HANDLE handle);
typedef BOOLEAN (__stdcall * PSTART)(HANDLE handle);
typedef BOOLEAN (__stdcall * PSTOP)(HANDLE handle);
typedef BOOLEAN (__stdcall * PSETSIGNALBUFFER)(HANDLE handle, PULONG sampling, PULONG size);
typedef LONG (__stdcall * PGETSAMPLES)(HANDLE handle, PULONG sampleBuffer, ULONG size);
typedef psignal_format_t (__stdcall * PGETSIGNALFORMAT)(HANDLE handle, char* frontEndName);
typedef BOOLEAN (__stdcall * PFREE)(VOID* memory);
typedef HANDLE (__stdcall * PLIBRARYINIT)(tmsi_connection_type_t givenConnectionType, int* errorCode);
typedef int (__stdcall * PLIBRARYEXIT)(HANDLE handle);
typedef int (__stdcall * PGETERRORCODE)(HANDLE handle);
typedef const char* (__stdcall * PGETERRORCODEMESSAGE)(HANDLE handle, int errorCode);
typedef char** (__stdcall * PGETDEVICELIST)(HANDLE handle, int* nFrontEnds);
typedef void (__stdcall * PFREEDEVICELIST)(HANDLE handle, int nFrontEnds, char** deviceList);
typedef BOOLEAN (__stdcall * PSETREFCALCULATION)(HANDLE handle, int onOrOff);

const char* const TMSI_DLL = "TMSiSDK.dll";

/// Shown in the device list, picks the first device the SDK reports.
const std::string FIRST_DEVICE = "(first device found)";

/// Seconds of signal the SDK buffers on its side.
constexpr uint32_t SDK_BUFFER_SECONDS = 10;

const std::vector<std::pair<std::string, tmsi_connection_type_t>> PROTOCOLS = {
    {"USB", TMSiConnectionUSB},
    {"WiFi", TMSiConnectionWifi},
    {"Network", TMSiConnectionNetwork},
    {"Bluetooth", TMSiConnectionBluetooth},
};

std::vector<std::string> protocolNames()
{
    std::vector<std::string> names;
    for (const auto& p : PROTOCOLS) { names.push_back(p.first); }
    return names;
}

std::string trim(const std::string& s)
{
    const size_t first = s.find_first_not_of(" \t");
    if (first == std::string::npos) { return ""; }
    return s.substr(first, s.find_last_not_of(" \t") - first + 1);
}

/// Splits "a;b; c" into {"a", "b", "c"}, dropping empty entries.
std::vector<std::string> splitNames(const std::string& s)
{
    std::vector<std::string> names;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ';')) {
        item = trim(item);
        if (!item.empty()) { names.push_back(item); }
    }
    return names;
}
}  // namespace

struct TMSiApi
{
    HMODULE module = nullptr;
    HANDLE handle  = nullptr;

    psignal_format_t signalFormat = nullptr;

    POPEN open                               = nullptr;
    PCLOSE close                             = nullptr;
    PSTART start                             = nullptr;
    PSTOP stop                               = nullptr;
    PSETSIGNALBUFFER setSignalBuffer         = nullptr;
    PGETSAMPLES getSamples                   = nullptr;
    PGETSIGNALFORMAT getSignalFormat         = nullptr;
    PFREE free                               = nullptr;
    PLIBRARYINIT libraryInit                 = nullptr;
    PLIBRARYEXIT libraryExit                 = nullptr;
    PGETERRORCODE getErrorCode               = nullptr;
    PGETERRORCODEMESSAGE getErrorCodeMessage = nullptr;
    PGETDEVICELIST getDeviceList             = nullptr;
    PFREEDEVICELIST freeDeviceList           = nullptr;
    PSETREFCALCULATION setRefCalculation     = nullptr;
};

//___________________________________________________________________//
//                                                                   //

CDriverTMSi::CDriverTMSi()
    : m_api(std::make_unique<TMSiApi>()),
      m_paramProtocol("ConnectionProtocol", "Connection protocol", protocolNames(), "USB"),
      m_paramDevice("DeviceIdentifier", "Device", {FIRST_DEVICE}, FIRST_DEVICE),
      m_paramAdditionalChannels("AdditionalChannels", "Additional channels (names, separated by ;). DIG channels listed here send triggers", ""),
      m_paramCommonModeRejection("CommonModeRejection", "Common mode rejection (common average reference)", true),
      m_paramMontage("Montage", "Charmeleon montage, selects and names the EEG channels instead of the channel count", "",
                     OpenViBE::Parameters::Read, {"Charmeleon montage (*.json)", "Any File (*)"})
{
    m_samplingRate = 512;
    m_channelCount = 32;

    m_parameters.push_back(&m_paramProtocol);
    m_parameters.push_back(&m_paramDevice);
    m_parameters.push_back(&m_samplingRate);
    m_parameters.push_back(&m_channelCount);
    m_parameters.push_back(&m_paramMontage);
    m_parameters.push_back(&m_paramAdditionalChannels);
    m_parameters.push_back(&m_paramCommonModeRejection);
}

CDriverTMSi::~CDriverTMSi()
{
    // Close the interface if it is still open, should prevent device locking in some cases
    closeFrontEnd();
    exitLibrary();
    if (m_api->module) { FreeLibrary(m_api->module); }
}

OpenViBE::Parameters::Parameters CDriverTMSi::getParameters()
{
    // Refresh the device list, but never touch the SDK while a device is in use
    if (!m_opened && initLibrary()) {
        std::vector<std::string> devices{FIRST_DEVICE};
        devices.insert(devices.end(), m_devices.begin(), m_devices.end());
        m_paramDevice.setAvailableValues(devices);
    }
    return AbstractDriver::getParameters();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverTMSi::loadLibrary()
{
    if (m_api->module) { return true; }

    HMODULE module = LoadLibraryA(TMSI_DLL);
    if (!module) {
        std::cout << "[TMSi] Can not load " << TMSI_DLL << ", windows error = " << GetLastError()
                  << ". Copy it next to openvibe-acquisition-server.exe." << std::endl;
        return false;
    }

    bool valid = true;
    auto load  = [&](auto& fp, const char* name) {
        fp = reinterpret_cast<std::remove_reference_t<decltype(fp)>>(GetProcAddress(module, name));
        if (!fp) {
            std::cout << "[TMSi] Can not load method " << name << std::endl;
            valid = false;
        }
    };

    load(m_api->open, "Open");
    load(m_api->close, "Close");
    load(m_api->start, "Start");
    load(m_api->stop, "Stop");
    load(m_api->setSignalBuffer, "SetSignalBuffer");
    load(m_api->getSamples, "GetSamples");
    load(m_api->getSignalFormat, "GetSignalFormat");
    load(m_api->free, "Free");
    load(m_api->libraryInit, "LibraryInit");
    load(m_api->libraryExit, "LibraryExit");
    load(m_api->getErrorCode, "GetErrorCode");
    load(m_api->getErrorCodeMessage, "GetErrorCodeMessage");
    load(m_api->getDeviceList, "GetDeviceList");
    load(m_api->freeDeviceList, "FreeDeviceList");
    load(m_api->setRefCalculation, "SetRefCalculation");

    if (!valid) {
        FreeLibrary(module);
        return false;
    }

    m_api->module = module;
    return true;
}

bool CDriverTMSi::initLibrary()
{
    if (!loadLibrary()) { return false; }

    exitLibrary();
    m_devices.clear();

    const auto protocol = std::find_if(PROTOCOLS.begin(), PROTOCOLS.end(), [this](const auto& p) { return p.first == m_paramProtocol.value(); });
    const tmsi_connection_type_t connection = protocol != PROTOCOLS.end() ? protocol->second : TMSiConnectionUSB;

    int error     = 0;
    HANDLE handle = m_api->libraryInit(connection, &error);
    if (error != 0 || handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        std::cout << "[TMSi] Can not initialize TMSi library on [" << m_paramProtocol.value() << "], errorcode = " << error << std::endl;
        return false;
    }
    m_api->handle = handle;

    int nDevices      = 0;
    char** deviceList = m_api->getDeviceList(m_api->handle, &nDevices);
    if (nDevices == 0) {
        error = m_api->getErrorCode(m_api->handle);
        if (error == 0) { std::cout << "[TMSi] No TMSi devices connected" << std::endl; }
        else { printError("Could not list TMSi devices"); }
        return true;
    }

    for (int i = 0; i < nDevices; ++i) { m_devices.emplace_back(deviceList[i]); }
    // the device list allocated in the library has to be freed
    m_api->freeDeviceList(m_api->handle, nDevices, deviceList);

    return true;
}

void CDriverTMSi::exitLibrary()
{
    if (m_api->handle) {
        m_api->libraryExit(m_api->handle);
        m_api->handle = nullptr;
    }
}

bool CDriverTMSi::openFrontEnd(const std::string& deviceID)
{
    if (!m_api->open(m_api->handle, deviceID.c_str())) {
        const int error = m_api->getErrorCode(m_api->handle);
        if (error == 10061) {
            std::cout << "[TMSi] The device " << deviceID << " is present but refuses access. "
                         "It might be connected to another program. Restarting the device might help." << std::endl;
        }
        else { printError("Device " + deviceID + " is present but can not be opened. Restarting the device might help"); }
        return false;
    }
    m_opened = true;
    return true;
}

void CDriverTMSi::closeFrontEnd()
{
    if (m_started) {
        m_api->stop(m_api->handle);
        m_started = false;
    }
    if (m_api->signalFormat) {
        m_api->free(m_api->signalFormat);
        m_api->signalFormat = nullptr;
    }
    if (m_opened) {
        if (!m_api->close(m_api->handle)) { printError("Can not close device"); }
        m_opened = false;
    }
}

bool CDriverTMSi::readSignalFormat()
{
    char deviceIdentifier[1024] = {};
    m_api->signalFormat = m_api->getSignalFormat(m_api->handle, deviceIdentifier);
    if (!m_api->signalFormat) {
        printError("Error getting channel format");
        return false;
    }

    m_nDeviceChannel    = m_api->signalFormat->Elements;
    m_nDeviceEEGChannel = 0;
    // NOTE that this ONLY works if all EEG channels are listed first
    for (uint32_t i = 0; i < m_nDeviceChannel; ++i) {
        const ULONG type = m_api->signalFormat[i].Type;
        if (type == CHANNELTYPE_EXG || type == CHANNELTYPE_BIP || type == CHANNELTYPE_AUX) { m_nDeviceEEGChannel++; }
    }

    for (uint32_t i = 0; i < m_nDeviceChannel; ++i) {
        std::cout << "[TMSi] " << i << ": " << channelName(i) << " (" << channelType(i) << ")" << std::endl;
    }
    return true;
}

bool CDriverTMSi::setSignalBuffer(const uint32_t sampling)
{
    // The SDK expects the sampling rate in mHz
    ULONG samplingFrequency = sampling * 1000;
    ULONG size              = sampling * SDK_BUFFER_SECONDS;

    if (!m_api->setSignalBuffer(m_api->handle, &samplingFrequency, &size)) {
        printError("Can not set signal buffer");
        return false;
    }
    if (samplingFrequency != sampling * 1000) {
        std::cout << "[TMSi] Device does not support a sampling rate of " << sampling << " Hz (it proposes "
                  << samplingFrequency / 1000 << " Hz)" << std::endl;
        return false;
    }

    m_rawBuffer.assign(size_t(size) * m_nDeviceChannel, 0);
    m_rawSampleCount = 0;
    m_rawSampleIndex = 0;
    return true;
}

void CDriverTMSi::printError(const std::string& what) const
{
    const int error = m_api->getErrorCode(m_api->handle);
    std::cout << "[TMSi] " << what << ", errorcode = " << error << ", message = \"" << m_api->getErrorCodeMessage(m_api->handle, error) << "\""
              << std::endl;
}

std::string CDriverTMSi::channelName(const size_t index) const
{
    if (!m_api->signalFormat || index >= m_nDeviceChannel) { return ""; }

    std::string name;
    for (size_t i = 0; i < SIGNAL_NAME && m_api->signalFormat[index].Name[i] != 0; ++i) { name += char(m_api->signalFormat[index].Name[i]); }
    return trim(name);
}

std::string CDriverTMSi::channelType(const size_t index) const
{
    if (!m_api->signalFormat || index >= m_nDeviceChannel) { return ""; }

    switch (m_api->signalFormat[index].Type) {
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

//___________________________________________________________________//
//                                                                   //

bool CDriverTMSi::init()
{
    if (!initLibrary()) { return false; }
    if (m_devices.empty()) { return false; }

    std::string deviceID = m_paramDevice.value();
    if (deviceID == FIRST_DEVICE || std::find(m_devices.begin(), m_devices.end(), deviceID) == m_devices.end()) {
        if (deviceID != FIRST_DEVICE) { std::cout << "[TMSi] Device " << deviceID << " not found, using " << m_devices.front() << std::endl; }
        deviceID = m_devices.front();
    }
    std::cout << "[TMSi] Opening device " << deviceID << std::endl;

    if (!openFrontEnd(deviceID)) { return false; }

    if (!m_api->setRefCalculation(m_api->handle, m_paramCommonModeRejection.value() ? 1 : 0)) {
        printError("Can not set common mode rejection");
        closeFrontEnd();
        return false;
    }

    if (!readSignalFormat() || !setSignalBuffer(uint32_t(m_samplingRate.value()))) {
        closeFrontEnd();
        return false;
    }

    // Select the EEG channels: those a montage names, or else the first m_channelCount ones
    m_requestedChannelCount = uint32_t(m_channelCount.value());
    m_isChannelActivated.assign(m_nDeviceChannel, false);
    std::vector<std::string> eegNames;

    if (!m_paramMontage.value().empty()) {
        std::string error;
        std::vector<std::string> warnings;
        const std::map<int, std::string> montage = loadCharmeleonMontage(m_paramMontage.value(), error, warnings);
        for (const auto& warning : warnings) { std::cout << "[TMSi] Montage: " << warning << std::endl; }
        if (montage.empty()) {
            std::cout << "[TMSi] Montage: " << error << std::endl;
            closeFrontEnd();
            return false;
        }
        for (const auto& [hardwareChannel, name] : montage) {
            if (uint32_t(hardwareChannel) > m_nDeviceEEGChannel) {
                std::cout << "[TMSi] Montage: " << name << " is on hardware channel " << hardwareChannel << ", but the device has "
                          << m_nDeviceEEGChannel << " EEG channels. Ignored." << std::endl;
                continue;
            }
            m_isChannelActivated[hardwareChannel - 1] = true;
            eegNames.push_back(name);
        }
        std::cout << "[TMSi] Montage " << m_paramMontage.value() << " selects " << eegNames.size() << " EEG channels" << std::endl;
    }
    else {
        const uint32_t count = std::min<uint32_t>(m_requestedChannelCount, m_nDeviceEEGChannel);
        if (count < m_requestedChannelCount) { std::cout << "[TMSi] The device only has " << m_nDeviceEEGChannel << " EEG channels" << std::endl; }
        for (uint32_t i = 0; i < count; ++i) { m_isChannelActivated[i] = true; }
    }
    m_eegChannelCount = uint32_t(std::count(m_isChannelActivated.begin(), m_isChannelActivated.end(), true));

    std::vector<uint32_t> additionalChannels;
    for (const auto& name : splitNames(m_paramAdditionalChannels.value())) {
        bool found = false;
        for (uint32_t i = m_nDeviceEEGChannel; i < m_nDeviceChannel; ++i) {
            if (channelName(i) == name && !m_isChannelActivated[i]) {
                m_isChannelActivated[i] = true;
                additionalChannels.push_back(i);
                found = true;
                break;
            }
        }
        if (!found) { std::cout << "[TMSi] Additional channel " << name << " not found on the device" << std::endl; }
    }
    std::sort(additionalChannels.begin(), additionalChannels.end());

    m_lastTriggerValue.assign(m_nDeviceChannel, 0);
    for (const uint32_t i : additionalChannels) {
        if (m_api->signalFormat[i].Type == CHANNELTYPE_DIG) { std::cout << "[TMSi] Triggers are read from channel " << channelName(i) << std::endl; }
    }

    // Widen the channel count for the duration of the acquisition; unInit() restores it. Growing it also resizes
    // m_channelNames, through the notifier AbstractDriver installs in its constructor.
    m_channelCount.setValue(int(m_eegChannelCount + additionalChannels.size()));

    // A montage names the EEG channels; otherwise only fill in the blanks, so that channels the user renamed keep their name
    for (uint32_t i = 0; i < m_eegChannelCount; ++i) {
        if (!eegNames.empty()) { m_channelNames.setAt(i, eegNames[i]); }
        else if (m_channelNames.getAt(i).empty()) { m_channelNames.setAt(i, channelName(i)); }
    }
    for (size_t i = 0; i < additionalChannels.size(); ++i) { m_channelNames.setAt(m_eegChannelCount + i, channelName(additionalChannels[i])); }

    return true;
}

bool CDriverTMSi::unInit()
{
    closeFrontEnd();
    exitLibrary();

    // Back to the channel count the user asked for, so the additional channels' names are not persisted as if the user had chosen them
    if (m_requestedChannelCount > 0) { m_channelCount.setValue(int(m_requestedChannelCount)); }
    return true;
}

bool CDriverTMSi::start()
{
    m_pendingTriggers.clear();
    m_stimQueue.clear();
    std::fill(m_lastTriggerValue.begin(), m_lastTriggerValue.end(), 0);
    m_rawSampleCount = 0;
    m_rawSampleIndex = 0;

    // sometimes the driver might refuse to start, some sampling frequencies can be set but not used for example
    if (!m_api->start(m_api->handle)) {
        printError("Can not start acquisition");
        return false;
    }
    m_started = true;
    return true;
}

bool CDriverTMSi::stop()
{
    // Nothing device-side: Acquisition::stop() calls this before joining the reader thread, which
    // may still be inside read(). The device is stopped in unInit(), once that thread is gone.
    return true;
}

//___________________________________________________________________//
//                                                                   //

int CDriverTMSi::read(sampleType* data, const uint64_t size)
{
    m_pendingTriggers.clear();
    if (!m_started) { return 0; }

    if (m_rawSampleIndex == m_rawSampleCount) {
        const LONG nBytesReceived = m_api->getSamples(m_api->handle, m_rawBuffer.data(), ULONG(m_rawBuffer.size() * sizeof(m_rawBuffer[0])));
        if (nBytesReceived < 0) {
            printError("Can not read data");
            // Report no samples: AbstractDriver::getData() can not handle a negative return
            return 0;
        }
        if (nBytesReceived == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 0;
        }
        m_rawSampleCount = uint32_t(nBytesReceived / (m_nDeviceChannel * sizeof(m_rawBuffer[0])));
        m_rawSampleIndex = 0;
    }

    const uint32_t channels = uint32_t(m_channelCount.value());
    const uint64_t toRead   = std::min<uint64_t>(m_rawSampleCount - m_rawSampleIndex, size);

    for (uint64_t s = 0; s < toRead; ++s, ++m_rawSampleIndex) {
        const unsigned long* raw = &m_rawBuffer[size_t(m_rawSampleIndex) * m_nDeviceChannel];
        sampleType* out          = data + s * channels;
        uint32_t virtualChannel  = 0;

        for (uint32_t c = 0; c < m_nDeviceChannel; ++c) {
            if (!m_isChannelActivated[c]) { continue; }

            const signal_format_t& format = m_api->signalFormat[c];

            // For overflow of an analog channel, set the value to zero
            if (raw[c] == OVERFLOW_32BITS && (format.Type == CHANNELTYPE_EXG || format.Type == CHANNELTYPE_BIP || format.Type == CHANNELTYPE_AUX)) {
                out[virtualChannel] = 0;
            }
            else {
                switch (format.Format) {
                    case SF_UNSIGNED: out[virtualChannel] = sampleType(raw[c] * format.UnitGain + format.UnitOffSet); break;
                    case SF_INTEGER: out[virtualChannel] = sampleType(int(raw[c]) * format.UnitGain + format.UnitOffSet); break;
                    default: out[virtualChannel] = 0; break;
                }
            }
            virtualChannel++;

            // Only activated DIG channels carry triggers: the low 8 bits, as is, sent on change to a non-zero value
            if (format.Type == CHANNELTYPE_DIG) {
                const uint32_t trigger = raw[c] & 0xFFU;
                if (trigger != m_lastTriggerValue[c]) {
                    m_lastTriggerValue[c] = trigger;
                    if (trigger != 0) { m_pendingTriggers.push_back({uint64_t(OVTK_StimulationId_Label(trigger)), s}); }
                }
            }
        }
    }

    return int(toRead);
}

void CDriverTMSi::getData(samplesBuffer* buff)
{
    buff->count             = read(buff->samplesBuff, buff->maxSampleCount);
    const uint64_t readTime = timestamp();
    if (buff->count > 0) { buff->timestampsBuff[buff->count - 1] = readTime; }

    // Date each trigger at its own sample, counting back from the last sample of the block, which is stamped readTime
    const uint64_t periodNs = 1'000'000'000ULL / uint64_t(m_samplingRate.value());
    for (const auto& trigger : m_pendingTriggers) {
        const uint64_t samplesBefore = buff->count - 1 - trigger.sampleIndex;
        m_stimQueue.push_back(ovStimulation{trigger.id, readTime - samplesBefore * periodNs, 0});
    }
    m_pendingTriggers.clear();

    buff->stimCount = readStimulations(buff->stimulationsBuff, buff->maxStimCount);
}

int CDriverTMSi::readStimulations(ovStimulation* buff, const uint32_t maxCount)
{
    int stimAdded = 0;
    while (!m_stimQueue.empty() && uint32_t(stimAdded) < maxCount) {
        buff[stimAdded] = m_stimQueue.front();
        m_stimQueue.pop_front();
        stimAdded++;
    }
    return stimAdded;
}
