/**-------------------------------------------------------------------------------------------------
 * \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
 * Rights transferred to Inria, contract signed 21.11.2014
 *-------------------------------------------------------------------------------------------------
 */

#include "CDriverTMSi.h"

#include <BlockStamps.h>  // UG fork
#include <CharmeleonMontage.h>
#include <SteadyTrigger.h>  // UG fork

#include "toolkit/ovtk_defines.h"
#include "toolkit/ovtk_stimulations.h"

#include <Windows.h>

#include <algorithm>
#include <any>
#include <cctype>
#include <cmath>
#include <map>
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

// UG fork: the unit of a channel's values, SSignalFormat::UnitId, as the TMSi SDK numbers it; only volt is converted
constexpr ULONG UNIT_VOLT = 1;

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
typedef BOOLEAN (__stdcall * PGETBUFFERINFO)(HANDLE handle, PULONG overflow, PULONG percentFull);  // UG fork

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

/// True for the sawtooth/ramp channel, which the SDK may type as a digital channel.
bool looksLikeSawtooth(std::string name)
{
    std::transform(name.begin(), name.end(), name.begin(), [](const unsigned char c) { return char(std::tolower(c)); });
    return name.find("saw") != std::string::npos || name.find("ramp") != std::string::npos;
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
    PGETBUFFERINFO getBufferInfo             = nullptr;  ///< UG fork: optional, see noteBufferLevel()
};

//___________________________________________________________________//
//                                                                   //

CDriverTMSi::CDriverTMSi()
    : m_api(std::make_unique<TMSiApi>()),
      m_paramProtocol("ConnectionProtocol", "Connection protocol", protocolNames(), "USB"),
      m_paramDevice("DeviceIdentifier", "Device", {FIRST_DEVICE}, FIRST_DEVICE),
      m_paramTriggerChannel("TriggerChannel", "Trigger channel (device name; empty = the first digital channel that is not the sawtooth)", ""),
      m_paramCommonModeRejection("CommonModeRejection", "Common mode rejection (common average reference)", true),
      m_paramMontage("Montage",
                     "Charmeleon montage (a .json file, or the name of one in Charmeleon's library): names the channels, and decides which EEG "
                     "channels are recorded",
                     "",
                     OpenViBE::Parameters::Read, {"Charmeleon montage (*.json)", "Any File (*)"})
{
    m_samplingRate = 512;
    m_channelCount = 32;
    // Set from the amplifier, but kept among the parameters so that the names of the last session's channels can be restored
    // when it is not connected at startup
    m_channelCount.setDescription("Number of channels (read from the amplifier)");

    m_parameters.push_back(&m_paramProtocol);
    m_parameters.push_back(&m_paramDevice);
    m_parameters.push_back(&m_samplingRate);
    m_parameters.push_back(&m_channelCount);
    m_parameters.push_back(&m_paramMontage);
    m_parameters.push_back(&m_paramTriggerChannel);
    m_parameters.push_back(&m_paramCommonModeRejection);

    m_paramMontage.connect([this](std::any) { onMontageChanged(); });
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
        if (queryLayout()) { fitNamesToLayout(); }
        else if (m_channelNames.value().size() == size_t(m_channelCount.value())) {
            // No device: the montage still names the list, provisionally, and says what it did
            std::vector<std::string> names = m_channelNames.value();
            if (applyMontage(names)) { m_channelNames.setValue(names); }
        }
    }
    return AbstractDriver::getParameters();
}

std::vector<std::string> CDriverTMSi::channelHints() const
{
    // UG fork: without a device, the hints of the assumed layout, so that it is clear where BIP and AUX names go
    if (m_layout.empty()) {
        if (m_opened) { return {}; }
        const std::vector<MontageChannelKind> kinds = assumedLayout(m_channelNames.value().size());
        std::vector<std::string> hints(kinds.size());
        uint32_t nBipolar = 0, nAuxiliary = 0;
        for (size_t i = 0; i < kinds.size(); ++i) {
            if (kinds[i] == MontageChannelKind::Bipolar) { hints[i] = "BIP" + std::to_string(++nBipolar); }
            else if (kinds[i] == MontageChannelKind::Auxiliary) { hints[i] = "AUX" + std::to_string(++nAuxiliary); }
        }
        return hints;
    }

    // During an acquisition the list holds the recorded channels only, which all have a name
    if (m_channelNames.value().size() != m_layout.size()) { return {}; }

    std::vector<std::string> hints(m_layout.size());
    uint32_t nBipolar = 0, nAuxiliary = 0;
    for (size_t i = 0; i < m_layout.size(); ++i) {
        switch (m_layout[i].type) {
            case CHANNELTYPE_EXG: break;
            case CHANNELTYPE_BIP: hints[i] = "BIP" + std::to_string(++nBipolar); break;
            case CHANNELTYPE_AUX: hints[i] = "AUX" + std::to_string(++nAuxiliary); break;
            default: hints[i] = m_layout[i].name; break;
        }
    }
    return hints;
}

std::string CDriverTMSi::selectedDevice() const
{
    if (m_devices.empty()) { return ""; }
    const std::string& device = m_paramDevice.value();
    return std::find(m_devices.begin(), m_devices.end(), device) != m_devices.end() ? device : m_devices.front();
}

bool CDriverTMSi::queryLayout()
{
    const std::string device = selectedDevice();
    if (device.empty()) { return false; }
    if (device == m_layoutDevice && !m_layout.empty()) { return true; }

    if (!openFrontEnd(device)) { return false; }
    const bool ok = readSignalFormat();
    closeFrontEnd();
    if (ok) { m_layoutDevice = device; }
    return ok;
}

bool CDriverTMSi::fitNamesToLayout()
{
    if (m_layout.empty()) { return false; }

    // Names stored for another layout (another device, or a changed channel count) mean nothing here: start over
    std::vector<std::string> names = m_channelNames.value();
    if (names.size() != m_layout.size()) {
        names.assign(m_layout.size(), "");
        for (size_t i = 0; i < m_layout.size(); ++i) { if (m_layout[i].type == CHANNELTYPE_EXG) { names[i] = m_layout[i].name; } }
    }
    const bool ok = applyMontage(names);

    // Resizes m_channelNames too, through the notifier AbstractDriver installs in its constructor
    m_channelCount.setValue(int(m_layout.size()));
    m_channelNames.setValue(names);
    return ok;
}

std::vector<MontageChannelKind> CDriverTMSi::layoutKinds() const
{
    std::vector<MontageChannelKind> kinds;
    kinds.reserve(m_layout.size());
    for (const auto& channel : m_layout) {
        switch (channel.type) {
            case CHANNELTYPE_EXG: kinds.push_back(MontageChannelKind::EEG); break;
            case CHANNELTYPE_BIP: kinds.push_back(MontageChannelKind::Bipolar); break;
            case CHANNELTYPE_AUX: kinds.push_back(MontageChannelKind::Auxiliary); break;
            default: kinds.push_back(MontageChannelKind::Other); break;
        }
    }
    return kinds;
}

std::vector<MontageChannelKind> CDriverTMSi::assumedLayout(const size_t channelCount)
{
    constexpr size_t BIPOLAR = 4, AUXILIARY = 4;
    std::vector<MontageChannelKind> kinds(channelCount, MontageChannelKind::EEG);
    if (channelCount <= BIPOLAR + AUXILIARY) { return kinds; }
    const size_t firstBipolar = channelCount - BIPOLAR - AUXILIARY;
    for (size_t i = firstBipolar; i < channelCount; ++i) {
        kinds[i] = i < firstBipolar + BIPOLAR ? MontageChannelKind::Bipolar : MontageChannelKind::Auxiliary;
    }
    return kinds;
}

bool CDriverTMSi::applyMontage(std::vector<std::string>& names)
{
    // UG fork: the montage rules live in CharmeleonMontage, shared with the eego driver; this only feeds them the
    // device's channel kinds and shows the outcome under the settings
    if (trim(m_paramMontage.value()).empty()) {
        m_montageNote = {};
        clearNote();
        setChannelLocks({});
        return true;
    }

    MontageApplication application;
    std::string error;
    // Without a device, the layout a Refa has (EXG, then 4 BIP and 4 AUX), which Play replaces by the device's own
    const bool assumed = m_layout.empty();
    const std::vector<MontageChannelKind> kinds = assumed ? assumedLayout(names.size()) : layoutKinds();
    if (!applyCharmeleonMontageSetting(m_paramMontage.value(), kinds, names, application, error, assumed)) {
        report(error);
        m_montageNote = {Note::Level::Error, error};
        setNote(m_montageNote.level, m_montageNote.text);
        setChannelLocks({});
        return false;
    }

    std::cout << "[TMSi] " << application.summary << std::endl;
    names         = std::move(application.names);
    m_montageNote = {application.warning ? Note::Level::Warning : Note::Level::Info, application.summary};
    setNote(m_montageNote.level, m_montageNote.text);
    setChannelLocks(std::move(application.locked));
    return true;
}

std::vector<std::string> CDriverTMSi::charmeleonArguments() const
{
    // Charmeleon 2.2 and later; an older one opens the first amplifier it finds, eego before TMSi
    std::vector<std::string> arguments{"--amplifier", "tmsi", "--protocol", m_paramProtocol.value()};
    if (m_paramDevice.value() != FIRST_DEVICE) {
        arguments.push_back("--device");
        arguments.push_back(m_paramDevice.value());
    }
    return arguments;
}

std::string CDriverTMSi::charmeleonMontage() const
{
    const MontageLocation location = locateCharmeleonMontage(m_paramMontage.value());
    return location.source == MontageLocation::Source::NotFound ? m_paramMontage.value() : location.path;
}

void CDriverTMSi::takeOverCharmeleonMontage(const std::string& path)
{
    const std::string setting = montageSettingAfterSave(m_paramMontage.value(), path);
    // A new value applies itself, through onMontageChanged(); the same one has to be read again explicitly
    if (setting != m_paramMontage.value()) { m_paramMontage.setValue(setting); }
    else { onMontageChanged(); }
}

void CDriverTMSi::onMontageChanged()
{
    // During an acquisition the list only holds the recorded channels; init() applies the montage anyway
    if (m_opened || (!m_layout.empty() && m_channelNames.value().size() != m_layout.size())) { return; }

    std::vector<std::string> names = m_channelNames.value();
    if (!applyMontage(names)) { return; }
    if (trim(m_paramMontage.value()).empty()) {
        // Without the device there are no device names to go back to
        if (m_layout.empty()) { return; }
        for (size_t i = 0; i < m_layout.size(); ++i) { if (m_layout[i].type == CHANNELTYPE_EXG) { names[i] = m_layout[i].name; } }
    }
    m_channelNames.setValue(names);
}

//___________________________________________________________________//
//                                                                   //

bool CDriverTMSi::loadLibrary()
{
    if (m_api->module) { return true; }

    HMODULE module = LoadLibraryA(TMSI_DLL);
    if (!module) {
        report(std::string("Can not load ") + TMSI_DLL + " (Windows error " + std::to_string(GetLastError())
               + "). Copy it next to openvibe-acquisition-server.exe.");
        return false;
    }

    bool valid = true;
    auto load  = [&](auto& fp, const char* name) {
        fp = reinterpret_cast<std::remove_reference_t<decltype(fp)>>(GetProcAddress(module, name));
        if (!fp) {
            report(std::string(TMSI_DLL) + " has no function " + name + "; it may be another version.");
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
    // UG fork: only for showing the buffer level, so a library without it is still used
    m_api->getBufferInfo = reinterpret_cast<PGETBUFFERINFO>(GetProcAddress(module, "GetBufferInfo"));

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
        report("Can not start the TMSi library on " + m_paramProtocol.value() + " (TMSi error " + std::to_string(error) + ").");
        return false;
    }
    m_api->handle = handle;

    int nDevices      = 0;
    char** deviceList = m_api->getDeviceList(m_api->handle, &nDevices);
    if (nDevices == 0) {
        error = m_api->getErrorCode(m_api->handle);
        if (error == 0) { report("No TMSi device connected (" + m_paramProtocol.value() + ")."); }
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
            report("The device " + deviceID + " is present but refuses access. It might be connected to another program, e.g. Charmeleon. "
                   "Restarting the device might help.");
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

    m_nDeviceChannel = m_api->signalFormat->Elements;
    m_layout.clear();
    for (uint32_t i = 0; i < m_nDeviceChannel; ++i) {
        std::string name;
        for (size_t c = 0; c < SIGNAL_NAME && m_api->signalFormat[i].Name[c] != 0; ++c) { name += char(m_api->signalFormat[i].Name[c]); }
        m_layout.push_back({trim(name), uint32_t(m_api->signalFormat[i].Type)});
        std::cout << "[TMSi] " << i + 1 << ": " << channelName(i) << " (" << channelType(i) << ")" << std::endl;
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
        report("The device does not support a sampling rate of " + std::to_string(sampling) + " Hz; it proposes "
               + std::to_string(samplingFrequency / 1000) + " Hz.");
        return false;
    }

    m_rawBuffer.assign(size_t(size) * m_nDeviceChannel, 0);
    m_bufferSamples  = uint32_t(size);
    m_bufferRate     = sampling;
    m_rawSampleCount = 0;
    m_rawSampleIndex = 0;
    return true;
}

void CDriverTMSi::printError(const std::string& what) const
{
    const int error = m_api->getErrorCode(m_api->handle);
    const char* message = m_api->getErrorCodeMessage(m_api->handle, error);
    report(what + " (TMSi error " + std::to_string(error) + (message && *message ? std::string(": ") + message : std::string()) + ")");
}

void CDriverTMSi::report(const std::string& message) const
{
    std::cout << "[TMSi] " << message << std::endl;
    m_lastError = message;
}

bool CDriverTMSi::failInit(const std::string& message)
{
    if (!message.empty()) { report(message); }
    // UG fork: upstream drivers only print why they could not start; the server shows this under the settings
    setNote(Note::Level::Error, "Play failed: " + (m_lastError.empty() ? std::string("the TMSi device could not be started.") : m_lastError));
    return false;
}

std::string CDriverTMSi::channelName(const size_t index) const { return index < m_layout.size() ? m_layout[index].name : ""; }

std::string CDriverTMSi::channelType(const size_t index) const
{
    if (index >= m_layout.size()) { return ""; }

    switch (m_layout[index].type) {
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
    m_lastError.clear();
    if (!initLibrary() || m_devices.empty()) { return failInit(); }

    const std::string deviceID = selectedDevice();
    if (m_paramDevice.value() != FIRST_DEVICE && deviceID != m_paramDevice.value()) {
        std::cout << "[TMSi] Device " << m_paramDevice.value() << " not found, using " << deviceID << std::endl;
    }
    std::cout << "[TMSi] Opening device " << deviceID << std::endl;

    if (!openFrontEnd(deviceID)) { return failInit(); }

    if (!m_api->setRefCalculation(m_api->handle, m_paramCommonModeRejection.value() ? 1 : 0)) {
        printError("Can not set common mode rejection");
        closeFrontEnd();
        return failInit();
    }

    if (!readSignalFormat() || !setSignalBuffer(uint32_t(m_samplingRate.value()))) {
        closeFrontEnd();
        return failInit();
    }
    m_layoutDevice = deviceID;

    // Record the channels that have a name, under that name
    if (!fitNamesToLayout()) {
        closeFrontEnd();
        return failInit();
    }
    m_configuredNames = m_channelNames.value();
    m_isChannelActivated.assign(m_nDeviceChannel, false);
    std::vector<std::string> recordedNames;
    for (uint32_t i = 0; i < m_nDeviceChannel; ++i) {
        const std::string name = trim(m_configuredNames[i]);
        if (name.empty()) { continue; }
        m_isChannelActivated[i] = true;
        recordedNames.push_back(name);
    }
    if (recordedNames.empty()) {
        m_configuredNames.clear();
        closeFrontEnd();
        return failInit("No channel has a name, so there is nothing to record. Name the channels, or choose a montage.");
    }
    std::cout << "[TMSi] Recording " << recordedNames.size() << " of the " << m_nDeviceChannel << " channels" << std::endl;

    // UG fork: every channel in volt goes out in microvolt, the unit the BrainVision writer declares. The SDK gives a channel's
    // values in its UnitId times 10^UnitExponent: microvolt for EXG channels (-6), but BIP and AUX channels may differ. Upstream
    // ignored the exponent, so such a channel was recorded a thousand or a million times off.
    m_channelScale.assign(m_nDeviceChannel, 1.0);
    for (uint32_t i = 0; i < m_nDeviceChannel; ++i) {
        if (!m_isChannelActivated[i]) { continue; }
        const signal_format_t& format = m_api->signalFormat[i];
        const bool analog = format.Type == CHANNELTYPE_EXG || format.Type == CHANNELTYPE_BIP || format.Type == CHANNELTYPE_AUX;
        if (format.UnitId == UNIT_VOLT) {
            m_channelScale[i] = std::pow(10.0, double(format.UnitExponent + 6));
            if (format.UnitExponent != -6) {
                std::cout << "[TMSi] " << trim(m_configuredNames[i]) << " (" << channelType(i) << ") is given in 1e" << format.UnitExponent
                          << " V; it is recorded in microvolt." << std::endl;
            }
        }
        else if (analog) {
            std::cout << "[TMSi] " << trim(m_configuredNames[i]) << " (" << channelType(i) << ") is not in volt (unit " << format.UnitId
                      << ", 1e" << format.UnitExponent << "); it is recorded as the device gives it, not in microvolt." << std::endl;
        }
    }
    m_overflowSamples = 0;
    m_overflowReported = 0;

    // The trigger channel, named or else the first DIG channel that is not the sawtooth, which the SDK also types as DIG
    m_triggerChannel   = -1;
    m_trigger.reset(0);  // UG fork
    const std::string triggerName = trim(m_paramTriggerChannel.value());
    for (uint32_t i = 0; i < m_nDeviceChannel && m_triggerChannel < 0; ++i) {
        if (!triggerName.empty()) {
            if (channelName(i) == triggerName) { m_triggerChannel = int(i); }
        }
        else if (m_api->signalFormat[i].Type == CHANNELTYPE_DIG && !looksLikeSawtooth(channelName(i))) { m_triggerChannel = int(i); }
    }
    if (m_triggerChannel >= 0) { std::cout << "[TMSi] Triggers are read from channel " << channelName(m_triggerChannel) << std::endl; }
    else if (!triggerName.empty()) { std::cout << "[TMSi] Trigger channel " << triggerName << " not found on the device, no triggers" << std::endl; }
    else { std::cout << "[TMSi] No digital channel found, no triggers" << std::endl; }

    // For the duration of the acquisition the channel count and names describe the recorded channels only; unInit() restores the full list
    m_configuredLocks = channelLocks();
    setChannelLocks({});
    m_channelCount.setValue(int(recordedNames.size()));
    m_channelNames.setValue(recordedNames);
    setNote(m_montageNote.level, m_montageNote.text);

    return true;
}

bool CDriverTMSi::unInit()
{
    m_bufferReported = false;
    closeFrontEnd();
    exitLibrary();

    if (!m_configuredNames.empty()) {
        m_channelCount.setValue(int(m_configuredNames.size()));
        m_channelNames.setValue(m_configuredNames);
        m_configuredNames.clear();
        setChannelLocks(std::move(m_configuredLocks));
        m_configuredLocks.clear();
    }
    return true;
}

bool CDriverTMSi::start()
{
    m_triggerValues.clear();  // UG fork
    m_stimQueue.clear();
    m_trigger.reset(0);  // UG fork
    m_lastSampleTime   = 0;
    m_rawSampleCount = 0;
    m_rawSampleIndex = 0;
    m_blockFetchTime = 0;  // UG fork
    m_readFirst      = 0;  // UG fork

    // sometimes the driver might refuse to start, some sampling frequencies can be set but not used for example
    if (!m_api->start(m_api->handle)) {
        printError("Can not start acquisition");
        return false;
    }
    m_started = true;

    // The SDK resets its overflow count at Start
    m_bufferPeakPercent = 0;
    m_bufferOverflows   = 0;
    m_lastBufferInfo    = {};
    m_lastReadNs        = std::chrono::steady_clock::now().time_since_epoch().count();
    m_bufferReported    = true;
    return true;
}

void CDriverTMSi::noteBufferLevel()
{
    m_lastReadNs = std::chrono::steady_clock::now().time_since_epoch().count();
    if (!m_api->getBufferInfo) { return; }

    // After a stall the first call comes well after the previous one, so the backlog it built up is always seen
    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastBufferInfo < std::chrono::milliseconds(20)) { return; }
    m_lastBufferInfo = now;

    ULONG overflows = 0, percentFull = 0;
    if (!m_api->getBufferInfo(m_api->handle, &overflows, &percentFull)) { return; }

    if (percentFull > m_bufferPeakPercent) { m_bufferPeakPercent = uint32_t(percentFull); }
    if (overflows != m_bufferOverflows) {
        std::cout << "[TMSi] The SDK's sample buffer overflowed (" << overflows << " times since Play): "
                  << uint64_t(overflows - m_bufferOverflows) * m_bufferSamples << " samples were lost." << std::endl;
        m_bufferOverflows = uint32_t(overflows);
    }
}

AbstractDriver::DeviceBuffer CDriverTMSi::takeDeviceBuffer()
{
    DeviceBuffer result;
    const uint32_t rate = m_bufferRate;
    if (!m_bufferReported || rate == 0) { return result; }

    result.reported        = true;
    result.capacitySeconds = double(m_bufferSamples) / rate;
    result.peakSeconds     = m_bufferPeakPercent.exchange(0) / 100.0 * result.capacitySeconds;
    // While the reader is held up nothing is read and nothing is measured, but the backlog grows all the same
    const int64_t sinceRead = std::chrono::steady_clock::now().time_since_epoch().count() - m_lastReadNs;
    result.peakSeconds      = (std::max)(result.peakSeconds, std::chrono::duration<double>(std::chrono::steady_clock::duration(sinceRead)).count());
    // Without GetBufferInfo the overflows are not known
    result.lossCounted = m_api->getBufferInfo != nullptr;
    result.lostSamples = uint64_t(m_bufferOverflows) * m_bufferSamples;
    return result;
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
    m_triggerValues.clear();  // UG fork
    if (!m_started) { return 0; }

    if (m_rawSampleIndex == m_rawSampleCount) {
        noteBufferLevel();  // UG fork
        const LONG nBytesReceived = m_api->getSamples(m_api->handle, m_rawBuffer.data(), ULONG(m_rawBuffer.size() * sizeof(m_rawBuffer[0])));
        const uint64_t fetchTime  = timestamp();  // UG fork: the block's last sample is the newest the SDK had now, see getData()
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
        // UG fork: a block that is not a whole number of samples loses its last, partial one; upstream dropped it silently
        if (nBytesReceived % (m_nDeviceChannel * sizeof(m_rawBuffer[0])) != 0) {
            std::cout << "[TMSi] The SDK returned " << nBytesReceived << " bytes, not a whole number of samples; the part of a sample at the "
                      << "end is lost." << std::endl;
        }
        m_rawSampleIndex = 0;
        m_blockFetchTime = fetchTime;  // UG fork
    }

    m_readFirst = m_rawSampleIndex;  // UG fork: where in the block this read starts, see getData()

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
                ++m_overflowSamples;  // UG fork: reported below, as a 0 in the recording does not show it
            }
            else {
                // UG fork: in microvolt for a channel in volt, see init()
                const double scale = m_channelScale[c];
                switch (format.Format) {
                    case SF_UNSIGNED: out[virtualChannel] = sampleType((raw[c] * format.UnitGain + format.UnitOffSet) * scale); break;
                    case SF_INTEGER: out[virtualChannel] = sampleType((int(raw[c]) * format.UnitGain + format.UnitOffSet) * scale); break;
                    default: out[virtualChannel] = 0; break;
                }
            }
            virtualChannel++;
        }

        // Only the trigger channel carries triggers: its low 8 bits, which getData() turns into triggers once it has the stamps
        if (m_triggerChannel >= 0) { m_triggerValues.push_back(raw[m_triggerChannel] & 0xFFU); }  // UG fork
    }

    // UG fork: an electrode out of range, e.g. come loose, is recorded as 0; say how often, at most once per block
    if (m_overflowSamples != m_overflowReported) {
        std::cout << "[TMSi] " << m_overflowSamples - m_overflowReported << " more samples out of the amplifier's range, recorded as 0 ("
                  << m_overflowSamples << " since Play)." << std::endl;
        m_overflowReported = m_overflowSamples;
    }

    return int(toRead);
}

void CDriverTMSi::getData(samplesBuffer* buff)
{
    buff->count = read(buff->samplesBuff, buff->maxSampleCount);

    // Stamp every sample from when its SDK block was fetched and its place in that block, and keep the stamps strictly
    // increasing across blocks. The client places a stimulation on the sample whose stamp matches its date, so dating each
    // trigger with its own sample's stamp puts it on exactly that sample.
    // UG fork: upstream counted back from the end of this read, so a block bigger than one read came out too late, by up to
    // the block's length, with its later samples squeezed 1 ns apart. See UG.md, 2.24.
    const uint64_t periodNs = 1'000'000'000ULL / uint64_t(m_samplingRate.value());
    stampFromBlock(m_blockFetchTime, m_rawSampleCount, m_readFirst, buff->count, periodNs, m_lastSampleTime, buff->timestampsBuff);

    // A trigger is a change to a non-zero value, sent as is, once it has held for two samples, and dated at the first sample
    // where the channel changed. A change can settle in a later read than it started in: its date then is a sample sent
    // before, and the client still places it on that sample.
    // UG fork: upstream sent every change, so a value caught while the trigger lines switched became an extra trigger with a
    // wrong code. See UG.md, 2.25.
    for (size_t s = 0; s < m_triggerValues.size(); ++s) {
        const auto change = m_trigger.feed(m_triggerValues[s], buff->timestampsBuff[s]);
        if (change && change->value != 0) { m_stimQueue.push_back(ovStimulation{uint64_t(OVTK_StimulationId_Label(change->value)), change->onset, 0}); }
    }
    m_triggerValues.clear();

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
