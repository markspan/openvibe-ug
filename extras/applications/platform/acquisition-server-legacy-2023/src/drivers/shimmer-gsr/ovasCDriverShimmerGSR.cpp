///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDriverShimmerGSR.cpp
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#if defined TARGET_OS_Windows

#include "ovasCDriverShimmerGSR.hpp"
#include "ovasCConfigurationShimmerGSR.hpp"

#include <toolkit/ovtk_all.h>
#include <algorithm>

#include <chrono>
#include <thread>

#include <Eigen/Eigen>


namespace OpenViBE {
namespace AcquisitionServer {

CDriverShimmerGSR::CDriverShimmerGSR(IDriverContext& ctx)
    :IDriver(ctx), m_settings("AcquisitionServer_Driver_ShimmerGSR", m_driverCtx.getConfigurationManager())
{
    m_settings.add("Header", &m_header);
    m_settings.add("SerialPortIndex", &m_portIndex);
    m_settings.add("SamplingFrequency", &m_samplingFrequency);

    m_settings.load();
}

CDriverShimmerGSR::~CDriverShimmerGSR() {}

const char* CDriverShimmerGSR::getName() { return "ShimmerGSR"; }

bool CDriverShimmerGSR::isConfigurable() { return true; } // change to false if your device is not configurable

// Called when clicking on "Driver Properties"
bool CDriverShimmerGSR::configure()
{
    detectDevices();

    // Change this line if you need to specify some references to your driver attribute that need configuration,
    // e.g. the connection ID.
    // This constructors returns after clicking on "Apply", in the "Device Configuration" window
    CConfigurationShimmerGSR config(
        Directories::getDataDir() + "/applications/acquisition-server/interface-ShimmerGSR.ui",
        m_serialPorts,
        m_portIndex,
        m_samplingFrequency
    );

    if (!config.configure(m_header)) { return false; }

    m_settings.save();

    m_port = m_serialPorts[m_portIndex];
    m_portSelected = true;

    std::stringstream ss;
    ss << "Selected port: COM" << m_port << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << ss.str();

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info <<
        "Before clicking the Connect button, make sure that your Shimmer device is on, " <<
        "and that it is either in \"Standby\" or \"RTC not set\" mode. (See Shimmer user manual). \n";

    return true;
}

// Called when clicking on "Connect"
bool CDriverShimmerGSR::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (!m_portSelected) { 
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "No serial port was selected.\n";
        return false;
    }

    if (m_driverCtx.isConnected()) { return false; }
    if (!m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    // ...
    // initialize hardware and get
    // available header information
    // from it
    // Using for example the connection ID provided by the configuration (m_connectionID)
    // ...

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Attempting to connect to device on port: " << m_serialPorts[m_portIndex] << "\n";

    const int serSuccess = initSPP(m_port);
    if (serSuccess == -1) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to open port, please check device and settings\n";
        return false;
    }

    // Saves parameters
    m_callback = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Successfuly connected to the device.\n";

    if (initializeDevice() == EXIT_FAILURE) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Device didn't respond to multiple requests. Possible causes are:\n" <<
        "       1. The selected port doesn't correspond to a Shimmer device\n" <<
        "       2. The Connect button was clicked before the Shimmer device was in \"Standby\" or \"RTC not set\" mode\n";
        return false;
    }

    // Builds up a buffer to store
    // acquired samples. This buffer
    // will be sent to the acquisition
    // server later

    m_sampleSize = m_numberOfChannels * nSamplePerSentBlock;
    m_sample.resize(m_sampleSize);

    m_dataArray.resize(m_packetSize);
    m_parsedData.resize(m_numberOfChannels+1); // m_numberOfChannels for sensors + 1 channel for timestamp
    m_realUnitsData.resize(m_numberOfChannels+1);

    return true;
}


// Called when clicking "Disconnect"
bool CDriverShimmerGSR::uninitialize()
{
    if (!m_driverCtx.isConnected()) return false;
    if (m_driverCtx.isStarted()) return false;

    closeSPP();

    m_callback = nullptr;

    return true;
}
// Called when clicking "Play"
bool CDriverShimmerGSR::start()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (m_driverCtx.isStarted()) { return false; }

    sendCommandToDevice(START_STREAMING_COMMAND);

    return true;
}

// Called when clicking "Stop"
bool CDriverShimmerGSR::stop()
{
    if (!m_driverCtx.isConnected()) return false;
    if (!m_driverCtx.isStarted()) return false;

    sendCommandToDevice(STOP_STREAMING_COMMAND);

    return true;
}

bool CDriverShimmerGSR::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    std::fill(m_sample.begin(), m_sample.end(), 0);

    unsigned char result[1] = { 0 };
    int bytesread = 0;

    bool sendSample = true;

    for (int sampleIndex = 0; sampleIndex < m_nSamplePerSentBlock; sampleIndex++) {
        
        do {
            bytesread = readSPP(result, 1);
        } while (bytesread == 0); // while nothing is read (sometimes, readSPP must be called twice before the device sends data)


        switch (result[0]) {
        case DATA_PACKET:
            for (int i = 0; i < m_packetSize; i++) {
                do {
                    bytesread = readSPP(result, 1);
                } while (bytesread == 0);
                m_dataArray[i] = result[0];
            }
            
            processDataPacket();

            for (int i = 1; i < m_numberOfChannels + 1; i++) { // timestamp (i = 0) is not sent
                int insert_idx = (i - 1) * m_nSamplePerSentBlock + sampleIndex;
                m_sample[insert_idx] = m_realUnitsData[i];
                m_realUnitsData[i] = 0; // reset this value for the next loop() call
            }

            break;
        case ACK_PROCESSED:
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Ack processed\n";
            break;
        default:
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Misaligned ByteStream Detected\n";

            sendSample = false;
            break;
        }
        result[0] = 0;
    }

    if (sendSample) {
        // ...
        // receive samples from hardware
        // put them the correct way in the sample array
        // whether the buffer is full, send it to the acquisition server
        //...
        m_callback->setSamples(m_sample.data());

        // When your sample buffer is fully loaded, 
        // it is advised to ask the acquisition server 
        // to correct any drift in the acquisition automatically.
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }


    return true;
}

// Higher level communication

int CDriverShimmerGSR::initializeDevice()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Initializing the device...\n";

    if (
        getFromDevice(GET_ACCEL_RANGE_COMMAND) +
        getFromDevice(GET_MAG_GAIN_COMMAND) +
        getFromDevice(GET_MPU9150_GYRO_RANGE_COMMAND) +
        getFromDevice(GET_ACCEL_SAMPLING_RATE_COMMAND) +
        getFromDevice(GET_INTERNAL_EXP_POWER_ENABLE_COMMAND) +
        getFromDevice(GET_MYID_COMMAND) +
        getFromDevice(GET_NSHIMMER_COMMAND) +
        getFromDevice(GET_BAUD_RATE_COMMAND) +

        getFromDeviceStr(GET_CENTER_COMMAND) + // Always returns EXIT_FAILURE, even in the ShimmerCapture software
        getFromDeviceStr(GET_SHIMMERNAME_COMMAND) +
        getFromDeviceStr(GET_EXPID_COMMAND) +
        getFromDeviceStr(GET_CONFIGTIME_COMMAND) +

        setAndGetSamplingRate() +
        inquiry()
        > 1) {
        return EXIT_FAILURE;
    }

    initializeShimmerTime();

    return EXIT_SUCCESS;
}

void CDriverShimmerGSR::sendCommandToDevice(unsigned char command)
{
    unsigned char instructionBuffer[1] = { command };
    writeSPP(instructionBuffer, 1);
}

// Serial Port Profile handling
int CDriverShimmerGSR::initSPP(const int port)
{
    // open the chosen serial port
    std::string portString = "\\\\.\\COM" + std::to_string(port);

    m_handleSerial = CreateFile(portString.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    std::stringstream ss;
    if (m_handleSerial == INVALID_HANDLE_VALUE) {

        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            ss << "Port COM" << port << " not found.\n";
        }
        else {
            ss << "Port COM" << port << " found, but corresponding device not found.\n";
        }
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << ss.str();
        return -1;
    }

    ss << "Port COM" << port << " and corresponding device found.\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << ss.str();

    // setup serial port parameters
    DCB dcbSerialParams = { 0 };

    if (!GetCommState(m_handleSerial, &dcbSerialParams)) {
        //error getting state
        return -1;
    }

    dcbSerialParams.BaudRate = 115200;

    if (!SetCommState(m_handleSerial, &dcbSerialParams)) {
        //error setting serial port state
        return -1;
    }

    // setup serial port timeouts
    COMMTIMEOUTS timeout = { 0 };
    timeout.ReadIntervalTimeout = 60;
    timeout.ReadTotalTimeoutConstant = 60;
    timeout.ReadTotalTimeoutMultiplier = 15;
    timeout.WriteTotalTimeoutConstant = 60;
    timeout.WriteTotalTimeoutMultiplier = 8;
    if (!SetCommTimeouts(m_handleSerial, &timeout)) {
        //handle error here
        return -1;
    }

    return 0;
}

void CDriverShimmerGSR::closeSPP() { CloseHandle(m_handleSerial); }

unsigned long CDriverShimmerGSR::writeSPP(unsigned char* buffer, const int nbBytes)
{
    DWORD dwBytesWritten = 0;
    if (!WriteFile(m_handleSerial, buffer, nbBytes, &dwBytesWritten, NULL)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error while writing to port\n";
    }
    return dwBytesWritten;
}

unsigned long CDriverShimmerGSR::readSPP(unsigned char* buffer, const int nbBytes)
{
    DWORD dwBytesRead = 0;
    ReadFile(m_handleSerial, buffer, nbBytes, &dwBytesRead, NULL);

    return int(dwBytesRead);
}


void CDriverShimmerGSR::detectDevices()
{

    m_serialPorts.clear();

    DWORD index = 0;
    LSTATUS s;

    do {
        s = registryRead("Hardware\\Devicemap\\Serialcomm\\", REG_SZ, index);
        index++;
    } while (s != ERROR_NO_MORE_ITEMS);

    std::sort(m_serialPorts.begin(), m_serialPorts.end());

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Found " << m_serialPorts.size() << " serial ports.\n";


}

LSTATUS CDriverShimmerGSR::registryRead(LPCTSTR subkey, DWORD type, DWORD index)
{
    HKEY key;

    TCHAR name[255];
    DWORD name_length = 255;

    TCHAR data[255];
    DWORD lpdata = 255;

    RegOpenKey(HKEY_LOCAL_MACHINE, subkey, &key);

    LSTATUS status = RegEnumValueA(key, index, (LPSTR)&name, &name_length, NULL, &type, (LPBYTE)&data, &lpdata);

    RegCloseKey(key);

    // convert to string and remove the first 3 characters (COM) so that only the port number remains
    std::string dataStr = ((std::string)data).erase(0, 3);

    // if there are still ports after this one, add the port to the list
    if (status != ERROR_NO_MORE_ITEMS) m_serialPorts.push_back(std::stoul(dataStr));

    return status;
}

int CDriverShimmerGSR::getFromDevice(unsigned char command)
{
    unsigned char response;
    std::string msg;
    int* attribute;

    switch (command) {
    case GET_ACCEL_RANGE_COMMAND:
        response = ACCEL_RANGE_RESPONSE;
        msg = "accelerometer range";
        attribute = &m_accelRange;
        break;

    case GET_MAG_GAIN_COMMAND:
        response = MAG_GAIN_RESPONSE;
        msg = "magnetometer gain";
        attribute = &m_magGain;
        break;

    case GET_MPU9150_GYRO_RANGE_COMMAND:
        response = MPU9150_GYRO_RANGE_RESPONSE;
        msg = "gyroscope range";
        attribute = &m_gyroRange;
        break;

    case GET_ACCEL_SAMPLING_RATE_COMMAND:
        response = ACCEL_SAMPLING_RATE_RESPONSE;
        msg = "accelerometer sampling rate";
        attribute = &m_accelSamplingRate;
        break;

    case GET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
        response = INTERNAL_EXP_POWER_ENABLE_RESPONSE;
        msg = "internal exp power setting";
        attribute = &m_internalExpPower;
        break;

    case GET_MYID_COMMAND:
        response = MYID_RESPONSE;
        msg = "multi shimmer setting";
        attribute = &m_myid;
        break;

    case GET_NSHIMMER_COMMAND:
        response = NSHIMMER_RESPONSE;
        msg = "nshimmer";
        attribute = &m_nshimmer;
        break;

    case GET_BAUD_RATE_COMMAND:
        response = BAUD_RATE_RESPONSE;
        msg = "baud rate";
        attribute = &m_baudRate;
        break;

    default:
        break;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Acquiring " << msg << "...\n";

    unsigned char result[1] = { 0 };
    int bytesread = 0;

    sendCommandToDevice(command);
    int timeout = 0;
    do {
        bytesread = readSPP(result, 1);
        timeout++;
    } while (result[0] != response && timeout < 5);

    // If the device doesn't respond after 5 attempts, give up (to avoid potential infinite loops)
    if (timeout == 5) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Device didn't send any response.\n";
        return EXIT_FAILURE;
    }

    bytesread = readSPP(result, 1);
    if (bytesread != 0) {
        *attribute = result[0];
    }

    return EXIT_SUCCESS;
}

int CDriverShimmerGSR::getFromDeviceStr(unsigned char command)
{

    unsigned char response;
    std::string msg;
    std::string* attribute;

    switch (command) {
    case GET_CENTER_COMMAND:
        response = CENTER_RESPONSE;
        msg = "center name";
        attribute = &m_centerName;
        break;
    case GET_SHIMMERNAME_COMMAND:
        response = SHIMMERNAME_RESPONSE;
        msg = "shimmer name";
        attribute = &m_shimmerName;
        break;
    case GET_EXPID_COMMAND:
        response = EXPID_RESPONSE;
        msg = "experiment ID";
        attribute = &m_expId;
        break;
    case GET_CONFIGTIME_COMMAND:
        response = CONFIGTIME_RESPONSE;
        msg = "configuration time";
        attribute = &m_configTime;
        break;
    default:
        break;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Acquiring " << msg << "...\n";

    unsigned char result[1] = { 0 };
    int bytesread = 0;

    sendCommandToDevice(command);
    int timeout = 0;
    do {
        bytesread = readSPP(result, 1);
        timeout++;
    } while (result[0] != response && timeout < 5);

    // If the device doesn't respond after 5 attempts, give up (to avoid potential infinite loops)
    // There seems to be a problem with GET_CENTER_COMMAND (E.G. the shimmer nevers sends it, even in the ShimmerCapture software)
    if (timeout == 5) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Device didn't send any response.\n";
        return EXIT_FAILURE;
    }

    bytesread = readSPP(result, 1);
    if (bytesread != 0) {
        const int bufferLength = result[0];
        std::string buffer = "";
        for (int i = 0; i < bufferLength; i++) {
            bytesread = readSPP(result, 1);
            std::string resultStr(1, result[0]);
            buffer = buffer + resultStr;
        }
        *attribute = buffer;
    }

    return EXIT_SUCCESS;
}

int CDriverShimmerGSR::setAndGetSamplingRate()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Acquiring sampling rate...\n";

    if (m_samplingFrequency > 0) { // if user chose a frenquency in the driver properties window
        int invRate = 32768 / m_samplingFrequency;
        unsigned char hexInvRate1 = invRate & 0x00FF;
        unsigned char hexInvRate2 = (invRate & 0xFF00) >> 8;

        unsigned char instructionBuffer[3] = { SET_SAMPLING_RATE_COMMAND, hexInvRate1, hexInvRate2 };
        writeSPP(instructionBuffer, 3);

        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Sampling frequency set to " << m_samplingFrequency << ".\n";
    }

    unsigned char result[1] = { 0 };
    int bytesread = 0;

    sendCommandToDevice(GET_SAMPLING_RATE_COMMAND);
    int timeout = 0;
    do {
        bytesread = readSPP(result, 1);
        timeout++;
    } while (result[0] != SAMPLING_RATE_RESPONSE && timeout < 5);

    // If the device doesn't respond after 5 attempts, give up (to avoid potential infinite loops)
    if (timeout == 5) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Device didn't send any response.\n";
        return EXIT_FAILURE;
    }
    
    bytesread = readSPP(result, 1);
    if (bytesread != 0) {
        int value = (int)result[0];
    
        bytesread = readSPP(result, 1);

        value += (int)(result[0] << 8 & 0xFF00);
        this->m_samplingRate = (double)32768 / value;
    
    }
    
    m_header.setSamplingFrequency(m_samplingRate);

    return EXIT_SUCCESS;
}

int CDriverShimmerGSR::inquiry()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Inquiry...\n";

    unsigned char result[1] = { 0 };
    int bytesread = 0;

    sendCommandToDevice(INQUIRY_COMMAND);
    int timeout = 0;
    do {
        bytesread = readSPP(result, 1);
        timeout++;
    } while (result[0] != INQUIRY_RESPONSE && timeout < 5);

    // If the device doesn't respond after 5 attempts, give up (to avoid potential infinite loops)
    if (timeout == 5) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Device didn't send any response.\n";
        return EXIT_FAILURE;
    }

    const int bufferFinalSize = 8;
    std::vector<unsigned char> buffer;
    buffer.reserve(bufferFinalSize);

    for (int i = 0; i < bufferFinalSize; i++) {
        bytesread = readSPP(result, 1);
        buffer.push_back(result[0]);
    }

    m_numberOfChannels = buffer[6];
    m_header.setChannelCount(m_numberOfChannels);

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << m_numberOfChannels << " sensors are enabled:" << "\n";

    std::vector<unsigned char> channelIds;
    channelIds.reserve(m_numberOfChannels);
    for (int i = 0; i < m_numberOfChannels; i++) {
        bytesread = readSPP(result, 1);
        channelIds.push_back(result[0]);
    }

    interpretInquiryResponse(channelIds);

    return EXIT_SUCCESS;
}

void CDriverShimmerGSR::interpretInquiryResponse(std::vector<unsigned char> channelIds)
{
    m_signalNameArray.resize(m_numberOfChannels + 1); // +1 for the timestamp
    m_signalDataTypeArray.resize(m_numberOfChannels + 1);

    m_signalNameArray[0] = "Timestamp";
    m_signalDataTypeArray[0] = "u24";
    int packetSize = 3;

    int enabledSensors = 0x00;

    int numberOfSensors = 0;

    for (int i = 0; i < m_numberOfChannels; i++) {
        if (channelIds[i] == 0x00) {
            m_signalNameArray[i + 1] = LOW_NOISE_ACCELEROMETER_X;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_A_ACCEL);
            m_header.setChannelName(numberOfSensors, LOW_NOISE_ACCELEROMETER_X);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << LOW_NOISE_ACCELEROMETER_X << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x01) {
            m_signalNameArray[i + 1] = LOW_NOISE_ACCELEROMETER_Y;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_A_ACCEL);
            m_header.setChannelName(numberOfSensors, LOW_NOISE_ACCELEROMETER_Y);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << LOW_NOISE_ACCELEROMETER_Y << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x02) {
            m_signalNameArray[i + 1] = LOW_NOISE_ACCELEROMETER_Z;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_A_ACCEL);
            m_header.setChannelName(numberOfSensors, LOW_NOISE_ACCELEROMETER_Z);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << LOW_NOISE_ACCELEROMETER_Z << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x03) {
            m_signalNameArray[i + 1] = V_SENSE_BATT;
            m_signalDataTypeArray[i + 1] = "i16";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_VBATT);
            m_header.setChannelName(numberOfSensors, V_SENSE_BATT);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << V_SENSE_BATT << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x04) {
            m_signalDataTypeArray[i + 1] = "i16";
            packetSize = packetSize + 2;
            m_signalNameArray[i + 1] = WIDE_RANGE_ACCELEROMETER_X;
            enabledSensors = (enabledSensors | SENSOR_D_ACCEL);
            m_header.setChannelName(numberOfSensors, WIDE_RANGE_ACCELEROMETER_X);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << WIDE_RANGE_ACCELEROMETER_X << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x05) {
            m_signalDataTypeArray[i + 1] = "i16";
            packetSize = packetSize + 2;
            m_signalNameArray[i + 1] = WIDE_RANGE_ACCELEROMETER_Y;
            enabledSensors = (enabledSensors | SENSOR_D_ACCEL);
            m_header.setChannelName(numberOfSensors, WIDE_RANGE_ACCELEROMETER_Y);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << WIDE_RANGE_ACCELEROMETER_Y << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x06) {
            m_signalDataTypeArray[i + 1] = "i16";
            packetSize = packetSize + 2;
            m_signalNameArray[i + 1] = WIDE_RANGE_ACCELEROMETER_Z;
            enabledSensors = (enabledSensors | SENSOR_D_ACCEL);
            m_header.setChannelName(numberOfSensors, WIDE_RANGE_ACCELEROMETER_Z);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << WIDE_RANGE_ACCELEROMETER_Z << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x07) {
            m_signalNameArray[i + 1] = MAGNETOMETER_X;
            m_signalDataTypeArray[i + 1] = "i16";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_LSM303DLHC_MAG);
            m_header.setChannelName(numberOfSensors, MAGNETOMETER_X);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << MAGNETOMETER_X << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x08) {
            m_signalNameArray[i + 1] = MAGNETOMETER_Y;
            m_signalDataTypeArray[i + 1] = "i16";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_LSM303DLHC_MAG);
            m_header.setChannelName(numberOfSensors, MAGNETOMETER_Y);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << MAGNETOMETER_Y << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x09) {
            m_signalNameArray[i + 1] = MAGNETOMETER_Z;
            m_signalDataTypeArray[i + 1] = "i16";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_LSM303DLHC_MAG);
            m_header.setChannelName(numberOfSensors, MAGNETOMETER_Z);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << MAGNETOMETER_Z << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x0A) {
            m_signalNameArray[i + 1] = GYROSCOPE_X;
            m_signalDataTypeArray[i + 1] = "i16*";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_MPU9150_GYRO);
            m_header.setChannelName(numberOfSensors, GYROSCOPE_X);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << GYROSCOPE_X << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x0B) {
            m_signalNameArray[i + 1] = GYROSCOPE_Y;
            m_signalDataTypeArray[i + 1] = "i16*";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_MPU9150_GYRO);
            m_header.setChannelName(numberOfSensors, GYROSCOPE_Y);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << GYROSCOPE_Y << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x0C) {
            m_signalNameArray[i + 1] = GYROSCOPE_Z;
            m_signalDataTypeArray[i + 1] = "i16*";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_MPU9150_GYRO);
            m_header.setChannelName(numberOfSensors, GYROSCOPE_Z);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << GYROSCOPE_Z << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x0D) {
            m_signalNameArray[i + 1] = EXTERNAL_ADC_A7;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_EXT_A7);
            m_header.setChannelName(numberOfSensors, EXTERNAL_ADC_A7);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXTERNAL_ADC_A7 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x0E) {
            m_signalNameArray[i + 1] = EXTERNAL_ADC_A6;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_EXT_A6);
            m_header.setChannelName(numberOfSensors, EXTERNAL_ADC_A6);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXTERNAL_ADC_A6 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x0F) {
            m_signalNameArray[i + 1] = EXTERNAL_ADC_A15;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_EXT_A15);
            m_header.setChannelName(numberOfSensors, EXTERNAL_ADC_A15);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXTERNAL_ADC_A15 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x10) {
            m_signalNameArray[i + 1] = INTERNAL_ADC_A1;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_INT_A1);
            m_header.setChannelName(numberOfSensors, INTERNAL_ADC_A1);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << INTERNAL_ADC_A1 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x11) {
            m_signalNameArray[i + 1] = INTERNAL_ADC_A12;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_INT_A12);
            m_header.setChannelName(numberOfSensors, INTERNAL_ADC_A12);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << INTERNAL_ADC_A12 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x12) {
            m_signalNameArray[i + 1] = INTERNAL_ADC_A13;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_INT_A13);
            m_header.setChannelName(numberOfSensors, INTERNAL_ADC_A13);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << INTERNAL_ADC_A13 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x13) {
            m_signalNameArray[i + 1] = INTERNAL_ADC_A14;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_INT_A14);
            m_header.setChannelName(numberOfSensors, INTERNAL_ADC_A14);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << INTERNAL_ADC_A14 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x1A) {
            m_signalNameArray[i + 1] = TEMPERATURE;
            m_signalDataTypeArray[i + 1] = "u16r";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_BMP180_PRESSURE);
            m_header.setChannelName(numberOfSensors, TEMPERATURE);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << TEMPERATURE << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x1B) {
            m_signalNameArray[i + 1] = PRESSURE;
            m_signalDataTypeArray[i + 1] = "u24r";
            packetSize = packetSize + 3;
            enabledSensors = (enabledSensors | SENSOR_BMP180_PRESSURE);
            m_header.setChannelName(numberOfSensors, PRESSURE);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << PRESSURE << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x1C) {
            m_signalNameArray[i + 1] = GSR;
            m_signalDataTypeArray[i + 1] = "u16";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_GSR);
            m_header.setChannelName(numberOfSensors, GSR);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << GSR << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x1D) {
            m_signalNameArray[i + 1] = EXG1_STATUS;
            m_signalDataTypeArray[i + 1] = "u8";
            packetSize = packetSize + 1;
            m_header.setChannelName(numberOfSensors, EXG1_STATUS);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG1_STATUS << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x1E) { //EXG
            m_signalNameArray[i + 1] = EXG1_CH1;
            m_signalDataTypeArray[i + 1] = "i24r";
            packetSize = packetSize + 3;
            enabledSensors = (enabledSensors | SENSOR_EXG1_24BIT);
            m_header.setChannelName(numberOfSensors, EXG1_CH1);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG1_CH1 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x1F) { //EXG
            m_signalNameArray[i + 1] = EXG1_CH2;
            m_signalDataTypeArray[i + 1] = "i24r";
            packetSize = packetSize + 3;
            enabledSensors = (enabledSensors | SENSOR_EXG1_24BIT);
            m_header.setChannelName(numberOfSensors, EXG1_CH2);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG1_CH2 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x20) { //EXG
            m_signalNameArray[i + 1] = EXG2_STATUS;
            m_signalDataTypeArray[i + 1] = "u8";
            packetSize = packetSize + 1;
            m_header.setChannelName(numberOfSensors, EXG2_STATUS);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG2_STATUS << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x21) { //EXG
            m_signalNameArray[i + 1] = EXG2_CH1;
            m_signalDataTypeArray[i + 1] = "i24r";
            packetSize = packetSize + 3;
            enabledSensors = (enabledSensors | SENSOR_EXG2_24BIT);
            m_header.setChannelName(numberOfSensors, EXG2_CH1);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG2_CH1 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x22) { //EXG
            m_signalNameArray[i + 1] = EXG2_CH2;
            m_signalDataTypeArray[i + 1] = "i24r";
            packetSize = packetSize + 3;
            enabledSensors = (enabledSensors | SENSOR_EXG2_24BIT);
            m_header.setChannelName(numberOfSensors, EXG2_CH2);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG2_CH2 << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x23) { //EXG
            m_signalNameArray[i + 1] = EXG1_CH1_16BIT;
            m_signalDataTypeArray[i + 1] = "i16r";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_EXG1_16BIT);
            m_header.setChannelName(numberOfSensors, EXG1_CH1_16BIT);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG1_CH1_16BIT << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x24) { //EXG
            m_signalNameArray[i + 1] = EXG1_CH2_16BIT;
            m_signalDataTypeArray[i + 1] = "i16r";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_EXG1_16BIT);
            m_header.setChannelName(numberOfSensors, EXG1_CH2_16BIT);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG1_CH2_16BIT << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x25) { //EXG
            m_signalNameArray[i + 1] = EXG2_CH1_16BIT;
            m_signalDataTypeArray[i + 1] = "i16r";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_EXG2_16BIT);
            m_header.setChannelName(numberOfSensors, EXG2_CH1_16BIT);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG2_CH1_16BIT << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x26) { //EXG
            m_signalNameArray[i + 1] = EXG2_CH2_16BIT;
            m_signalDataTypeArray[i + 1] = "i16r";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_EXG2_16BIT);
            m_header.setChannelName(numberOfSensors, EXG2_CH2_16BIT);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << EXG2_CH2_16BIT << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x27) { //BRIDGE AMPLIFIER
            m_signalNameArray[i + 1] = BRIGE_AMPLIFIER_HIGH;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_BRIDGE_AMP);
            m_header.setChannelName(numberOfSensors, BRIGE_AMPLIFIER_HIGH);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << BRIGE_AMPLIFIER_HIGH << "\n";
            numberOfSensors++;
        }
        else if (channelIds[i] == 0x28) { //BRIDGE AMPLIFIER
            m_signalNameArray[i + 1] = BRIGE_AMPLIFIER_LOW;
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
            enabledSensors = (enabledSensors | SENSOR_BRIDGE_AMP);
            m_header.setChannelName(numberOfSensors, BRIGE_AMPLIFIER_LOW);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << BRIGE_AMPLIFIER_LOW << "\n";
            numberOfSensors++;
        }
        else {
            m_signalNameArray[i + 1] = "";
            m_signalDataTypeArray[i + 1] = "u12";
            packetSize = packetSize + 2;
        }
    }

    m_enabledSensors = enabledSensors;
    m_packetSize = packetSize;

}

void CDriverShimmerGSR::initializeShimmerTime() //writeRealWorldClock in the original code
{
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::system_clock::now()).time_since_epoch()).count();
    long long currentTimeTicks = currentTime * 32.768; // Convert miliseconds to clock ticks

    unsigned char timeArray[9] = { 0 }; //magic number ? already there in shimmers code
    timeArray[0] = SET_RWC_COMMAND;

    for (int i = 0; i < 8; i++) {
        timeArray[i+1] = (currentTimeTicks >> (8 * i)) & 0xFF;
    }
    
    writeSPP(timeArray, 9);
}


int CDriverShimmerGSR::calculateTwosComplement(int signedData, int bitLength)
{
    int newData = signedData;
    if (signedData >= (1 << (bitLength - 1))) {
        newData = -((signedData ^ (int)(pow(2, bitLength) - 1)) + 1);
    }

    return newData;
}

void CDriverShimmerGSR::parseData()
{
    int iData = 0;

    for (int i = 0; i < m_numberOfChannels + 1; i++) {
        if (m_signalDataTypeArray[i] == "u8") {
            m_parsedData[i] = (int)m_dataArray[iData];
            iData = iData + 1;
        }
        else if (m_signalDataTypeArray[i] == "i8") {
            m_parsedData[i] = calculateTwosComplement((int)((int)0xFF & m_dataArray[iData]), 8);
            iData = iData + 1;
        }
        else if (m_signalDataTypeArray[i] == "u12") {
            m_parsedData[i] = (int)((int)(m_dataArray[iData] & 0xFF) + ((int)(m_dataArray[iData + 1] & 0xFF) << 8));
            iData = iData + 2;
        }
        else if (m_signalDataTypeArray[i] == "i12>") {
            m_parsedData[i] = calculateTwosComplement((int)((int)(m_dataArray[iData] & 0xFF) + ((int)(m_dataArray[iData + 1] & 0xFF) << 8)), 16);
            m_parsedData[i] = m_parsedData[i] >> 4;
            iData = iData + 2;
        }
        else if (m_signalDataTypeArray[i] == "u16") {
            m_parsedData[i] = (int)((int)(m_dataArray[iData] & 0xFF) + ((int)(m_dataArray[iData + 1] & 0xFF) << 8));
            iData = iData + 2;
        }
        else if (m_signalDataTypeArray[i] == "u16r") {
            m_parsedData[i] = (int)((int)(m_dataArray[iData + 1] & 0xFF) + ((int)(m_dataArray[iData + 0] & 0xFF) << 8));
            iData = iData + 2;
        }
        else if (m_signalDataTypeArray[i] == "i16") {
            m_parsedData[i] = calculateTwosComplement((int)((int)(m_dataArray[iData] & 0xFF) + ((int)(m_dataArray[iData + 1] & 0xFF) << 8)), 16);
            iData = iData + 2;
        }
        else if (m_signalDataTypeArray[i] == "i16*") {
            m_parsedData[i] = calculateTwosComplement((int)((int)(m_dataArray[iData + 1] & 0xFF) + ((int)(m_dataArray[iData] & 0xFF) << 8)), 16);
            iData = iData + 2;
        }
        else if (m_signalDataTypeArray[i] == "i16r") {
            m_parsedData[i] = calculateTwosComplement((int)((int)(m_dataArray[iData + 1] & 0xFF) + ((int)(m_dataArray[iData] & 0xFF) << 8)), 16);
            iData = iData + 2;
        }
        else if (m_signalDataTypeArray[i] == "u24") {
            long xmsb = ((long)(m_dataArray[iData + 2] & 0xFF) << 16);
            long msb = ((long)(m_dataArray[iData + 1] & 0xFF) << 8);
            long lsb = ((long)(m_dataArray[iData + 0] & 0xFF));
            m_parsedData[i] = xmsb + msb + lsb;
            iData = iData + 3;
        }
        else if (m_signalDataTypeArray[i] == "u24r") {
            long xmsb = ((long)(m_dataArray[iData + 0] & 0xFF) << 16);
            long msb = ((long)(m_dataArray[iData + 1] & 0xFF) << 8);
            long lsb = ((long)(m_dataArray[iData + 2] & 0xFF));
            m_parsedData[i] = xmsb + msb + lsb;
            iData = iData + 3;
        }
        else if (m_signalDataTypeArray[i] == "i24r") {
            long xmsb = ((long)(m_dataArray[iData + 0] & 0xFF) << 16);
            long msb = ((long)(m_dataArray[iData + 1] & 0xFF) << 8);
            long lsb = ((long)(m_dataArray[iData + 2] & 0xFF));
            m_parsedData[i] = xmsb + msb + lsb;
            m_parsedData[i] = calculateTwosComplement((int)m_parsedData[i], 24);
            iData = iData + 3;
        }       
    }
    // Once the data is parsed, reset m_dataArray as it won't be used until the next loop() call
    for (int i = 0; i < m_packetSize; i++) {
        m_dataArray[i] = 0;
    }
}

double calibrateU12AdcValue(double uncalibratedData, double offset, double vRefP, double gain)
{
    double calibratedData = (uncalibratedData - offset) * (((vRefP * 1000.0) / gain) / 4095.0);
    return calibratedData;
}

double calibrateMspAdcChannel(double unCalData)
{
    double offset = 0; double vRefP = 3; double gain = 1;
    double calData = calibrateU12AdcValue(unCalData, offset, vRefP, gain);
    return calData;
}

double calibrateGsrDataToResistanceFromAmplifierEq(double gsrUncalibratedData, int range)
{
    double SHIMMER3_GSR_REF_RESISTORS_KOHMS[] = { 40.2, 287.0, 1000.0, 3300.0 }; // magic numbers from shimmer api
    double rFeedback = SHIMMER3_GSR_REF_RESISTORS_KOHMS[range];
    double volts = calibrateMspAdcChannel(gsrUncalibratedData) / 1000.0;
    double rSource = rFeedback / ((volts / 0.5) - 1.0);
    return rSource;
}

void calibrateInertialSensorData(std::array<long, 3> rawData, Eigen::Matrix3d AM, Eigen::Matrix3d SM, Eigen::Vector3d OV, std::array<double, 3> &result)
{
    Eigen::Vector3d data2d;
    data2d << rawData[0], rawData[1], rawData[2];

    Eigen::Vector3d res = (AM.inverse() * SM.inverse()) * (data2d - OV);

    result[0] = res[0];
    result[1] = res[1];
    result[2] = res[2];
}

Eigen::Vector3d calibrateInertialSensorData(Eigen::Vector3d rawData, Eigen::Matrix3d AM, Eigen::Matrix3d SM, Eigen::Vector3d OV)
{
    return (AM.inverse() * SM.inverse()) * (rawData - OV);
}

Eigen::Vector3d calibrateInertialSensorDataLNAccel(Eigen::Vector3d rawLNAccel)
{
    Eigen::Matrix3d alignmentMatrixAccel;
    alignmentMatrixAccel << 
        0, -1, 0,
        -1, 0, 0,
        0, 0, -1;

    Eigen::Matrix3d sensitivityMatrixAccel;
    sensitivityMatrixAccel <<
        83, 0, 0,
        0, 83, 0,
        0, 0, 83;

    Eigen::Vector3d offsetVectorAccel(2047, 2047, 2047);
    
    return calibrateInertialSensorData(rawLNAccel, alignmentMatrixAccel, sensitivityMatrixAccel, offsetVectorAccel);
}

Eigen::Vector3d calibrateInertialSensorDataGyro(Eigen::Vector3d rawGyro)
{
    Eigen::Matrix3d alignmentMatrixGyro;
    alignmentMatrixGyro <<
        0, -1, 0,
        -1, 0, 0,
        0, 0, -1;

    Eigen::Matrix3d sensitivityMatrixGyro;
    sensitivityMatrixGyro <<
        65.5, 0, 0,
        0, 65.5, 0,
        0, 0, 65.5;

    Eigen::Vector3d offsetVectorGyro( 0, 0, 0 );

    return calibrateInertialSensorData(rawGyro, alignmentMatrixGyro, sensitivityMatrixGyro, offsetVectorGyro);
}

void CDriverShimmerGSR::convertToRealUnits()
{
    for (int i = 0; i < m_numberOfChannels + 1; i++) {
        if (m_signalNameArray[i] == INTERNAL_ADC_A13) {
            long rawA13 = m_parsedData[i];
            float mVoltA13 = (calibrateU12AdcValue((double)rawA13, 0.0, 3.0, 1.0)); //magic numbers from shimmer api
            m_realUnitsData[i] = mVoltA13;
        }
        else if( m_signalNameArray[i] == GSR) {
            long rawGSR = m_parsedData[i];
            int GSRRange = 4;
            int newGSRRange = -1;
            float kOhmGSR = 0;

            if (GSRRange == 4) {
                newGSRRange = (49152 & (int)rawGSR) >> 14;
            }
            rawGSR = (double)((int)rawGSR & 4095);
            if (GSRRange == 0 || newGSRRange == 0) {
                kOhmGSR = calibrateGsrDataToResistanceFromAmplifierEq(rawGSR, 0);
            }
            else if (GSRRange == 1 || newGSRRange == 1) {

                kOhmGSR = calibrateGsrDataToResistanceFromAmplifierEq(rawGSR, 1);
            }
            else if (GSRRange == 2 || newGSRRange == 2) {

                kOhmGSR = calibrateGsrDataToResistanceFromAmplifierEq(rawGSR, 2);
            }
            else if (GSRRange == 3 || newGSRRange == 3) {

                if (rawGSR < 683) {
                    rawGSR = 683;
                }
                kOhmGSR = calibrateGsrDataToResistanceFromAmplifierEq(rawGSR, 3);
            }
            m_realUnitsData[i] = kOhmGSR;
        }
        else if (m_signalNameArray[i] == LOW_NOISE_ACCELEROMETER_X) {
            Eigen::Vector3d rawLNAccel(m_parsedData[i], m_parsedData[i + 1], m_parsedData[i + 2]);

            //mps2 = meters per second squared
            Eigen::Vector3d mps2LNAccel = calibrateInertialSensorDataLNAccel(rawLNAccel);

            m_realUnitsData[i] = mps2LNAccel[0];
            m_realUnitsData[i+1] = mps2LNAccel[1];
            m_realUnitsData[i+2] = mps2LNAccel[2];
        }
        else if (m_signalNameArray[i] == GYROSCOPE_X) {
            Eigen::Vector3d rawGyro(m_parsedData[i], m_parsedData[i + 1], m_parsedData[i + 2]);

            //dps = degrees per second
            Eigen::Vector3d dpsGyro = calibrateInertialSensorDataGyro(rawGyro);

            m_realUnitsData[i] = dpsGyro[0];
            m_realUnitsData[i + 1] = dpsGyro[1];
            m_realUnitsData[i + 2] = dpsGyro[2];
        }
        m_parsedData[i] = 0; // reset this value for the next loop() call
    }
}

void CDriverShimmerGSR::processDataPacket()
{
    parseData();
    convertToRealUnits();
}

}  //namespace AcquisitionServer
}  //namespace OpenViBE

#endif  // TARGET_OS_Windows
