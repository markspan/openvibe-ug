#if defined TARGET_HAS_ThirdPartyNeXus

#include "ovasCDriverTMSiRefa32B.h"
#include "ovasCConfigurationTMSIRefa32B.h"

#include <toolkit/ovtk_all.h>

#include <iostream>
#include <cmath>
#include <cstring>
#include <system/ovCTime.h>
#include "ovasCConfigurationTMSIRefa32B.h"
#include <windows.h>

namespace OpenViBE {
namespace AcquisitionServer {
//structure define in the DLL

typedef struct SSpDevicePath
{
    DWORD dwCbSize;
    TCHAR devicePath[1];
} SP_DEVICE_PATH, *PSP_DEVICE_PATH;

typedef struct SFeatureData
{
    ULONG FeatureId;
    ULONG Info;
} FEATURE_DATA, *PFEATURE_DATA;

typedef struct _SYSTEM_TIME
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEM_TIME;

typedef struct SSignalFormat
{
    ULONG Size;      // Size of this structure
    ULONG Elements;  // Number of elements in list

    ULONG Type;      // One of the signal types above
    ULONG SubType;   // One of the signal sub-types above
    ULONG Format;    // Float / Integer / Asci / Ect..
    ULONG Bytes;     // Number of bytes per sample including subsignals

    FLOAT UnitGain;
    FLOAT UnitOffSet;
    ULONG UnitId;
    LONG UnitExponent;

    WCHAR Name[SIGNAL_NAME];

    ULONG Port;
    WCHAR PortName[SIGNAL_NAME];
    ULONG SerialNumber;
} signal_format_t, *psignal_format_t;

typedef struct _FeatureMemory
{
    FEATURE_DATA Feature;
    ULONG Data[1];
} FEATURE_MEMORY, *PFEATURE_MEMORY;

typedef struct _FeatureMode
{
    FEATURE_DATA Feature;
    ULONG Mode;
} FEATURE_MODE, *PFEATURE_MODE;

//___________________________________________________________//
//                                                           //

//methods define in the DLL

typedef HANDLE ( __stdcall * POPEN)(PSP_DEVICE_PATH devicePath);
typedef BOOL ( __stdcall * PCLOSE)(HANDLE handle);
typedef ULONG ( __stdcall * PGETDEVICESTATE)(IN HANDLE handle);
typedef BOOLEAN ( __stdcall * PSTART)(IN HANDLE handle);
typedef BOOLEAN ( __stdcall * PRESETDEVICE)(IN HANDLE handle);
typedef BOOLEAN ( __stdcall * PSTOP)(IN HANDLE handle);
typedef HANDLE ( __stdcall * PGETSLAVEHANDLE)(IN HANDLE handle);
typedef BOOLEAN ( __stdcall * PADDSLAVE)(IN HANDLE handle, IN HANDLE slavehandle);
typedef psignal_format_t ( __stdcall * PGETSIGNALFORMAT)(IN HANDLE handle, IN OUT psignal_format_t format);
typedef BOOLEAN ( __stdcall * PSETSIGNALBUFFER)(IN HANDLE handle, IN OUT PULONG sampling, IN OUT PULONG size);
typedef ULONG ( __stdcall * PGETSAMPLES)(IN HANDLE handle, OUT PULONG sampleBuffer, IN ULONG size);
typedef BOOLEAN ( __stdcall * PGETBUFFERINFO)(IN HANDLE handle, OUT PULONG overflow, OUT PULONG percentFull);
typedef BOOLEAN ( __stdcall * PDEVICEFEATURE)(IN HANDLE handle, IN LPVOID dataIn, IN DWORD inSize, OUT LPVOID dataOut, IN DWORD outSize);
typedef PSP_DEVICE_PATH ( __stdcall * PGETINSTANCEID)(IN LONG deviceIndex, IN BOOLEAN present, OUT ULONG* maxDevices);
typedef HKEY ( __stdcall * POPENREGKEY)(IN PSP_DEVICE_PATH path);
typedef BOOL ( __stdcall * PFREE)(IN VOID* memory);

//___________________________________________________________//
//                                                           //

//vars used for load the DLL's methods
POPEN fpOpen;
PCLOSE fpClose;
PGETDEVICESTATE fpGetDeviceState;
PSTART fpStart;
PRESETDEVICE fpReset;
PSTOP fpStop;
PGETSLAVEHANDLE fpGetSlaveHandle;
PADDSLAVE fpAddSlave;
PGETSIGNALFORMAT fpGetSignalFormat;
PSETSIGNALBUFFER fpSetSignalBuffer;
PGETSAMPLES fpGetSamples;
PGETBUFFERINFO fpGetBufferInfo;
PDEVICEFEATURE fpDeviceFeature;
PGETINSTANCEID fpGetInstanceId;
POPENREGKEY fpOpenRegKey;
PFREE fpFree;

//___________________________________________________________//
//                                                           //

HANDLE handleMaster;                //Device Handle Master
std::vector<HANDLE> handleSlaves;   //Device Handle Slave
HINSTANCE libHandle;                //Library Handle

//  Buffer for storing the samples
//----------------------------------
ULONG* bufferUnsigned;
LONG* buffer;
bool islBufferUnsigned;
ULONG sampling;
ULONG bufferSize;

//  device
//----------
std::map<PSP_DEVICE_PATH, CString> devicePaths; // devicePaths contains all connected devicePath and their name
CString devicePathMaster;                       // the name of the Master devicePath chosen
std::vector<std::string> devicePathSlaves;      // a list with the name of the Slave devicePath chosen
ULONG nDevicesConnected;                        // Number of devices on this PC
ULONG nDevicesOpen;                             // total of Master/slaves device open

//  store value for calculate the data
//--------------------------------------
std::vector<LONG> exponentChannels;
std::vector<FLOAT> unitGains;
std::vector<FLOAT> unitOffSets;

//number of channels
ULONG nTotalChannels;
uint32_t m_bufferSize;

#define LOAD_DLL_FUNC(var, type, name) \
    var = (type)::GetProcAddress(libHandle, name); \
    if(!var) \
    { \
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Load method " << name << "\n"; \
        m_valid=false; \
        return; \
    }

CDriverTMSiRefa32B::CDriverTMSiRefa32B(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_TMSIRefa32B", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(32);

    //load the DLL of the driver
    libHandle    = nullptr;
    handleMaster = nullptr;

    //Open library
    const CString path = m_driverCtx.getConfigurationManager().expand("${Path_Bin}") + "/" + RTLOADER;
    libHandle          = ::LoadLibrary(path.toASCIIString());

    //if it can't be open return FALSE;
    if (libHandle == nullptr)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Couldn't load DLL: " << path << "\n";
        return;
    }

    //load DLL methods for initialized the driver
    LOAD_DLL_FUNC(fpOpen, POPEN, "Open");
    LOAD_DLL_FUNC(fpClose, PCLOSE, "Close");
    LOAD_DLL_FUNC(fpGetDeviceState, PGETDEVICESTATE, "GetDeviceState");
    LOAD_DLL_FUNC(fpStart, PSTART, "Start");
    LOAD_DLL_FUNC(fpReset, PRESETDEVICE, "ResetDevice");
    LOAD_DLL_FUNC(fpStop, PSTOP, "Stop");
    LOAD_DLL_FUNC(fpGetSlaveHandle, PGETSLAVEHANDLE, "GetSlaveHandle");
    LOAD_DLL_FUNC(fpAddSlave, PADDSLAVE, "AddSlave");
    LOAD_DLL_FUNC(fpGetSignalFormat, PGETSIGNALFORMAT, "GetSignalFormat");
    LOAD_DLL_FUNC(fpSetSignalBuffer, PSETSIGNALBUFFER, "SetSignalBuffer");
    LOAD_DLL_FUNC(fpGetSamples, PGETSAMPLES, "GetSamples");
    LOAD_DLL_FUNC(fpGetBufferInfo, PGETBUFFERINFO, "GetBufferInfo");
    LOAD_DLL_FUNC(fpDeviceFeature, PDEVICEFEATURE, "DeviceFeature");

    LOAD_DLL_FUNC(fpGetInstanceId, PGETINSTANCEID, "GetInstanceId");
    LOAD_DLL_FUNC(fpOpenRegKey, POPENREGKEY, "OpenRegKey");
    LOAD_DLL_FUNC(fpFree, PFREE, "Free");

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Succeeded in loading DLL: " << CString(path) << "\n";
    devicePathMaster = "";
    nDevicesOpen     = 0;
    m_checkImpedance = m_driverCtx.getConfigurationManager().expandAsBoolean("${AcquisitionServer_CheckImpedance}", false);//m_checkImpedance=false;

    m_settings.add("Header", &m_header);
    m_settings.add("DevicePathMaster", &devicePathMaster);
    m_settings.add("DevicePathSlave", &devicePathSlaves);
    m_settings.load();
}

bool CDriverTMSiRefa32B::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (!m_valid || m_driverCtx.isConnected()) { return false; }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Initialized TMSI\n";

    if (handleMaster != nullptr)
    {
        fpClose(handleMaster);
        handleMaster = nullptr;
    }

    //Refresh information about connected device
    if (!refreshDevicePath()) { return false; }

    if (nDevicesConnected == 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "There is no device connected to the PC\n";
        return false;
    }

    //open master
    auto iter       = devicePaths.begin();
    bool masterFind = false;
    while (devicePathMaster != CString("") && !masterFind && iter != devicePaths.end())
    {
        if ((*iter).second == devicePathMaster)
        {
            masterFind   = true;
            handleMaster = fpOpen((*iter).first);
            if (!handleMaster)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Open Driver\n";
                return false;
            }
            nDevicesOpen++;
        }
        ++iter;
    }
    if (handleMaster == nullptr && nDevicesConnected > 0)
    {
        handleMaster = fpOpen((*devicePaths.begin()).first);
        if (!handleMaster)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Open Driver\n";
            return false;
        }
        nDevicesOpen++;
    }

    //open slave
    for (uint32_t i = 0; i < devicePathSlaves.size(); ++i)
    {
        auto j    = devicePaths.begin();
        bool find = false;
        while (!find && j != devicePaths.end())
        {
            if ((*j).second == devicePathMaster &&
                (*j).second == CString(devicePathSlaves[i].c_str()))
            {
                find = true;

                //open slave driver
                handleSlaves.push_back(fpOpen((*j).first));
                fpAddSlave(handleMaster, handleSlaves[handleSlaves.size() - 1]);
                nDevicesOpen++;
            }
            ++j;
        }
    }

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">size for 1 channel, 1 block: " << m_nSamplePerSentBlock << "\n";


    //initialized the buffer
    sampling   = m_header.getSamplingFrequency() * 1000;
    bufferSize = MAX_BUFFER_SIZE;
    if (!fpSetSignalBuffer(handleMaster, &sampling, &bufferSize))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "For allocate the buffer\n";
        return false;
    }
    m_header.setSamplingFrequency(sampling / 1000);
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Maximum sample rate =" << uint32_t(sampling / 1000) << "Hz\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Maximum Buffer size =" << uint32_t(bufferSize) << "\n";

    const BOOLEAN start = fpStart(handleMaster);
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Start handle state: " << uint32_t(fpGetDeviceState(handleMaster)) << "\n";
    if (!start)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Start handle failed\n";
        this->uninitialize();
        return false;
    }

    //get informations of the format signal for all channels of the Master Handle
    psignal_format_t format = fpGetSignalFormat(handleMaster, nullptr);

    if (format != nullptr)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Master device name: " << (char*)format[0].PortName << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Nb channels: " << uint32_t(format[0].Elements) << "\n\n";
        nTotalChannels    = format[0].Elements;
        islBufferUnsigned = format[0].Format == 0;
        for (uint32_t i = 0; i < format[0].Elements; ++i)
        {
            exponentChannels.push_back(format[i].UnitExponent + 6/*changed measure unit in V*/);
            unitGains.push_back(format[i].UnitGain);
            unitOffSets.push_back(format[i].UnitOffSet);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "channel[" << i << "]: Exponent=" << exponentChannels[i] <<
                    " unitGain=" << unitGains[i] <<
                    " offSet=" << unitOffSets[i] <<
                    " format data signed=" << uint32_t(format[i].Format) <<
                    " type=" << uint32_t(format[i].Type) <<
                    " sub type=" << uint32_t(format[i].SubType) <<
                    " unit id=" << uint32_t(format[i].UnitId) << "\n";
            //if no trigger channel was found and this channel is a digital input, this channel contains information about Trigger
            if (m_nTriggerChannel == -1 && format[i].Type == 4) { m_nTriggerChannel = i; }
        }

        for (uint32_t j = 0; j < handleSlaves.size(); ++j)
        {
            format = fpGetSignalFormat(handleSlaves[j], nullptr);

            if (format != nullptr)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Slave device n" << j << " name: " << (char*)format[0].PortName << "\n";
                m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Nb channels: " << uint32_t(format[0].Elements) << "\n\n";
                nTotalChannels += format[0].Elements;
                for (uint32_t i = 0; i < format[0].Elements; ++i)
                {
                    exponentChannels.push_back(format[i].UnitExponent);
                    unitGains.push_back(format[i].UnitGain);
                    unitOffSets.push_back(format[i].UnitOffSet);
                }
            }
        }
    }
    //m_header.setChannelCount(nTotalChannels);
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Number of Channels: " << size_t(m_header.getChannelCount()) << "\n";
    m_sample = new float[m_header.getChannelCount() * m_nSamplePerSentBlock * 2];

    m_sampleIdx = 0;
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Sample driver size " << uint32_t(nTotalChannels * 4) << "\n";
    m_bufferSize = (bufferSize < (m_nSamplePerSentBlock * nTotalChannels * 4)) ? bufferSize
                       : (m_nSamplePerSentBlock * nTotalChannels * 32);
    bufferUnsigned = new ULONG[m_bufferSize];
    if (!islBufferUnsigned) { buffer = new LONG[m_bufferSize]; }

    //activate the mode Impedance of the device
    if (m_checkImpedance)
    {
        if (!measureMode(MEASURE_MODE_IMPEDANCE, IC_OHM_005))
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error impedance measure mode ic_ohm_005\n";
            return false;
        }
    }
    return true;
}

bool CDriverTMSiRefa32B::start()
{
    if (!m_valid || !m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">start TMSI\n";
    if (m_checkImpedance) { measureMode(MEASURE_MODE_NORMAL, 0); }
    m_sampleIdx           = 0;
    m_totalSampleReceived = 0;
    return true;
}

bool CDriverTMSiRefa32B::loop()
{
    if (!m_valid || !m_driverCtx.isConnected()) { return false; }
    if (m_driverCtx.isStarted())
    {
        //get data receive by the driver
        if (fpGetSamples == nullptr)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "fpGetSample not load\n";
            return false;
        }
        //uint32_t nextSampleIdx = 0;

        //uint64_t elapsed = System::Time::zgetTime()-m_startTime;
        const ULONG size = fpGetSamples(handleMaster, (islBufferUnsigned) ? PULONG(bufferUnsigned) : PULONG(buffer), m_bufferSize);

        if (size < 1) { return true; }

        //number of samples contains in the data receive
        const uint32_t nSample = size / (nTotalChannels * 4);

        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "size=" << uint32_t(size) << " ;;number of sample received=" << uint32_t(nSample) <<
                " ;; Samp[" << 0
                << "]=" << uint32_t((islBufferUnsigned) ? bufferUnsigned[0] : buffer[0]) << ";; "
                << uint32_t((islBufferUnsigned) ? bufferUnsigned[1] : buffer[1]) << "\n";

        //index of the data buffer
        uint32_t indexBuffer = 0;

        while (indexBuffer < nSample)//-m_autoRemovedSampleCount)
        {
            //take the minimum value between the size for complete the current block and the size of data received
            ULONG min;
            if (m_nSamplePerSentBlock - m_sampleIdx < (nSample - indexBuffer)) { min = m_nSamplePerSentBlock - m_sampleIdx; }
            else { min = nSample - indexBuffer; }

            //loop on the channel
            for (size_t i = 0; i < m_header.getChannelCount(); ++i)
            {
                //loop on the samples by channel
                for (uint32_t j = 0; j < min; ++j)
                {
                    // save the data of one sample for one channel on the table
                    if (nTotalChannels <= i) { m_sample[m_sampleIdx + j + i * m_nSamplePerSentBlock] = 0; }
                    else if (islBufferUnsigned)
                    {
                        m_sample[m_sampleIdx + j + i * m_nSamplePerSentBlock] = float(
                            (float(bufferUnsigned[(indexBuffer + j) * nTotalChannels + i]) * unitGains[i] + unitOffSets[i]) * pow(
                                10., double(exponentChannels[i])));
                    }
                    else
                    {
                        m_sample[m_sampleIdx + j + i * m_nSamplePerSentBlock] = float(
                            (float(buffer[(indexBuffer + j) * nTotalChannels + i]) * unitGains[i] + unitOffSets[i]) * pow(
                                10., double(exponentChannels[i])));
                    }
                }
            }

            for (uint32_t j = 0; j < min; ++j)
            {
                uint32_t trigger;
                if (islBufferUnsigned) { trigger = bufferUnsigned[(indexBuffer + j) * nTotalChannels + m_nTriggerChannel]; }
                else { trigger = buffer[(indexBuffer + j) * nTotalChannels + m_nTriggerChannel]; }
                //std::cout<<trigger<<" ";
                trigger = ~trigger;
                trigger &= 255;

                if (m_lastTriggerValue != trigger)
                {
                    //uint32_t indexStimulation = uint32_t(m_stimSet.size());
                    const uint64_t stimulationTime = CTime(m_header.getSamplingFrequency(), uint64_t(m_sampleIdx + j)).time();
                    m_stimSet.push_back(trigger, stimulationTime, 0);
                    m_lastTriggerValue = trigger;
                }
            }
            //Calculate the number of index receive on the block
            m_sampleIdx += min;
            indexBuffer += min;
            m_totalSampleReceived += min;
            //see if the block is complete
            if (m_sampleIdx >= m_nSamplePerSentBlock)
            {
                //sent the data block
                m_callback->setSamples(m_sample);
                m_callback->setStimulationSet(m_stimSet);
                m_stimSet.clear();
                m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

                //calculate the index of the new block
                m_sampleIdx -= m_nSamplePerSentBlock;
            }
        }
    }
    else if (m_checkImpedance)
    {
        //get Impedance value
        //get size of data receive
        const ULONG size = fpGetSamples(handleMaster, PULONG(bufferUnsigned), m_bufferSize);
        for (uint32_t i = 0; i < size / sizeof(ULONG) && i < nTotalChannels; ++i) { m_driverCtx.updateImpedance(i, bufferUnsigned[i] * 1000); }
    }
    else { fpGetSamples(handleMaster, PULONG(bufferUnsigned), m_bufferSize); }
    return true;
}

bool CDriverTMSiRefa32B::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Stop TMSI\n";

    if (m_checkImpedance) { measureMode(MEASURE_MODE_IMPEDANCE, IC_OHM_005); }
    m_sampleIdx           = 0;
    m_totalSampleReceived = 0;
    return true;
}

bool CDriverTMSiRefa32B::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << ">Uninit TMSI\n";

    //stop the driver
    BOOLEAN stop = TRUE;

    for (uint32_t i = 0; i < handleSlaves.size(); ++i) { stop = stop && fpStop(handleSlaves[i]); }
    stop = stop && fpStop(handleMaster);
    if (!stop)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Stop handler\n";
        this->uninitialize();
        return false;
    }
    for (uint32_t i = 0; i < handleSlaves.size(); ++i) { fpClose(handleSlaves[i]); }
    handleSlaves.clear();
    fpClose(handleMaster);
    handleMaster = nullptr;
    for (uint32_t i = 0; i < handleSlaves.size(); ++i) { fpClose(handleSlaves[i]); }
    devicePathSlaves.clear();
    devicePathMaster = "";
    devicePaths.clear();
    delete[] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverTMSiRefa32B::configure()
{
    //refresh the information of the device connected
    refreshDevicePath();

    CConfigurationTMSIRefa32B config(Directories::getDataDir() + "/applications/acquisition-server/interface-TMSI-Refa32B.ui");
    //create a vector with all name of device connected
    std::vector<std::string> paths;
    for (auto i = devicePaths.begin(); i != devicePaths.end(); ++i) { paths.push_back(std::string((*i).second.toASCIIString())); }

    //call configuration frame
    config.setDeviceList(paths, &std::string(devicePathMaster.toASCIIString()), &devicePathSlaves);

    if (!config.configure(m_header)) { return false; }

    m_settings.save();
    return true;
}

bool CDriverTMSiRefa32B::refreshDevicePath() const
{
    devicePaths.clear();

    //get the number of devices connected
    ULONG maxDevices = 0;
    if (fpGetInstanceId == nullptr)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Initialized the device, fpGetNrDevice not load\n";
        return false;
    }
    fpGetInstanceId(0, TRUE, &maxDevices);

    for (uint32_t i = 0; i < maxDevices; ++i)
    {
        TCHAR deviceName[40] = "Unknown Device";
        ULONG serialNumber   = 0;

        // get the device path connected
        PSP_DEVICE_PATH device = fpGetInstanceId(i, TRUE, &maxDevices);

        // get the name corresponding to this device
        const HKEY hKey = fpOpenRegKey(device);
        if (hKey != INVALID_HANDLE_VALUE)
        {
            //get the serial number of the device
            ULONG sizeSerial = sizeof(serialNumber);
            ::RegQueryValueEx(hKey, "DeviceSerialNumber", nullptr, nullptr, PBYTE(&serialNumber), &sizeSerial);

            //get the name of the device
            ULONG sizeDesc = sizeof(deviceName);
            ::RegQueryValueEx(hKey, "DeviceDescription", nullptr, nullptr, PBYTE(&deviceName[0]), &sizeDesc);
            //put the device path and it name in the map devicePaths
            devicePaths[device] = (deviceName + std::to_string(serialNumber)).c_str();
            RegCloseKey(hKey);
        }
    }

    //verify if the device Master is connected
    const std::string pathMaster = devicePathMaster.toASCIIString();
    devicePathMaster             = CString("");
    if (!pathMaster.empty())
    {
        auto index = devicePaths.begin();
        while (devicePathMaster == CString("") && index != devicePaths.end())
        {
            std::string deviceName = (*index).second.toASCIIString();
            devicePathMaster       = (pathMaster == deviceName) ? pathMaster.c_str() : nullptr;
            ++index;
        }
    }

    //verify if all device slaves are connected
    std::vector<std::string> pathSlaves;
    for (uint32_t i = 0; i < devicePathSlaves.size(); ++i)
    {
        for (auto j = devicePaths.begin(); j != devicePaths.end(); ++j)
        {
            if ((*j).second == CString(devicePathSlaves[i].c_str()))
            {
                pathSlaves.push_back(devicePathSlaves[i]);
                break;
            }
        }
    }
    devicePathSlaves.clear();
    devicePathSlaves  = pathSlaves;
    nDevicesConnected = maxDevices;
    return true;
}

bool CDriverTMSiRefa32B::measureMode(const uint32_t mode, const uint32_t info)
{
    FEATURE_MODE fMode;
    fMode.Feature.FeatureId = DEVICE_FEATURE_MODE;
    fMode.Feature.Info      = info;
    fMode.Mode              = mode;
    if (!fpDeviceFeature(handleMaster, &fMode, sizeof(FEATURE_MODE), nullptr, 0)) { return false; }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyNeXus
