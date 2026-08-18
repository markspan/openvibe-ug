///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDriverShimmerGSR.hpp
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


#pragma once

#if defined TARGET_OS_Windows

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include "ovasCDefinesShimmerGSR.hpp"
#include "array"

namespace OpenViBE {
namespace AcquisitionServer {
class CDriverShimmerGSR : public IDriver
{
public:
    CDriverShimmerGSR(IDriverContext& ctx);
    virtual ~CDriverShimmerGSR();
    virtual const char* getName();

    virtual bool isConfigurable();
    virtual bool configure();

    virtual bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback);
    virtual bool uninitialize();

    virtual bool start();
    virtual bool stop();
    virtual bool loop();

    virtual const IHeader* getHeader() { return &m_header; }

protected:
    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    std::vector<uint32_t> m_serialPorts;

    size_t m_nSamplePerSentBlock = 0;
    std::vector<float> m_sample;
    size_t m_sampleSize = 0;

private:
    // Detection of the active bluetooth ports
    void detectDevices();
    LSTATUS registryRead(LPCTSTR subkey, DWORD type, DWORD index);

    // Port selection and connection to device
    HANDLE m_handleSerial;

    uint32_t m_portIndex = 0;
    uint32_t m_port = 0;
    bool m_portSelected = false;

    int initSPP(const int port);
    void closeSPP();

    // Communication with device
    unsigned long writeSPP(unsigned char* buffer, const int nbBytes);
    unsigned long readSPP(unsigned char* buffer, const int nbBytes);

    int m_accelRange;
    int m_magGain;
    int m_gyroRange;
    int m_accelSamplingRate;
    int m_internalExpPower = -1;
    int m_myid;
    int m_nshimmer;
    int m_baudRate;

    std::string m_centerName;
    std::string m_shimmerName;
    std::string m_expId;
    std::string m_configTime;

    double m_samplingFrequency = 0;
    double m_samplingRate;

    long long shimmerRealWorldClock;

    int m_enabledSensors;
    std::vector<std::string> m_signalNameArray;
    std::vector<std::string> m_signalDataTypeArray;
    int m_numberOfChannels;
    int m_packetSize;

    std::vector<unsigned char> m_dataArray;
    std::vector<long> m_parsedData;
    std::vector<float> m_realUnitsData;

    int initializeDevice();

    void sendCommandToDevice(unsigned char command);

    // get from device, then set the corresponding attribute
    int getFromDevice(unsigned char command);
    int getFromDeviceStr(unsigned char command);
    int setAndGetSamplingRate();
    
    int inquiry();
    void interpretInquiryResponse(std::vector<unsigned char> channelIds);

    void initializeShimmerTime();

    int calculateTwosComplement(int signedDate, int bitLength);
    void parseData();
    void convertToRealUnits();
    void processDataPacket();

};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif  // TARGET_OS_Windows
