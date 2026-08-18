/**
 * SPDX-FileCopyrightText: Inria / Gipsa-lab
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the Gipsa-lab g.USBamp driver (originally by Anton Andreev, VIBS team).
 * Multi-device, queued async I/O, ring-buffer based acquisition.
 */

#include "CDriverGTecGUSBamp.h"
#include "gUSBamp.h"

#include <iostream>
#include <functional>
#include <cstring>

CDriverGTecGUSBamp::CDriverGTecGUSBamp()
    : m_commonGndAndRefBitmap("CommonGndAndRefBitmap",
                               "Common GND and REF bitmap (bits 0-3: GND1-4, bits 4-7: REF1-4)", 0, 0, 255),
      m_notchFilterIdx("NotchFilterIndex", "Notch filter index (-1 = none)", -1, -1, 1000),
      m_bandPassFilterIdx("BandPassFilterIndex", "Bandpass filter index (-1 = none)", -1, -1, 1000),
      m_triggerInputEnabled("TriggerInputEnabled", "Enable hardware trigger input channel", false),
      m_masterSerial("MasterSerial", "Master device serial number (empty = auto)", m_availableMasterSerialValues),
      m_bipolarEnabled("Bipolar", "Enable bipolar electrode mode", false),
      m_calibrationSignalEnabled("CalibrationSignal", "Enable calibration signal output", false),
      m_showDeviceName("ShowDeviceName", "Append amplifier serial to channel names", false)
{
    m_samplingRate = 512;
    m_channelCount = 16;

    m_parameters.push_back(&m_samplingRate);
    m_parameters.push_back(&m_channelCount);
    m_parameters.push_back(&m_commonGndAndRefBitmap);
    m_parameters.push_back(&m_notchFilterIdx);
    m_parameters.push_back(&m_bandPassFilterIdx);
    m_parameters.push_back(&m_triggerInputEnabled);
    m_parameters.push_back(&m_masterSerial);
    m_parameters.push_back(&m_bipolarEnabled);
    m_parameters.push_back(&m_calibrationSignalEnabled);
    m_parameters.push_back(&m_showDeviceName);

    detectDevices();
}

DWORD CDriverGTecGUSBamp::bufferSizeBytes() const
{
    return HEADER_SIZE + static_cast<DWORD>(m_chunkSize * (GTEC_NUM_CHANNELS + 1) * sizeof(float));
}

// ─────────────────────────────────────────────────────────────────────────────

bool CDriverGTecGUSBamp::init()
{
    // Todo: detect devices again, in case device was plugged in after construction (selection of the driver in UI).
    // At the moment, it fails detecting devices twice in a row, so commented.
    //detectDevices();
    if (numDevices() == 0)
    {
        std::cout << "[gUSBamp] No devices found.\n";
        return false;
    }

    // Validate stored master serial — clear if device no longer present
    bool found = false;
    for (size_t i = 0; i < numDevices(); ++i) {
        if (m_devices[i].serial == m_masterSerial.value()) { 
            found = true; break;
        }
    }
    if (!found) {
        m_masterSerial.setValue(std::string{});
    }

    // Auto-assign last device as master for multi-device setups
    if (numDevices() > 1 && m_masterSerial.value().empty()) {
        m_masterSerial.setValue(m_devices.back().serial);
    }

    if (numDevices() > 1 && m_channelCount.value() != static_cast<int>(numDevices() * GTEC_NUM_CHANNELS)) {
        std::cout << "[gUSBamp] Warning: " << m_channelCount.value() << " EEG channels selected in a "
                  << numDevices() * GTEC_NUM_CHANNELS << " channel setup.\n";
    }

    m_nAcquiredChannel = static_cast<uint32_t>(m_channelCount.value());

    // Allocate outer buffer arrays (inner arrays allocated on first acquire() call)
    m_validPoints        = static_cast<int>(m_chunkSize * (GTEC_NUM_CHANNELS + 1) * numDevices());
    m_buffers            = new BYTE**[numDevices()];
    m_overlapped         = new OVERLAPPED*[numDevices()];
    m_bufferReceivedData = new float[m_validPoints];

    m_ringBuffer.Initialize(
        static_cast<uint32_t>(BUFFER_SIZE_SECONDS * m_samplingRate.value() * (GTEC_NUM_CHANNELS + 1) * numDevices()));

    for (uint32_t i = 0; i < numDevices(); ++i)
        configureDevice(i);

    if (numDevices() > 1)
    {
        std::cout << "[gUSBamp] Multi-amplifier mode. Sync cable required between amps.\n";
        setMasterDevice(m_masterSerial.value());
    }
    else
    {
        if (GT_SetSlave(m_devices[0].handle, false))
        {
            m_masterSerial.setValue(m_devices[0].serial);
            std::cout << "[gUSBamp] Configured as MASTER: " << m_masterSerial.value() << "\n";
            m_mastersCnt++;
        }
        else { std::cout << "[gUSBamp] Error calling GT_SetSlave.\n"; }
    }

    buildChannelMap();
    updateChannelNames();

    if (m_samplingRate.value() > 4800)
        std::cout << "[gUSBamp] Warning: sampling rates >4800 Hz may need specific steps. See gUSBamp manuals.\n";

    return true;
}

bool CDriverGTecGUSBamp::unInit()
{
    for (auto it = m_devices.begin(); it != m_devices.end(); )
    {
        if (!GT_CloseDevice(&it->handle))
            std::cout << "[gUSBamp] Failed to close device: " << it->serial << "\n";
        it = m_devices.erase(it);
    }

    // Restore channel count to EEG-only so the next init() starts clean
    m_channelCount.setValue(static_cast<int>(m_nAcquiredChannel));

    delete[] m_bufferReceivedData; m_bufferReceivedData = nullptr;
    delete[] m_overlapped;         m_overlapped = nullptr;
    delete[] m_buffers;            m_buffers = nullptr;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

bool CDriverGTecGUSBamp::start()
{
    if (m_reconfigurationRequired)
    {
        for (uint32_t i = 0; i < numDevices(); ++i) configureDevice(i);
        m_reconfigurationRequired = false;
    }

    m_totalDriverChunksLost   = 0;
    m_totalDriverTimeouts     = 0;
    m_totalRingBufferOverruns = 0;

    {
        std::lock_guard<std::mutex> lock(m_io_mutex);
        m_ringBuffer.Reset();
    }

    for (uint32_t i = 0; i < numDevices(); ++i)
        GT_Start(m_devices[i].handle);

    m_isThreadRunning = true;
    m_flagIsFirstLoop = true;
    m_bufferOverrun   = false;
    m_currentQueueIdx = 0;

    m_threadPtr.reset(new std::thread([this] { acquire(); }));

    return true;
}

bool CDriverGTecGUSBamp::stop()
{
    m_isThreadRunning = false;
    m_itemAvailable.notify_all(); // unblock read() if it is waiting

    if (m_threadPtr && m_threadPtr->joinable())
        m_threadPtr->join();

    std::cout << "[gUSBamp] Acquisition complete."
              << " Chunks lost: "          << m_totalDriverChunksLost
              << ", timeouts: "            << m_totalDriverTimeouts
              << ", ring buffer overruns: "<< m_totalRingBufferOverruns << "\n";

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

int CDriverGTecGUSBamp::read(sampleType* data, uint64_t /*size*/)
{
    const uint32_t totalChannels  = static_cast<uint32_t>((GTEC_NUM_CHANNELS + 1) * numDevices());
    const uint32_t activeChannels = static_cast<uint32_t>(m_channelCount.value());

    {
        std::unique_lock<std::mutex> lock(m_io_mutex);
        m_itemAvailable.wait(lock, [this] {
            return m_ringBuffer.GetSize() >= m_validPoints || !m_isThreadRunning;
        });

        if (!m_isThreadRunning && m_ringBuffer.GetSize() < m_validPoints)
            return 0;

        if (m_bufferOverrun)
        {
            m_ringBuffer.Reset();
            m_bufferOverrun = false;
            m_totalRingBufferOverruns++;
            m_itemAvailable.notify_one();
            return 0;
        }

        m_ringBuffer.Read(m_bufferReceivedData, static_cast<uint32_t>(m_validPoints));
        m_itemAvailable.notify_one();
    }

    // Ring buffer layout (scan-major): [scan0*(dev0ch0..dev0ch16, dev1ch0..dev1ch16,...), scan1*..., ...]
    // Output layout (scan-major):      data[scan * activeChannels + outputChannelIdx]
    for (uint32_t i = 0; i < totalChannels; ++i)
    {
        const int channel = m_channels[i].idx;
        if (channel < 0) continue;
        for (uint32_t j = 0; j < m_chunkSize; ++j)
            data[j * activeChannels + channel] = m_bufferReceivedData[j * totalChannels + i];
    }

    return static_cast<int>(m_chunkSize);
}

void CDriverGTecGUSBamp::setChunkSize(uint32_t chunkSize)
{
    m_chunkSize = chunkSize;
}

// ─────────────────────────────────────────────────────────────────────────────
// acquire() — runs in m_threadPtr; based on DoAcquisition from gUSBampSyncDemo
// ─────────────────────────────────────────────────────────────────────────────

bool CDriverGTecGUSBamp::acquire()
{
    // First-loop setup: allocate per-device per-queue buffers and prime GT_GetData calls
    if (m_flagIsFirstLoop)
    {
        const DWORD bufBytes = bufferSizeBytes();
        for (uint32_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
        {
            m_buffers[deviceIndex]    = new BYTE*[QUEUE_SIZE];
            m_overlapped[deviceIndex] = new OVERLAPPED[QUEUE_SIZE];

            for (int queueIndex = 0; queueIndex < QUEUE_SIZE; ++queueIndex)
            {
                m_buffers[deviceIndex][queueIndex] = new BYTE[bufBytes];
                memset(&(m_overlapped[deviceIndex][queueIndex]), 0, sizeof(OVERLAPPED));
                m_overlapped[deviceIndex][queueIndex].hEvent =
                    CreateEvent(nullptr, false, false, nullptr);

                if (!m_overlapped[deviceIndex][queueIndex].hEvent)
                {
                    std::cout << "[gUSBamp] Could not create event handle.\n";
                    return false;
                }
            }
        }

        for (uint32_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
        {
            HANDLE hDevice = m_devices[deviceIndex].handle;
            const DWORD bufBytes2 = bufferSizeBytes();
            for (int queueIndex = 0; queueIndex < QUEUE_SIZE; ++queueIndex)
            {
                if (!GT_GetData(hDevice, m_buffers[deviceIndex][queueIndex],
                                bufBytes2, &m_overlapped[deviceIndex][queueIndex]))
                {
                    std::cout << "[gUSBamp] GT_GetData failed during initialization.\n";
                    return false;
                }
            }
        }

        m_flagIsFirstLoop = false;
        m_currentQueueIdx = 0;
    }

    while (m_isThreadRunning)
    {
        try
        {
            DWORD numBytesReceived = 0;
            const DWORD bufBytes   = bufferSizeBytes();

            // Wait for data from all devices
            for (uint32_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
            {
                HANDLE device = m_devices[deviceIndex].handle;
                if (WaitForSingleObject(
                        m_overlapped[deviceIndex][m_currentQueueIdx].hEvent, 1000) == WAIT_TIMEOUT)
                    m_totalDriverTimeouts++;

                GetOverlappedResult(device,
                                    &m_overlapped[deviceIndex][m_currentQueueIdx],
                                    &numBytesReceived, false);
                if (numBytesReceived != bufBytes)
                    m_totalDriverChunksLost++;
            }

            // Push received scans into the ring buffer
            {
                std::lock_guard<std::mutex> lock(m_io_mutex);
                try
                {
                    m_bufferOverrun = (m_ringBuffer.GetFreeSize() <
                                       static_cast<int>(m_chunkSize * (GTEC_NUM_CHANNELS + 1) * numDevices()));

                    for (size_t scanIndex = 0; scanIndex < m_chunkSize; ++scanIndex)
                    {
                        for (size_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
                        {
                            m_ringBuffer.Write(
                                reinterpret_cast<float*>(
                                    m_buffers[deviceIndex][m_currentQueueIdx]
                                    + scanIndex * (GTEC_NUM_CHANNELS + 1) * sizeof(float)
                                    + HEADER_SIZE),
                                static_cast<uint32_t>(GTEC_NUM_CHANNELS + 1));
                        }
                    }
                }
                catch (std::exception& e)
                {
                    std::cout << "[gUSBamp] Error writing to ring buffer: " << e.what() << "\n";
                }
                m_itemAvailable.notify_one();
            }

            // Re-queue GT_GetData for the buffer slot just processed
            for (uint32_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
            {
                HANDLE hDevice = m_devices[deviceIndex].handle;
                if (!GT_GetData(hDevice,
                                m_buffers[deviceIndex][m_currentQueueIdx],
                                bufBytes,
                                &m_overlapped[deviceIndex][m_currentQueueIdx]))
                {
                    std::cout << "[gUSBamp] GT_GetData failed in standard loop.\n";
                    m_isThreadRunning = false;
                }
            }

            m_currentQueueIdx = (m_currentQueueIdx + 1) % QUEUE_SIZE;
        }
        catch (std::exception& e)
        {
            std::cout << "[gUSBamp] Error in acquire thread: " << e.what() << "\n";
            m_isThreadRunning = false;
        }
    }

    // Thread teardown: drain pending overlapped ops, stop & reset devices, free per-queue resources
    std::cout << "[gUSBamp] Stopping devices and cleaning up...\n";

    for (uint32_t i = 0; i < numDevices(); ++i)
    {
        HANDLE hDevice = m_devices[i].handle;

        for (int j = 0; j < QUEUE_SIZE; ++j)
        {
            WaitForSingleObject(m_overlapped[i][m_currentQueueIdx].hEvent, 1000);
            CloseHandle(m_overlapped[i][m_currentQueueIdx].hEvent);
            m_currentQueueIdx = (m_currentQueueIdx + 1) % QUEUE_SIZE;
        }

        if (!GT_Stop(hDevice))
            std::cout << "[gUSBamp] Stop failed for: " << m_devices[i].serial << "\n";
        if (!GT_ResetTransfer(hDevice))
            std::cout << "[gUSBamp] ResetTransfer failed for: " << m_devices[i].serial << "\n";

        for (int j = 0; j < QUEUE_SIZE; ++j)
            delete[] m_buffers[i][j];
        delete[] m_overlapped[i];
        delete[] m_buffers[i];
    }

    m_flagIsFirstLoop = true;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void CDriverGTecGUSBamp::detectDevices()
{
    m_devices.clear();
    char serial[16];
    for (int i = 0; i < 11; ++i)
    {
        HANDLE handle = GT_OpenDevice(i);
        if (handle)
        {
            GDevice device;
            device.handle = handle;
            GT_GetSerial(handle, serial, 16);
            device.serial = serial;
            m_devices.push_back(device);
        }
    }
    
    m_availableMasterSerialValues.clear();
    for (size_t j = 0; j < m_devices.size(); ++j) {
        m_availableMasterSerialValues.push_back(m_devices[j].serial);
    }
    m_masterSerial.setAvailableValues(m_availableMasterSerialValues);
}

bool CDriverGTecGUSBamp::configureDevice(const uint32_t deviceNumber)
{
    HANDLE device            = m_devices[deviceNumber].handle;
    const std::string serial = m_devices[deviceNumber].serial;

    UCHAR channel[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

    const uint8_t bitmap = static_cast<uint8_t>(m_commonGndAndRefBitmap.value());
    GND ground;
    ground.GND1 = (bitmap & 1);
    ground.GND2 = (bitmap & (1 << 1));
    ground.GND3 = (bitmap & (1 << 2));
    ground.GND4 = (bitmap & (1 << 3));

    REF reference;
    reference.ref1 = (bitmap & (1 << 4));
    reference.ref2 = (bitmap & (1 << 5));
    reference.ref3 = (bitmap & (1 << 6));
    reference.ref4 = (bitmap & (1 << 7));

    const uint32_t mode = m_calibrationSignalEnabled.value() ? M_CALIBRATE : M_NORMAL;
    if (!GT_SetMode(device, static_cast<UCHAR>(mode)))
        std::cout << "[gUSBamp] " << serial << ": GT_SetMode failed.\n";
    if (!GT_SetBufferSize(device, static_cast<WORD>(m_chunkSize)))
        std::cout << "[gUSBamp] " << serial << ": GT_SetBufferSize failed.\n";
    if (!GT_SetChannels(device, channel, sizeof(channel) / sizeof(UCHAR)))
        std::cout << "[gUSBamp] " << serial << ": GT_SetChannels failed.\n";
    if (!GT_EnableTriggerLine(device, TRUE))
        std::cout << "[gUSBamp] " << serial << ": GT_EnableTriggerLine failed (trigger channel disabled).\n";

    CHANNEL settings;
    if (m_bipolarEnabled.value())
    {
        settings.Channel1  = 2;  settings.Channel2  = 0;
        settings.Channel3  = 4;  settings.Channel4  = 0;
        settings.Channel5  = 6;  settings.Channel6  = 0;
        settings.Channel7  = 8;  settings.Channel8  = 0;
        settings.Channel9  = 10; settings.Channel10 = 0;
        settings.Channel11 = 12; settings.Channel12 = 0;
        settings.Channel13 = 14; settings.Channel14 = 0;
        settings.Channel15 = 16; settings.Channel16 = 0;
    }
    else
    {
        memset(&settings, 0, sizeof(settings));
    }
    if (!GT_SetBipolar(device, settings))
        std::cout << "[gUSBamp] " << serial << ": GT_SetBipolar failed.\n";
    else if (m_bipolarEnabled.value())
        std::cout << "[gUSBamp] " << serial << ": Bipolar configuration active.\n";

    configFiltering(device);

    if (!GT_SetSampleRate(device, static_cast<WORD>(m_samplingRate.value())))
        std::cout << "[gUSBamp] " << serial << ": GT_SetSampleRate failed.\n";

    if (mode == M_NORMAL)
    {
        if (!GT_SetReference(device, reference))
            std::cout << "[gUSBamp] " << serial << ": GT_SetReference failed.\n";
        if (!GT_SetGround(device, ground))
            std::cout << "[gUSBamp] " << serial << ": GT_SetGround failed.\n";
    }

    return true;
}

bool CDriverGTecGUSBamp::setMasterDevice(const std::string& targetMasterSerial)
{
    int targetIndex = -1;
    for (size_t i = 0; i < numDevices(); ++i)
        if (m_devices[i].serial == targetMasterSerial) { targetIndex = static_cast<int>(i); break; }

    const uint32_t lastIndex = static_cast<uint32_t>(numDevices() - 1);

    if (numDevices() < 2 || targetIndex < 0)
    {
        std::cout << "[gUSBamp] Invalid master selection or wrong device count.\n";
        return false;
    }

    if (static_cast<uint32_t>(targetIndex) != lastIndex)
        std::swap(m_devices[lastIndex], m_devices[targetIndex]);

    m_slavesCnt  = 0;
    m_mastersCnt = 0;

    for (uint32_t i = 0; i < numDevices(); ++i)
    {
        const bool isSlave = (i != lastIndex);
        if (!GT_SetSlave(m_devices[i].handle, isSlave))
            std::cout << "[gUSBamp] GT_SetSlave failed.\n";

        if (isSlave)
        {
            std::cout << "[gUSBamp] Slave: "  << m_devices[i].serial << "\n";
            m_slavesCnt++;
        }
        else
        {
            std::cout << "[gUSBamp] MASTER: " << m_devices[i].serial << "\n";
            m_mastersCnt++;
            m_masterSerial.setValue(m_devices[i].serial);
        }
    }

    return verifySyncMode();
}

bool CDriverGTecGUSBamp::verifySyncMode()
{
    if (numDevices() > 1)
    {
        if ((m_mastersCnt + m_slavesCnt != numDevices()) || m_mastersCnt != 1)
        {
            std::cout << "[gUSBamp] Master/slave count mismatch!\n";
            return false;
        }
        if (m_devices.back().serial != m_masterSerial.value())
        {
            std::cout << "[gUSBamp] Master device is not last in sequence!\n";
            return false;
        }
    }
    return true;
}

void CDriverGTecGUSBamp::configFiltering(HANDLE device)
{
    const std::string serial = getSerialByHandler(device);
    const float fs = static_cast<float>(m_samplingRate.value());

    int nFilters = 0;

    // Bandpass
    if (!GT_GetNumberOfFilter(&nFilters))
    {
        std::cout << "[gUSBamp] " << serial << ": Could not get bandpass filter count.\n";
        return;
    }
    FILT* filters = new FILT[nFilters];
    if (!GT_GetFilterSpec(filters))
    {
        std::cout << "[gUSBamp] " << serial << ": Could not get bandpass filter specs.\n";
        delete[] filters; return;
    }
    for (int i = 1; i <= GTEC_NUM_CHANNELS; ++i)
        if (!GT_SetBandPass(device, static_cast<UCHAR>(i), m_bandPassFilterIdx.value()))
            std::cout << "[gUSBamp] " << serial << ": GT_SetBandPass failed on channel " << i << "\n";

    if (m_bandPassFilterIdx.value() == -1)
        std::cout << "[gUSBamp] " << serial << ": No bandpass filter applied.\n";
    else
        std::cout << "[gUSBamp] " << serial << ": Bandpass [" << filters[m_bandPassFilterIdx.value()].fu
                  << " - " << filters[m_bandPassFilterIdx.value()].fo << " Hz] applied at " << fs << " Hz.\n";
    delete[] filters;

    // Notch
    if (!GT_GetNumberOfNotch(&nFilters))
    {
        std::cout << "[gUSBamp] " << serial << ": Could not get notch filter count.\n";
        return;
    }
    filters = new FILT[nFilters];
    if (!GT_GetNotchSpec(filters))
    {
        std::cout << "[gUSBamp] " << serial << ": Could not get notch filter specs.\n";
        delete[] filters; return;
    }
    for (int i = 1; i <= GTEC_NUM_CHANNELS; ++i)
        if (!GT_SetNotch(device, static_cast<UCHAR>(i), m_notchFilterIdx.value()))
            std::cout << "[gUSBamp] " << serial << ": GT_SetNotch failed on channel " << i << "\n";

    if (m_notchFilterIdx.value() == -1)
        std::cout << "[gUSBamp] " << serial << ": No notch filter applied.\n";
    else
    {
        const uint32_t freq = static_cast<uint32_t>(
            (filters[m_notchFilterIdx.value()].fo + filters[m_notchFilterIdx.value()].fu) / 2);
        std::cout << "[gUSBamp] " << serial << ": Notch at " << freq << " Hz applied.\n";
    }
    delete[] filters;
}

void CDriverGTecGUSBamp::buildChannelMap()
{
    const uint32_t totalChannels = static_cast<uint32_t>((GTEC_NUM_CHANNELS + 1) * numDevices());
    m_channels.clear();
    m_channels.resize(totalChannels);

    for (uint32_t i = 0, signalChannels = 0, eventChannels = 0; i < totalChannels; ++i)
    {
        const uint32_t numChannelsPerDevice = static_cast<uint32_t>(GTEC_NUM_CHANNELS + 1);
        const uint32_t deviceIdx            = i / numChannelsPerDevice;
        const uint32_t channelIdx           = i % numChannelsPerDevice;

        m_channels[i].gtecDeviceIdx  = static_cast<int>(deviceIdx);
        m_channels[i].gtecChannelIdx = static_cast<int>(channelIdx);

        if ((i + 1) % (GTEC_NUM_CHANNELS + 1) == 0)
        {
            m_channels[i].idx            = m_triggerInputEnabled.value()
                                           ? static_cast<int>(signalChannels + eventChannels) : -1;
            m_channels[i].oldIdx         = -1;
            m_channels[i].isEventChannel = true;
            if (m_triggerInputEnabled.value()) eventChannels++;
        }
        else if (signalChannels < m_nAcquiredChannel)
        {
            m_channels[i].idx            = static_cast<int>(signalChannels + eventChannels);
            m_channels[i].oldIdx         = static_cast<int>(signalChannels);
            m_channels[i].isEventChannel = false;
            signalChannels++;
        }
        else
        {
            m_channels[i].idx            = -1;
            m_channels[i].oldIdx         = -1;
            m_channels[i].isEventChannel = false;
        }
    }
}

void CDriverGTecGUSBamp::updateChannelNames()
{
    const uint32_t triggerChannels = m_triggerInputEnabled.value()
                                     ? static_cast<uint32_t>(numDevices()) : 0u;
    const uint32_t totalOutput     = m_nAcquiredChannel + triggerChannels;

    // Update channel count so the framework knows about trigger channels
    m_channelCount.setValue(static_cast<int>(totalOutput));

    const uint32_t maxPerAmp = static_cast<uint32_t>(GTEC_NUM_CHANNELS + 1);
    std::vector<std::string> names(totalOutput);

    for (size_t i = 0; i < m_channels.size(); ++i)
    {
        if (m_channels[i].idx < 0) continue;
        const uint32_t ampIdx = static_cast<uint32_t>(i) / maxPerAmp;

        std::string name;
        if (m_channels[i].isEventChannel)
            name = "CH_Event" + std::to_string(ampIdx);
        else
            name = "Ch " + std::to_string(m_channels[i].oldIdx + 1);

        if (m_showDeviceName.value() && ampIdx < m_devices.size())
            name += "_" + m_devices[ampIdx].serial;

        names[static_cast<size_t>(m_channels[i].idx)] = name;
    }

    m_channelNames.setValue(names);
}

std::string CDriverGTecGUSBamp::getSerialByHandler(HANDLE device) const
{
    for (size_t i = 0; i < m_devices.size(); ++i)
        if (m_devices[i].handle == device) return m_devices[i].serial;
    return "UNKNOWN";
}
