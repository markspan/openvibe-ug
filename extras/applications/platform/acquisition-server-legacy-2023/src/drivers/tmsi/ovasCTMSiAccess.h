///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#pragma once

#if defined TARGET_OS_Windows

#include "ovas_base.h"
#include "Windows.h"
//#include "TMSiSDK.h"

#include <map>
#include <vector>

// TMSi Declared Defines, Enums and Types

// Measurement modes:
#define MEASURE_MODE_NORMAL         ((ULONG)0x0)
#define MEASURE_MODE_IMPEDANCE      ((ULONG)0x1)
#define MEASURE_MODE_CALIBRATION    ((ULONG)0x2)

#define MEASURE_MODE_IMPEDANCE_EX   ((ULONG)0x3)
#define MEASURE_MODE_CALIBRATION_EX ((ULONG)0x4)

// for MEASURE_MODE_IMPEDANCE:
#define IC_OHM_002  0       ///< 2K Impedance limit */
#define IC_OHM_005  1       ///<  5K Impedance limit */
#define IC_OHM_010  2       ///<  10K Impedance limit */
#define IC_OHM_020  3       ///<  20K Impedance limit */
#define IC_OHM_050  4       ///<  50K Impedance limit */
#define IC_OHM_100  5       ///<  100K Impedance limit */
#define IC_OHM_200  6       ///<  200K Impedance limit */

// for MEASURE_MODE_CALIBRATION:
#define IC_VOLT_050 0       ///< 50 uV t-t Calibration voltage */
#define IC_VOLT_100 1       ///< 100 uV t-t Calibration voltage */
#define IC_VOLT_200 2       ///< 200 uV t-t Calibration voltage */
#define IC_VOLT_500 3       ///< 500 uV t-t Calibration voltage */

// for Signat Format
#define SF_UNSIGNED 0x0     ///< Unsigned integer
#define SF_INTEGER  0x1     ///< signed integer

// integer overflow value for analog channels
#define OVERFLOW_32BITS ((long) 0x80000000)

// Get Signal info
#define SIGNAL_NAME 40

typedef struct SSignalFormat
{
    ULONG Size;             // Size of this structure
    ULONG Elements;         // Number of elements in list

    ULONG Type;             // One of the signal types above
    ULONG SubType;          // One of the signal sub-types above
    ULONG Format;           // Float / Integer / Asci / Ect..
    ULONG Bytes;            // Number of bytes per sample including subsignals

    FLOAT UnitGain;
    FLOAT UnitOffSet;
    ULONG UnitId;
    LONG UnitExponent;

    WCHAR Name[SIGNAL_NAME];

    ULONG Port;
    WCHAR PortName[SIGNAL_NAME];
    ULONG SerialNumber;
} signal_format_t, *psignal_format_t;

// This structure contains information about the possible configuration of the frontend
typedef struct SFrontEndInfo
{
    unsigned short NrOfChannels;        ///<  Current number of channels used */
    unsigned short SampleRateSetting;   ///<  Current sample rate setting (a.k.a. base sample rate divider ) */
    unsigned short Mode;                ///<  operating mode */
    unsigned short maxRS232;
    unsigned long Serial;               ///<  Serial number */
    unsigned short NrExg;               ///<  Number of Exg channels in this device */
    unsigned short NrAux;               ///<  Number of Aux channels in this device */
    unsigned short HwVersion;           ///<  Version number for the hardware */
    unsigned short SwVersion;           ///<  Version number of the embedded software */
    unsigned short RecBufSize;          ///<  Used for debugging only */
    unsigned short SendBufSize;         ///<  Used for debugging only */
    unsigned short NrOfSwChannels;      ///<  Max. number of channels supported by this device */
    unsigned short BaseSf;              ///<  Max. sample frequency */
    unsigned short Power;
    unsigned short Check;
} front_end_info_t, *pfront_end_info_t;

// Enum defined based on the communication methods from tmsi_ext_frontend_info_type_t
typedef enum ETmSiConnection
{
    TMSiConnectionUndefined = 0,        ///< Undefined connection, indicates programming error */
    TMSiConnectionFiber,                ///< Obsolete, do not use */
    TMSiConnectionBluetooth,            ///< Bluetooth connection with Microsoft driver */
    TMSiConnectionUSB,                  ///< USB 2.0 connection direct */
    TMSiConnectionWifi,                 ///< Network connection, Ip-adress and port needed, wireless */
    TMSiConnectionNetwork               ///< Network connection, Ip-adress and port needed, wired */
} tmsi_connection_type_t;


// Mobita specific: This structure contains information about the current battery state
typedef struct STmSiBatReport
{
    short Temp;                         ///<  Battery temperatur in degree Celsius (�C) */
    short Voltage;                      ///<  Battery Voltage in milliVolt  (mV) */
    short Current;                      ///<  Battery Current in milliAmpere (mA) */
    short AccumCurrent;                 ///<  Battery Accumulated Current in milliAmpere (mA) */
    short AvailableCapacityInPercent;   ///<  Available battery Capacity In Percent, range 0-100 */
    unsigned short DoNotUse1;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse2;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse3;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse4;           ///<  Do not use, reserved for future use */
} tmsi_bat_report_type_t;

// Mobita specific: This structure contains information about the current state of the internal storage
typedef struct STmSiStorageReport
{
    uint32_t StructSize;                ///<  Size of struct in words */
    uint32_t TotalSize;                 ///<  Total size of the internal storage in MByte (=1024x1024 bytes) */
    uint32_t UsedSpace;                 ///<  Used space on the internal storage in MByte (=1024x1024 bytes)*/
    uint32_t SDCardCID[4];              ///<  The CID register of the current SD-Card. */
    unsigned short DoNotUse1;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse2;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse3;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse4;           ///<  Do not use, reserved for future use */
} tmsi_storage_report_type_t;

// Mobita specific: This structure contains information about the current and past use of the Mobita
typedef struct STmSiDeviceReport
{
    uint32_t AdapterSN;                 ///<  Serial number of the current connected Adapter */
    uint32_t AdapterStatus;             ///<  0=Unknown; 1=Ok;2=MemError */
    uint32_t AdapterCycles;             ///<  Number of connections made by the Adapter. */
    uint32_t MobitaSN;                  ///<  Serial number of the Mobita */
    uint32_t MobitaStatus;              ///<  Statis of the Mobita : 0=Unknown; 1=Ok;2=MemError;3=BatError; */
    uint32_t MobitaCycles;              ///<  Number of adapter connections made by the Mobita */
    unsigned short DoNotUse1;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse2;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse3;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse4;           ///<  Do not use, reserved for future use */
} tmsi_device_report_type_t;

// Mobita specific: This structure contains information about the current sampling configuration
typedef struct STmSiExtFrontendInfo
{
    unsigned short CurrentSamplerate;   ///<  in Hz */
    unsigned short CurrentInterface;    ///<  0 = Unknown; 1 = Fiber;  2 = Bluetooth; 3 = USB; 4 = WiFi; 5 = Network*/
    unsigned short CurrentBlockType;    ///<  The blocktype used to send sample data for the selected CurrentFs and selected CurrentInterface */
    unsigned short DoNotUse1;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse2;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse3;           ///<  Do not use, reserved for future use */
    unsigned short DoNotUse4;           ///<  Do not use, reserved for future use */
} tmsi_ext_frontend_info_type_t;

//----------- TYPE ---------------------

#define CHANNELTYPE_UNKNOWN 0
#define CHANNELTYPE_EXG 1
#define CHANNELTYPE_BIP 2
#define CHANNELTYPE_AUX 3
#define CHANNELTYPE_DIG 4
#define CHANNELTYPE_TIME 5
#define CHANNELTYPE_LEAK 6
#define CHANNELTYPE_PRESSURE 7
#define CHANNELTYPE_ENVELOPE 8
#define CHANNELTYPE_MARKER 9
#define CHANNELTYPE_SAW 10
#define CHANNELTYPE_SAO2 11

#define MAX_BUFFER_SIZE 0xFFFFFFFF

typedef BOOLEAN ( __stdcall * POPEN )(void* handle, const char* deviceLocator);
typedef BOOLEAN ( __stdcall * PCLOSE )(HANDLE handle);
typedef BOOLEAN ( __stdcall * PSTART)(IN HANDLE handle);
typedef BOOLEAN ( __stdcall * PSTOP)(IN HANDLE handle);
typedef BOOLEAN ( __stdcall * PSETSIGNALBUFFER)(IN HANDLE handle,IN OUT PULONG sampling,IN OUT PULONG size);
typedef BOOLEAN ( __stdcall * PGETBUFFERINFO)(IN HANDLE handle,OUT PULONG overflow,OUT PULONG percentFull);
typedef LONG ( __stdcall * PGETSAMPLES)(IN HANDLE handle,OUT PULONG sampleBuffer,IN ULONG size);
typedef psignal_format_t ( __stdcall * PGETSIGNALFORMAT)(IN HANDLE handle, IN OUT char* frontEndName);
typedef BOOLEAN ( __stdcall * PFREE)(IN VOID* memory);
typedef HANDLE ( __stdcall * PLIBRARYINIT)(IN tmsi_connection_type_t givenConnectionType, IN OUT int* errorCode);
typedef int ( __stdcall * PLIBRARYEXIT)(IN HANDLE handle);
typedef BOOLEAN ( __stdcall * PGETFRONTENDINFO)(IN HANDLE handle, IN OUT front_end_info_t* frontEndInfo);
typedef BOOLEAN ( __stdcall * PSETRTCTIME)(IN HANDLE handle,IN SYSTEMTIME* inTime);
typedef BOOLEAN ( __stdcall * PGETRTCTIME)(IN HANDLE handle,IN SYSTEMTIME* inTime);
typedef int ( __stdcall * PGETERRORCODE)(IN HANDLE handle);
typedef const char* ( __stdcall * PGETERRORCODEMESSAGE)(IN HANDLE handle, IN int errorCode);
typedef char** ( __stdcall * PGETDEVICELIST)(IN HANDLE handle, IN OUT int* nFrontEnds);
typedef void ( __stdcall * PFREEDEVICELIST)(HANDLE handle, int nFrontEnds, char** deviceList);
typedef BOOLEAN ( __stdcall * PSETREFCALCULATION)(IN HANDLE handle, int onOrOff);
typedef BOOLEAN ( __stdcall * PSETMEASURINGMODE)(IN HANDLE handle,IN ULONG mode, IN int value);
typedef BOOLEAN ( __stdcall * PGETCONNECTIONPROPERTIES)(IN HANDLE handle, IN OUT uint32_t* signalStrength, IN OUT uint32_t* nCrcErrors,
                                                        IN OUT uint32_t* nSampleBlocks);
typedef BOOLEAN ( __stdcall * PGETEXTFRONTENDINFO)(IN HANDLE handle, IN OUT tmsi_ext_frontend_info_type_t* extFrontEndInfo,
                                                   tmsi_bat_report_type_t* batteryReport, tmsi_storage_report_type_t* storageReport,
                                                   tmsi_device_report_type_t* deviceReport);


// END TMSi

namespace OpenViBE {
namespace AcquisitionServer {
class IDriverContext;
class IDriverCallback;
class CHeader;

class CTMSiAccess
{
public:
    explicit CTMSiAccess(IDriverContext& ctx);
    ~CTMSiAccess();

    std::map<CString, std::pair<ETmSiConnection, int>> getConnectionProtocols() const { return m_connectionProtocols; }

    // Initialize the TMSi library with the currently chosen protocol
    bool initializeTMSiLibrary(const char* connectionProtocol);

    // Open a frontend (identified by a string) on the currently set protocol
    bool openFrontEnd(const char* deviceID);

    // Close the currently opened frontend
    bool closeFrontEnd();

    // Returns a vector of available sampling frequencies of the device, in case of error returns an empty vector
    std::vector<unsigned long> discoverDeviceSamplingFrequencies();

    // Run diagnostics on the device, ask for FrontEndInfo and extFrontEndInfo
    bool runDiagnostics();

    bool getImpedanceTestingCapability(bool* hasImpedanceTestingAbility) const;

    // Initializes the SignalFormat structure inside the driver
    // Calculates number of EEG and additional Channels
    bool calculateSignalFormat(const char* deviceID);

    // Print the signal format into Trace Log
    bool printSignalFormat();

    // Return the number of EEG channels on the device (must call calculateSignalFormat first)
    uint32_t getMaximumEEGChannelCount() const { return m_nMaxEEGChannel; }

    // Return the number of all channels on the device (must call calculateSignalFormat first)
    uint32_t getActualChannelCount() const { return m_nActualChannel; }

    // Return the name of the channel at desired index
    CString getChannelName(size_t index) const;

    // Return the type of the channel at desired index
    CString getChannelType(size_t index) const;

    // Frees the SignalFormat structure in this object and in the library
    void freeSignalFormat();

    // Returns the list of devices found on the current protocol
    std::vector<CString> getDeviceList() const { return m_devices; }

    // ACQUISITION SETTINGS

    // Enable or disable the common average reference calculation
    bool setCommonModeRejection(bool isCommonModeRejectionEnabled);

    bool setActiveChannels(CHeader* header, const CString& additionalChannels);

    // sets the signal buffer to values known to be functional, returns false if they are inconsistent
    bool setSignalBuffer(unsigned long sampling, unsigned long bufferSizeInSamples);

    bool setSignalMeasuringModeToNormal();
    bool setSignalMeasuringModeToImpedanceCheck(int limit);
    bool setSignalMeasuringModeToCalibration();

    // ACQUISITION HANDLING

    bool startAcquisition();
    int getSamples(float* samples, IDriverCallback* driverCB, uint64_t nSamplePerSentBlock, uint32_t sampling);
    bool getImpedanceValues(std::vector<double>* impedanceValues);
    bool stopAcquisition();


    // Gets connection properties, such as signal strength or errors since last call and prints them to Trace Log
    bool getConnectionProperties() const;

private: // private variables

    // status holders
    bool m_isInitialized       = false;
    bool m_opened              = false;
    bool m_hasChannelStructure = false;
    bool m_hasBufferSet        = false;
    bool m_isStarted           = false;

    // informations about the last scanned protocol
    std::vector<CString> m_devices;

    // informations about the last scanned device
    psignal_format_t m_signalFormat;
    unsigned long m_maxBufferSize;

    unsigned long m_nMaxEEGChannel;
    unsigned long m_nActualChannel;
    std::vector<bool> m_isChannelsActivated;
    uint32_t m_nActiveChannel = 0;

    unsigned long m_signalBufferSizeInBytes;

    // buffer for stored signal
    unsigned long* m_sampleBuffer;
    // index of the current (last) sample in the buffer
    uint32_t m_lastSampleIndexInBuffer = 0;

    // device connection protocols
    std::map<CString, std::pair<ETmSiConnection, int>> m_connectionProtocols;

    IDriverContext& m_driverCtx;
    bool m_valid = false;

    CStimulationSet m_stimSet;
    uint32_t m_lastTriggerValue = 0;

    // private methods
    bool setSignalMeasuringMode(ULONG measuringMode, int value = 0);

    // DLL library handling members
    // The HANDLE type returned by the library is basically a void* (avoids including windows headers to the .h)
    HANDLE m_libraryHandle;

    template <typename T>
    void loadDLLFunct(T* functionPointer, const char* functionName);

    // TMSi Library functions
    POPEN m_fpOpen;
    PCLOSE m_fpClose;
    PSTART m_fpStart;
    PSTOP m_fpStop;
    PSETSIGNALBUFFER m_fpSetSignalBuffer;
    PGETBUFFERINFO m_fpGetBufferInfo;
    PGETSAMPLES m_fpGetSamples;
    PGETSIGNALFORMAT m_fpGetSignalFormat;
    PFREE m_fpFree;
    PLIBRARYINIT m_fpLibraryInit;
    PLIBRARYEXIT m_fpLibraryExit;
    PGETFRONTENDINFO m_fpGetFrontEndInfo;
    PSETRTCTIME m_fpSetRtcTime;
    PGETRTCTIME m_fpGetRtcTime;

    PGETERRORCODE m_fpGetErrorCode;
    PGETERRORCODEMESSAGE m_fpGetErrorCodeMessage;

    PGETDEVICELIST m_fpGetDeviceList;
    PFREEDEVICELIST m_fpFreeDeviceList;

    PGETCONNECTIONPROPERTIES m_fpGetConnectionProperties;
    PSETREFCALCULATION m_fpSetRefCalculation;
    PSETMEASURINGMODE m_fpSetMeasuringMode;
    PGETEXTFRONTENDINFO m_fpGetExtFrontEndInfo;

    /*
    // NeXus10MkII functionality
    PGETRANDOMKEY m_fpGetRandomKey;
    PUNLOCKFRONTEND m_fpUnlockFrontEnd;
    PGETOEMSIZE m_fpGetOEMSize;
    PSETOEMDATA m_fpSetOEMData;
    PGETOEMDATA m_fpGetOEMData;
    POPENFIRSTDEVICE m_oFopenFirstDevice;
    PSETSTORAGEMODE m_fpSetStorageMode;
    PGETDIGSENSORID m_fpGetDigSensorId;
    PGETDIGSENSORCONFIG m_fpGetDigSensorConfig;
    PGETDIGSENSORDATA m_fpGetDigSensorData;
    PGETFLASHSTATUS m_fpGetFlashStatus;
    PSTARTFLASHDATA m_fpStartFlashData;
    PGETFLASHSAMPLES m_fpGetFlashSamples;
    PSTOPFLASHDATA m_fpStopFlashData;
    PFLASHERASEMEMORY m_fpFlashEraseMemory;
    PSETFLASHDATA m_fpSetFlashData;
    */
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
