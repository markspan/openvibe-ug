///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDriverBrainProductsBrainampSeries.cpp
/// \brief Brain Products Brainamp Series driver for OpenViBE
/// \author Yann Renard
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

#include "ovasCDriverBrainProductsBrainampSeries.h"

#if defined TARGET_OS_Windows

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include <list>

#include <windows.h>
#include <WinIoCtl.h>

namespace OpenViBE {
namespace AcquisitionServer {

char* CDriverBrainProductsBrainampSeries::getErrorMessage(const uint32_t error)
{
    const uint32_t code = error & 0xffff;
    switch (code) {
        case 0: break;
        case 1: return
                    "Connection between Brainamp and USB 2 Adapter / PCI is broken.\n"
                    "Please check connectors, switch and battery power. After the\n"
                    "connection is established and if you wish to continue the\n"
                    "recording, please press the Start/Resume Recording button.\n"
                    "If problem still persists, contact us at techsup@brainproducts.com";
        case 2: return
                    "The voltage in the amplifier is too low!\n"
                    "Check the batteries!";
        case 3: return
                    "Could not establish communication with the amplifier.\n"
                    "Check the connectors and the battery power!";
        case 4: return
                    "Out of synch, Barker words missing!";
        case 5: return
                    "Connection between USB 2 Adapter and Computer is broken.\n"
                    "Monitoring or recording was interrupted. Please check\n"
                    "the USB connectors. If problem still persists, contact\n"
                    "us at techsup@brainproducts.com";
        default: return "Unknown Amplifier Error\n";
    }
    return "";
}

const Kernel::ELogLevel LogLevel_TraceAPI = Kernel::LogLevel_None;

//___________________________________________________________________//
//                                                                   //

CDriverBrainProductsBrainampSeries::CDriverBrainProductsBrainampSeries(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_BrainAmpSeries", m_driverCtx.getConfigurationManager()), m_headerAdapter(m_header, m_channelSelected)
{
    // char* channelName[]={"Fp1", "Fp2", "F3", "F4", "C3", "C4", "P3", "P4", "O1", "O2", "F7", "F8", "T7", "T8", "P7", "P8", "Fz", "Cz", "Pz", "FC1", "FC2", "CP1", "CP2", "FC5", "FC6", "CP5", "CP6", "TP9", "TP10", "Eog", "Ekg1", "Ekg2", };
    //char* channelNameActiCap32 [] = { "Fp1", "Fp2", "F7", "F3", "Fz", "F4", "F8", "FC5", "FC1", "FC2", "FC6", "T7", "C3", "Cz", "C4", "T8", "TP9", "CP5", "CP1", "CP2", "CP6", "TP10", "P7", "P3", "Pz", "P4", "P8", "PO9", "O1", "Oz", "O2", "PO10" };
    //char* channelNameActiCap64 [] = {
    //  "Fp1", "Fp2", "F7", "F3", "Fz", "F4", "F8", "FC5", "FC1", "FC2", "FC6", "T7", "C3", "Cz", "C4", "T8", "TP9", "CP5", "CP1", "CP2", "CP6", "TP10", "P7", "P3", "Pz", "P4", "P8", "PO9", "O1", "Oz", "O2", "PO10",
    //  "AF7", "AF3", "AF4", "AF8", "F5", "F1", "F2", "F6", "FT9", "FT7", "FC3", "FC4", "FT8", "FT10", "C5", "C1", "C2", "C6", "TP7", "CP3", "CPz", "CP4", "TP8", "P5", "P1", "P2", "P6", "PO7", "PO3", "POz", "PO4", "PO8",
    //};
    char* channelNameActiCap128[] = {
        "Fp1", "Fp2", "F7", "F3", "Fz", "F4", "F8", "FC5", "FC1", "FC2", "FC6", "T7", "C3", "Cz", "C4", "T8", "TP9", "CP5", "CP1", "CP2", "CP6", "TP10", "P7",
        "P3", "Pz", "P4", "P8", "PO9", "O1", "Oz", "O2", "PO10",
        "AF7", "AF3", "AF4", "AF8", "F5", "F1", "F2", "F6", "FT9", "FT7", "FC3", "FC4", "FT8", "FT10", "C5", "C1", "C2", "C6", "TP7", "CP3", "CPz", "CP4",
        "TP8", "P5", "P1", "P2", "P6", "PO7", "PO3", "POz", "PO4", "PO8",
        "FPz", "F9", "AFF5h", "AFF1h", "AFF2h", "AFF6h", "F10", "FTT9h", "FTT7h", "FCC5h", "FCC3h", "FCC1h", "FCC2h", "FCC4h", "FCC6h", "FTT8h", "FTT10h",
        "TPP9h", "TPP7h", "CPP5h", "CPP3h", "CPP1h", "CPP2h", "CPP4h", "CPP6h", "TPP8h", "TPP10h", "POO9h", "POO1", "POO2", "POO10h", "Iz",
        "AFp1", "AFp2", "FFT9h", "FFT7h", "FFC5h", "FFC3h", "FFC1h", "FFC2h", "FFC4h", "FFC6h", "FFT8h", "FFT10h", "TTP7h", "CCP5h", "CCP3h", "CCP1h", "CCP2h",
        "CCP4h", "CCP6h", "TTP8h", "P9", "PPO9h", "PPO5h", "PPO1h", "PPO2h", "PPO6h", "PPO10h", "P10", "I1", "OI1h", "OI2h", "I2",
    };
    char** channelName = channelNameActiCap128;

    uint32_t nAmplifier = 1;
    this->getDeviceDetails(m_usbIdx, &nAmplifier, nullptr);

    m_header.setSamplingFrequency(500);
    m_header.setChannelCount(nAmplifier * 32);
    for (uint32_t i = 0; i < 256; ++i) {
        m_header.setChannelName(i, (i < sizeof(channelNameActiCap128) / sizeof(void*) ? channelName[i] : ""));
        m_channelSelected[i]   = Channel_Selected;
        m_lowPassFilterFull[i] = Parameter_Default;
        m_resolutionFull[i]    = Parameter_Default;
        m_dcCouplingFull[i]    = Parameter_Default;
    }

    for (uint32_t i = 0; i < 256; ++i) { m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); }

    m_settings.add("Header", &m_header);
    m_settings.add("USBIndex", &m_usbIdx);
    m_settings.add("DecimationFactor", &m_decimationFactor);
    m_settings.add("ChannelSelected", m_channelSelected);
    m_settings.add("LowPassFilterFull", &m_lowPassFilterFull);
    m_settings.add("ResolutionFull", &m_resolutionFull);
    m_settings.add("DCCouplingFull", &m_dcCouplingFull);
    m_settings.add("LowPass", &m_lowPass);
    m_settings.add("Resolution", &m_resolution);
    m_settings.add("DCCoupling", &m_dcCoupling);
    m_settings.add("Impedance", &m_impedance);
    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainProductsBrainampSeries::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    // #define nSamplePerSentBlock 20*5


    if (m_driverCtx.isConnected()) { return false; }

    // --
    const std::string name = "\\\\.\\BrainAmpUSB" + std::to_string(m_usbIdx);
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Preparing device [" << name << "]\n";

    // -- Gets amplifiers type

    uint32_t nAmplifier = BrainAmp_MaximumAmplifierCount;
    uint32_t types[BrainAmp_MaximumAmplifierCount];
    if (!this->getDeviceDetails(m_usbIdx, &nAmplifier, types)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not get amplifier(s) type - got error " << size_t(GetLastError()) << "\n";
        return false;
    }

    for (size_t i = 0; i < BrainAmp_MaximumAmplifierCount; ++i) {
        CString type(getDeviceType(types[i]));
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << " - Amplifier " << i + 1 << " : " << type << "\n";
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Found " << nAmplifier << " amplifier(s)\n";

    // -- Opens device

    m_Device = ::CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (m_Device == INVALID_HANDLE_VALUE) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open device [" << name << "]\n";
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Opened device [" << name << "]\n";

    // -- Gets driver version from device

    char driverVersion[1024];
    uint32_t version = 0;
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_DRIVERVERSION\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_DRIVERVERSION, nullptr, 0, &version, sizeof(version), &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not get driver version from device - got error " << size_t(GetLastError()) << "\n";
        CloseHandle(m_Device);
        return false;
    }
    const uint32_t versionPatch = version % 10000;
    const uint32_t versionMinor = (version % 1000000) / 10000;
    const uint32_t versionMajor = (version) / 1000000;
    sprintf(driverVersion, "%i.%02i.%04i", versionMajor, versionMinor, versionPatch);
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Found driver version [" << CString(driverVersion) << "]\n";

    // -- Gets serial number from device

    char deviceSerialNumber[1024];
    uint32_t serialNumber = 0;
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_GET_SERIALNUMBER\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_GET_SERIALNUMBER, nullptr, 0, &serialNumber, sizeof(serialNumber), &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not get serial number from device - got error " << size_t(GetLastError()) << "\n";
        CloseHandle(m_Device);
        return false;
    }
    if (m_nBytesReturned != 0) {
        sprintf(deviceSerialNumber, "0x%08x", serialNumber);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Found serial number [" << CString(deviceSerialNumber) << "]\n";
    }

    // -- Sets callibration settings

    m_deviceCalibrationSettings = new CBrainampCalibrationSettings;
    // m_deviceCalibrationSettings->waveForm=WaveForm_Ramp;
    // m_deviceCalibrationSettings->waveForm=WaveForm_Triangle;
    // m_deviceCalibrationSettings->waveForm=WaveForm_Square;
    m_deviceCalibrationSettings->waveForm  = WaveForm_SineWave;
    m_deviceCalibrationSettings->frequency = 5;

    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_CALIBRATION_SETTINGS\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_CALIBRATION_SETTINGS, m_deviceCalibrationSettings,
                         sizeof(CBrainampCalibrationSettings), nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not set calibration settings - got error " << size_t(GetLastError()) << "\n";
        CloseHandle(m_Device);
        return false;
    }

    // -- Configures device according to what has been enterd in the configuration dialog

    bool lowPassWarning = false;
    float resolution[]  = { 0.1F, 0.5F, 10.0F, 152.6F };

    m_deviceSetup = new CBrainampSetup;
    memset(m_deviceSetup, 0, sizeof(CBrainampSetup));
    m_deviceSetup->nChannel            = m_headerAdapter.getChannelCount();
    m_deviceSetup->holdValue           = 0x0; // Value without trigger
    m_deviceSetup->nSamplePerSentBlock = nSamplePerSentBlock * m_decimationFactor;
    m_deviceSetup->lowImpedance        = uint8_t(m_impedance);
    for (size_t j = 0, i = 0; i < m_header.getChannelCount(); ++i) {
        if (m_channelSelected[i] == Channel_Selected) {
            // Should I care about j being greater than 
            m_deviceSetup->channelLookup[j] = i;
            m_deviceSetup->lowPassFilter[j] = uint8_t(m_lowPassFilterFull[i] == Parameter_Default ? m_lowPass : m_lowPassFilterFull[i]); // 0 - 1000Hz, 1 - 250Hz
            m_deviceSetup->resolution[j] = uint8_t(m_resolutionFull[i] == Parameter_Default ? m_resolution : m_resolutionFull[i]); // 0 - 100 nV, 1 - 500 nV, 2 - 10 uV, 3 - 152.6 uV
            m_deviceSetup->dcCoupling[j] = uint8_t(m_dcCouplingFull[i] == Parameter_Default ? m_dcCoupling : m_dcCouplingFull[i]); // 0 - AC, 1 - DC

            m_resolutionFactor[j] = resolution[m_deviceSetup->resolution[j]];

            if (!lowPassWarning && m_decimationFactor != 1 && (m_lowPass == LowPass_1000 || m_lowPassFilterFull[i] == LowPass_1000)) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Using 1000 Hz low pass filter...\n";
                m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Using a decimation factor of " << m_decimationFactor << " (resulting in " <<
                        5000 / m_decimationFactor << " Hz sampling rate)\n";
                m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "This is probably not safe. Signal quality will suffer.\n";
                lowPassWarning = true;
            }

            j++;
        }
    }

    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_SETUP\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_SETUP, m_deviceSetup, sizeof(CBrainampSetup), nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not send setup parameters to device - got error " << size_t(GetLastError()) << "\n";
        CloseHandle(m_Device);
        return false;
    }

    // -- Allocates sample array and intermediate buffer

    m_sample = new float[m_headerAdapter.getChannelCount() * nSamplePerSentBlock];
    m_buffer = new int16_t[(m_headerAdapter.getChannelCount() + 1) * nSamplePerSentBlock * m_decimationFactor];
    if (!m_sample || !m_buffer) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not allocate memory for sample array / intermediate buffer\n";
        delete [] m_sample;
        delete [] m_buffer;
        m_sample = nullptr;
        m_buffer = nullptr;
        CloseHandle(m_Device);
        return false;
    }

    // -- Everything is up and ready to work

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    m_marker              = 0;

    // -- Optionally starts impedance check

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Impedance " << (m_driverCtx.isImpedanceCheckRequested() ? "will" : "won't") << " be checked\n";
    if (m_driverCtx.isImpedanceCheckRequested()) { if (!this->startImpedanceCheck()) { return false; } }

    return true;
}

bool CDriverBrainProductsBrainampSeries::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // -- Optionally stops impedance check
    if (m_driverCtx.isImpedanceCheckRequested()) { if (!this->stopImpedanceCheck()) { return false; } }

    // -- Configures pull up/down for triggers

    uint16_t pullUp = 0;
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_DIGITALINPUT_PULL_UP\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_DIGITALINPUT_PULL_UP, &pullUp, sizeof(pullUp), nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not switch pull up\n";
        CloseHandle(m_Device);
        return false;
    }

    // -- Starts acquisition

    uint32_t type = uint32_t(AcquisitionType_EEG);
    // uint32_t type=uint32_t(AcquisitionType_TestSignal);
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_START\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_START, &type, sizeof(uint32_t), nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not start acquisition\n";
        return false;
    }

    return true;
}

bool CDriverBrainProductsBrainampSeries::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted() && !m_driverCtx.isImpedanceCheckRequested()) { return true; }


    // -- Gets error code from device

    uint32_t error = 0;
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_ERROR_STATE\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_ERROR_STATE, nullptr, 0, &error, sizeof(error), &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not retrieve error state\n";
        return false;
    }

    // -- Checks error code

    if (error != 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Received error state " << error << " from device\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Full error text :\n" << this->getErrorMessage(error) << "\n";
        return false;
    }

    // -- Loops until the buffer is read

    do {
        // -- Reads intermediate buffer from device

        if (!ReadFile(m_Device, m_buffer, (m_headerAdapter.getChannelCount() + 1) * m_nSamplePerSentBlock * m_decimationFactor * sizeof(int16_t), &m_nBytesReturned, nullptr)) {
            const uint32_t lastError = GetLastError();
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not read from device - Got error " << lastError << " "
                    << (lastError == ERROR_MORE_DATA ? "(buffer overflow)" : "") << "\n";
            return true;
        }

        if (m_nBytesReturned == 0) { System::Time::sleep(2); }
    } while (m_nBytesReturned == 0);

#if 0
    // -- Gets buffer filling state

    static uint32_t lastBufferFillingState=0;
    static uint32_t bufferFillingState=0;
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_BUFFERFILLING_STATE\n";
    if (!::DeviceIoControl(m_Device, BRAINAMP_BUFFERFILLING_STATE, nullptr, 0, &bufferFillingState, sizeof(bufferFillingState), &m_nBytesReturned, nullptr))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not retrieve buffer filling state\n";
        return false;
    }
    if(bufferFillingState!=lastBufferFillingState)
    {
        lastBufferFillingState=bufferFillingState;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Buffer filling state : " << bufferFillingState << "\n";
    }
#endif

    if (!m_driverCtx.isStarted()) {
        // -- Converts the intermediate buffer in instant impedances
        const double scale = 1000 * (m_impedance == Impedance_High ? 2.1e-3 : 2.13219e-4);
        for (size_t i = 0; i < m_headerAdapter.getChannelCount(); ++i) {
            int16_t minValue = 32767;
            int16_t maxValue = -32767;
            int16_t* buffer  = m_buffer + i;
            for (size_t j = 0; j < m_nSamplePerSentBlock * m_decimationFactor; ++j) {
                if (*buffer > maxValue) { maxValue = *buffer; }
                if (*buffer < minValue) { minValue = *buffer; }
                buffer += m_headerAdapter.getChannelCount() + 1;
            }
            m_impedances[i] = (maxValue - minValue) * scale - 1. + .5;
        }

        // -- Backups this impedance in the impedance buffer and limit this buffer size

        m_impedanceBuffers.push_back(m_impedances);
        if (m_impedanceBuffers.size() > m_impedanceCheckSignalFrequency) { m_impedanceBuffers.pop_front(); }

        // -- Averages the impedances over last measures and send them to the acquisition server

        for (size_t i = 0; i < m_headerAdapter.getChannelCount(); ++i) {
            double average = 0;
            for (auto it = m_impedanceBuffers.begin(); it != m_impedanceBuffers.end(); ++it) { average += (*it)[i]; }
            average /= m_impedanceBuffers.size();

            m_driverCtx.updateImpedance(i, average);
        }
    }
    else {
        // -- Converts the intermediate buffer in acquisition server convenient format

        float* samples = m_sample;
        int16_t* buffer;
        for (size_t i = 0; i < m_headerAdapter.getChannelCount(); ++i) {
            buffer = m_buffer + i;
            for (size_t j = 0; j < m_nSamplePerSentBlock; ++j) {
                *samples = float(*buffer) * m_resolutionFactor[i];

                samples += 1;
                buffer += m_decimationFactor * (m_headerAdapter.getChannelCount() + 1);
            }
        }

        CStimulationSet stimSet;
        buffer = m_buffer + m_headerAdapter.getChannelCount();
        for (size_t j = 0; j < m_nSamplePerSentBlock * m_decimationFactor; ++j) {
            uint32_t marker = *buffer;
            marker ^= m_deviceSetup->holdValue;
            if (marker != m_marker) {
                m_marker = marker;
                stimSet.push_back(OVTK_StimulationId_Label(m_marker), CTime(m_header.getSamplingFrequency() * m_decimationFactor, uint64_t(j)).time(), 0);
                m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Got stim code " << m_marker << " at sample " << j << "\n";
            }
            buffer += m_headerAdapter.getChannelCount() + 1;
        }

        // -- Sends data to the acquisition server

        m_callback->setSamples(m_sample, m_nSamplePerSentBlock);
        m_callback->setStimulationSet(stimSet);
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }

    return true;
}

bool CDriverBrainProductsBrainampSeries::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    // -- Stops acquisition

    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_STOP\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_STOP, nullptr, 0, nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not stop acquisition\n";
    }

    // -- Optionally starts impedance check

    if (m_driverCtx.isImpedanceCheckRequested()) { if (!this->startImpedanceCheck()) { return false; } }

    return true;
}

bool CDriverBrainProductsBrainampSeries::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // -- Optionally stops impedance check

    if (m_driverCtx.isImpedanceCheckRequested()) { if (!this->stopImpedanceCheck()) { return false; } }

    // -- Deletes device setup object

    delete m_deviceSetup;
    m_deviceSetup = nullptr;

    // -- Closes handle to device

    CloseHandle(m_Device);
    m_Device = nullptr;

    // -- Deletes sample array and intermediate buffer

    delete [] m_sample;
    delete [] m_buffer;
    m_sample   = nullptr;
    m_buffer   = nullptr;
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainProductsBrainampSeries::startImpedanceCheck()
{
    if (!m_driverCtx.isImpedanceCheckRequested()) { return true; }

    // Configures impedance frequency

    m_impedanceCheckSignalFrequency = 5000 / (m_nSamplePerSentBlock * m_decimationFactor);
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Imepdance check sampling frequency set to " << m_impedanceCheckSignalFrequency << " Hz\n";

    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_IMPEDANCE_FREQUENCY\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_IMPEDANCE_FREQUENCY, &m_impedanceCheckSignalFrequency, sizeof(uint32_t), nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not set impedance frequency - got error " << size_t(GetLastError()) << "\n";
        return false;
    }

    // Starts acquiring impedance signal
    uint32_t type = uint32_t(AcquisitionType_Impedance);
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_START\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_START, &type, sizeof(uint32_t), nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not start acquisition\n";
        return false;
    }

    // Configures impedance group range
    uint32_t group = (m_impedance == Impedance_High ? 0 : 1);
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_IMPEDANCE_GROUPRANGE\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_IMPEDANCE_GROUPRANGE, &group, sizeof(uint32_t), nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could set impedance group range\n";
        return false;
    }

    // Prepares impedance buffers

    m_impedances.resize(m_headerAdapter.getChannelCount());
    m_impedanceBuffers.clear();

    return true;
}

bool CDriverBrainProductsBrainampSeries::stopImpedanceCheck()
{
    if (!m_driverCtx.isImpedanceCheckRequested()) { return true; }

    // Stops impedance acquisition
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_STOP\n";
    if (!DeviceIoControl(m_Device, BRAINAMP_STOP, nullptr, 0, nullptr, 0, &m_nBytesReturned, nullptr)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not stop acquisition\n";
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainProductsBrainampSeries::configure()
{
    CConfigurationBrainProductsBrainampSeries config(
        *this, Directories::getDataDir() + "/applications/acquisition-server/interface-BrainProducts-BrainampSeries.ui", m_usbIdx, m_decimationFactor,
        m_channelSelected, m_lowPassFilterFull, m_resolutionFull, m_dcCouplingFull, m_lowPass, m_resolution, m_dcCoupling, m_impedance);

    config.configure(m_header);

    m_settings.save();

    m_header.setSamplingFrequency(5000 / m_decimationFactor);
    return true;
}

bool CDriverBrainProductsBrainampSeries::getDeviceDetails(const uint32_t index, uint32_t* nAmplifier, uint32_t* amplifierType)
{
    const std::string name = ("\\\\.\\BrainAmpUSB" + std::to_string(index));
    // -- Opens device
    void* device = ::CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (device == INVALID_HANDLE_VALUE) { return false; }

    // -- Gets amplifiers type
    uint16_t type[BrainAmp_MaximumAmplifierCount];
    m_driverCtx.getLogManager() << LogLevel_TraceAPI << "BRAINAMP_AMPLIFIER_TYPE\n";
    if (!DeviceIoControl(device, BRAINAMP_AMPLIFIER_TYPE, nullptr, 0, type, sizeof(type), &m_nBytesReturned, nullptr)) {
        CloseHandle(device);
        return false;
    }

    // -- Closes device
    CloseHandle(device);

    // -- Formats result
    size_t count = BrainAmp_MaximumAmplifierCount;
    for (size_t i = 0; i < BrainAmp_MaximumAmplifierCount; ++i) {
        if (amplifierType) { amplifierType[i] = type[i]; }
        if (count == BrainAmp_MaximumAmplifierCount && type[i] == AmplifierType_None) { count = i; }
    }
    if (nAmplifier) { *nAmplifier = count; }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_OS_Windows
