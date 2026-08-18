/**
 *  Software License Agreement (AGPL-3 License)
 *
 * \file CDriverGTecUnicorn.cpp
 * \author Anton Andreev, Gipsa-lab, VIBS team
 * \date 21/08/2020
 * \brief Implementation of GTEC Unicorn Black Driver
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#if defined TARGET_HAS_ThirdPartyGtecUnicron

#include "CDriverGTecUnicorn.hpp"

#include <toolkit/ovtk_all.h>
#include <system/ovCTime.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <functional>

#include <mutex>
#include <thread>

#include "unicorn.h"

namespace OpenViBE {
namespace AcquisitionServer {

#if defined(TARGET_OS_Windows)
#pragma warning(disable: 4800) // disable "forcing value to bool 'true' or 'false' (performance warning)" nag coming from BOOL->bool cast on e.g. VS2010
#endif

CDriverGTecUnicorn::CDriverGTecUnicorn(IDriverContext& rDriverContext)
    : IDriver(rDriverContext), m_settings("AcquisitionServer_Driver_GTecGUSBamp", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(UNICORN_SAMPLING_RATE);
    m_header.setChannelCount(0);

    //m_settings.load();
}
//___________________________________________________________________//
//                                                                   //

bool CDriverGTecUnicorn::initialize(
    const uint32_t sampleCountPerSentBlock,
    IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    detectDevices();

    if (numDevices() == 0) { return false; }

    m_sampleCountPerSentBlock = sampleCountPerSentBlock;
    m_callback                = &callback;

    // Set number of channels
    int errorCode = UNICORN_GetNumberOfAcquiredChannels(m_devices[kSelectedDevice].handle, &m_acquiredChannelCount);
    if (errorCode != UNICORN_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to get channel count.\n";
        return false;
    }
    else { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Number of available channels is: " << m_acquiredChannelCount << "\n"; }

    m_header.setChannelCount(m_acquiredChannelCount); //only 8 channels are EEG, but currently we provide everything
    m_lengthBufferRawUnicornDevice = m_acquiredChannelCount * kFrameLength;

    /*
     * Set channel names
     * 17 channels from Unicorn Black on every sample: | EEG1| EEG2| EEG3| EEG4| EEG5| EEG6| EEG7| EEG8| ACCX|ACCY| ACCZ| GYRX|GYRY| GYRZ|CNT|BATLVL|VALID|
     * Order might be different. This is why we need to use UNICORN_GetChannelIndex(channel name);
     */
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "EEG 1"), "EEG1");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "EEG 2"), "EEG2");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "EEG 3"), "EEG3");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "EEG 4"), "EEG4");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "EEG 5"), "EEG5");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "EEG 6"), "EEG6");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "EEG 7"), "EEG7");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "EEG 8"), "EEG8");

    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "Accelerometer X"), "ACCX");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "Accelerometer Y"), "ACCY");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "Accelerometer Z"), "ACCZ");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "Gyroscope X"), "GYRX");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "Gyroscope Y"), "GYRY");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "Gyroscope Z"), "GYRZ");
    m_channelCounterIndex = getChannelIndex(m_devices[kSelectedDevice].handle, "Counter");
    m_header.setChannelName(m_channelCounterIndex, "Counter");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "Battery Level"), "Battery");
    m_header.setChannelName(getChannelIndex(m_devices[kSelectedDevice].handle, "Validation Indicator"), "VI");

    // Initialize buffers
    m_bufferReceivedDataFromRing = new float[m_lengthBufferRawUnicornDevice];
    m_bufferForOpenVibe          = new float[m_lengthBufferRawUnicornDevice];

    m_ringBuffer.Initialize(static_cast<uint32_t>(kBufferSizeSeconds * m_header.getSamplingFrequency() * m_lengthBufferRawUnicornDevice));

    // Configure each device
    for (size_t i = 0; i < numDevices(); i++) { configureDevice(i); }

    return true;
}

void CDriverGTecUnicorn::detectDevices()
{
    int errorCode = UNICORN_ERROR_SUCCESS;

    // Get number of available devices
    unsigned int availableDevicesCount = 0;
    errorCode                          = UNICORN_GetAvailableDevices(NULL, &availableDevicesCount, TRUE);

    // Get serials of available devices
    UNICORN_DEVICE_SERIAL* availableDevices = new UNICORN_DEVICE_SERIAL[availableDevicesCount];
    errorCode                               = UNICORN_GetAvailableDevices(availableDevices, &availableDevicesCount, TRUE);

    if (errorCode != UNICORN_ERROR_SUCCESS || availableDevicesCount < 1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "No device available. Please pair with a Unicorn device first.\n";
    }
    else
    {
        // Create a GDevice list for all devices and print available device serials
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Available Unicorn devices:\n";

        for (unsigned int i = 0; i < availableDevicesCount; i++)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "#" << i << ": " << availableDevices[i] << "\n";

            GDevice device;

            UNICORN_HANDLE deviceHandle;

            int errorCode = UNICORN_OpenDevice(availableDevices[i], &deviceHandle);
            if (errorCode != UNICORN_ERROR_SUCCESS)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to connect to device: '" << availableDevices[i] << "'\n";
            }

            device.handle = deviceHandle;

            device.serial = availableDevices[i];

            m_devices.push_back(device);
        }
    }
}

bool CDriverGTecUnicorn::configureDevice(size_t deviceNumber)
{
    int errorCode = UNICORN_ERROR_SUCCESS;

    UNICORN_HANDLE device           = m_devices[deviceNumber].handle;
    const std::string currentSerial = m_devices[deviceNumber].serial;

    UNICORN_AMPLIFIER_CONFIGURATION configuration;
    errorCode = UNICORN_GetConfiguration(device, &configuration);

    if (errorCode != UNICORN_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to get configuration for: '" << currentSerial.c_str() << "'\n";
        return false;
    }

    return true;
}

bool CDriverGTecUnicorn::start()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (m_driverCtx.isStarted()) { return false; }

    m_totalHardwareStimulations = 0;
    m_totalRingBufferOverruns   = 0;
    m_totalCounterErrors        = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_ringBuffer.Reset();
    }

    for (size_t i = 0; i < numDevices(); i++)
    {
        UNICORN_HANDLE device = m_devices[i].handle;
        UNICORN_StartAcquisition(device, kTestSignalEnabled);
    }

    m_isThreadRunning = true;
    m_flagIsFirstLoop = true;
    m_bufferOverrun   = false;

    m_thread.reset(new std::thread(std::bind(&CDriverGTecUnicorn::acquire, this)));

    return true;
}

// This method is called by the AS and it supplies the acquired data to the AS
bool CDriverGTecUnicorn::loop()
{
    if (m_driverCtx.isStarted())
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            while (m_ringBuffer.GetSize() < static_cast<int>(m_lengthBufferRawUnicornDevice)) { m_itemAvailable.wait(lock); }

            try
            {
                if (m_bufferOverrun)
                {
                    m_ringBuffer.Reset();
                    m_bufferOverrun = false;
                    m_totalRingBufferOverruns++;
                    return true;
                }

                m_ringBuffer.Read(m_bufferReceivedDataFromRing, m_lengthBufferRawUnicornDevice);
            }
            catch (std::exception& e)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error reading GTEC ring buffer! Error is:" << e.what() << "\n";
            }

            m_itemAvailable.notify_one();
        }

        // covert to openvibe format ch1,ch1,ch1,ch2,ch2,ch2 ...
        for (size_t i = 0; i < m_acquiredChannelCount; i++)
        {
            for (size_t j = 0; j < kFrameLength; j++)
            {
                m_bufferForOpenVibe[kFrameLength * i + j] = m_bufferReceivedDataFromRing[j * m_acquiredChannelCount + i];
            }
        }

        // verify counter
        const size_t counterPos = kFrameLength * m_channelCounterIndex;
        for (size_t i = counterPos; i < kFrameLength; i++) { if (!(m_bufferForOpenVibe[i] < m_bufferForOpenVibe[i + 1])) { m_totalCounterErrors++; } }

        // forward data
        m_callback->setSamples(m_bufferForOpenVibe, kFrameLength);
        CStimulationSet stimulationSet;
        m_callback->setStimulationSet(stimulationSet);
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }
    else { System::Time::sleep(20); }

    return true;
}

// This function used by the thread
bool CDriverGTecUnicorn::acquire()
{
    if (m_flagIsFirstLoop) //First time do some memory initialization, etc
    {
        m_bufferRawUnicornDevice = new float[m_lengthBufferRawUnicornDevice];

        m_flagIsFirstLoop = false;
    }

    while (m_isThreadRunning == true)
    {
        try
        {
            bool flagChunkLostDetected    = false;
            bool flagChunkTimeOutDetected = false;

            UNICORN_HANDLE device = m_devices[kSelectedDevice].handle;

            // Get kFrameLength number of samples
            if (UNICORN_GetData(device, kFrameLength, m_bufferRawUnicornDevice, m_lengthBufferRawUnicornDevice * sizeof(float)) != UNICORN_ERROR_SUCCESS)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error on GT_GetData\n";
                return false;
            }

            // store to ring buffer, lock it during insertion
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                try
                {
                    // if we are going to overrun on writing the received data into the buffer, set the appropriate flag; the reading thread will handle the overrun
                    m_bufferOverrun = (m_ringBuffer.GetFreeSize() < static_cast<int>(m_lengthBufferRawUnicornDevice));

                    m_ringBuffer.Write(m_bufferRawUnicornDevice, m_lengthBufferRawUnicornDevice);
                }
                catch (std::exception& e)
                {
                    // buffer should be unclocked automatically once the scope is left
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error writing to GTEC ring buffer! Error is: " << e.what() << "\n";
                }
                // buffer should be unclocked automatically once the scope is left

                m_itemAvailable.notify_one();
            }
        }
        catch (std::exception& e)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                    "General error in the thread function acquiring data from GTEC! Acquisition interrupted. Error is: " << e.what() << "\n";
            m_isThreadRunning = false;
            return false;
        }
    }

    // This code stops the amplifiers in the same thread:
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Stopping devices and cleaning up...\n";

        // clean up allocated resources for each device
        for (size_t i = 0; i < numDevices(); i++)
        {
            UNICORN_HANDLE device = m_devices[i].handle;

            // stop device
            m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Sending stop command ...\n";
            if (UNICORN_StopAcquisition(device) != UNICORN_ERROR_SUCCESS)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Stopping device failed! Serial = " << m_devices[numDevices() - 1].serial.c_str() << "\n";
            }

            // clear memory
            delete[] m_bufferRawUnicornDevice;
        }

        m_flagIsFirstLoop = true;
        m_isThreadRunning = false;
    }
    return true;
}

bool CDriverGTecUnicorn::stop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return false; }

    // stop thread
    m_isThreadRunning = false;
    m_thread->join(); //wait until the thread has stopped data acquisition

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Data acquisition completed.\n";

    if (m_totalRingBufferOverruns > 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Total internal ring buffer overruns: " << m_totalRingBufferOverruns << "\n";
    }
    if (m_totalCounterErrors > 0) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Total Unicorn counter errors: " << m_totalCounterErrors << "\n"; }
    else { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Total Unicorn counter errors: " << m_totalCounterErrors << "\n"; }

    return true;
}

bool CDriverGTecUnicorn::uninitialize()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (m_driverCtx.isStarted()) { return false; }

    const size_t totalDevices = numDevices();
    size_t deviceClosed       = 0;

    for (std::vector<GDevice>::iterator it = m_devices.begin(); it != m_devices.end();)
    {
        if (UNICORN_CloseDevice(&it->handle) != UNICORN_ERROR_SUCCESS)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to close device: " << it->serial.c_str() << "\n";
        }
        else { deviceClosed++; }

        it = m_devices.erase(it);
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Total devices closed: " << deviceClosed << " / " << totalDevices << "\n";

    if (!m_devices.empty() || deviceClosed != totalDevices)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Some devices were not closed properly!\n";
    }

    m_devices.clear();

    // clear memory
    if (m_bufferReceivedDataFromRing != nullptr)
    {
        delete[] m_bufferReceivedDataFromRing;
        m_bufferReceivedDataFromRing = nullptr;
    }

    if (m_bufferForOpenVibe != nullptr)
    {
        delete[] m_bufferForOpenVibe;
        m_bufferForOpenVibe = nullptr;
    }
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
inline size_t CDriverGTecUnicorn::getChannelIndex(UNICORN_HANDLE device, const char* name)
{
    uint32_t* result = new uint32_t[1];
    if (UNICORN_GetChannelIndex(device, name, result) != UNICORN_ERROR_SUCCESS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error getting channel index for channel: '" << name << "'\n";
    }
    return static_cast<size_t>(*result);
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& var)
{
    for (size_t i = 0; i < var.size(); i++) { out << var[i] << " "; }

    return out;
}

inline std::istream& operator>>(std::istream& in, std::vector<std::string>& var)
{
    var.clear();
    std::string tmp;
    while (in >> tmp) { var.push_back(tmp); }

    return in;
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<GDevice>& var)
{
    for (size_t i = 0; i < var.size(); i++) { out << var[i].serial << " "; }

    return out;
}

inline std::istream& operator>>(std::istream& in, std::vector<GDevice>& var)
{
    // "Error not implemented operator >>!";

    return in;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
