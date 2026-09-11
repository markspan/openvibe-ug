/**
 * SPDX-FileCopyrightText: Inria / Gipsa-lab
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the Gipsa-lab g.tec Unicorn Hybrid Black driver (originally by Anton Andreev,
 * VIBS team).
 */

#include "CDriverGTecUnicorn.h"

#include <algorithm>
#include <iostream>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
// Channel groups
// ─────────────────────────────────────────────────────────────────────────────

const std::vector<CDriverGTecUnicorn::SChannelGroup>& CDriverGTecUnicorn::channelGroups()
{
    // The vendor names are the exact keys UNICORN_GetChannelIndex() accepts (see unicorn.h);
    // the output names are the ones the pre-port driver published, kept for compatibility.
    static const std::vector<SChannelGroup> groups = {
        { "EEG",
          { "EEG 1", "EEG 2", "EEG 3", "EEG 4", "EEG 5", "EEG 6", "EEG 7", "EEG 8" },
          { "EEG1", "EEG2", "EEG3", "EEG4", "EEG5", "EEG6", "EEG7", "EEG8" } },
        { "Accelerometer",
          { "Accelerometer X", "Accelerometer Y", "Accelerometer Z" },
          { "ACCX", "ACCY", "ACCZ" } },
        { "Gyroscope",
          { "Gyroscope X", "Gyroscope Y", "Gyroscope Z" },
          { "GYRX", "GYRY", "GYRZ" } },
        { "Counter",    { "Counter" },              { "Counter" } },
        { "Battery",    { "Battery Level" },        { "Battery" } },
        { "Validation", { "Validation Indicator" }, { "VI" } },
    };
    return groups;
}

std::vector<std::string> CDriverGTecUnicorn::groupLabels()
{
    std::vector<std::string> labels;
    labels.reserve(channelGroups().size());
    for (const auto& group : channelGroups())
    {
        labels.push_back(group.label);
    }
    return labels;
}

// ─────────────────────────────────────────────────────────────────────────────

CDriverGTecUnicorn::CDriverGTecUnicorn()
    : m_device("Device", "Serial number of the Unicorn to acquire from", {}),
      m_channels("Channels", "Channel groups to forward to the acquisition server",
                 groupLabels(), { "EEG" }),
      m_testSignal("TestSignal", "Replace the EEG signal by the device's internal test signal", false)
{
    // The Unicorn digitises at a fixed rate and its channel count follows the Channels
    // selection, so neither is offered to the user (see CDriverLabStreamingLayer for the
    // same arrangement).
    m_samplingRate = UNICORN_SAMPLING_RATE;

    m_parameters.push_back(&m_device);
    m_parameters.push_back(&m_channels);
    m_parameters.push_back(&m_testSignal);

    detectDevices();
}

OpenViBE::Parameters::Parameters CDriverGTecUnicorn::getParameters()
{
    detectDevices();
    return AbstractDriver::getParameters();
}

void CDriverGTecUnicorn::detectDevices()
{
    m_availableSerials.clear();

    uint32_t count = 0;
    if (UNICORN_GetAvailableDevices(nullptr, &count, TRUE) == UNICORN_ERROR_SUCCESS && count > 0)
    {
        std::unique_ptr<UNICORN_DEVICE_SERIAL[]> serials(new UNICORN_DEVICE_SERIAL[count]);
        if (UNICORN_GetAvailableDevices(serials.get(), &count, TRUE) == UNICORN_ERROR_SUCCESS)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                m_availableSerials.push_back(serials[i]);
            }
        }
        else { std::cout << "[Unicorn] Could not list paired devices." << std::endl; }
    }

    // setAvailableValues() keeps the current selection when the device is still paired and
    // otherwise falls back to the first entry (or to empty), which is exactly what we want.
    m_device.setAvailableValues(m_availableSerials);
}

// ─────────────────────────────────────────────────────────────────────────────

bool CDriverGTecUnicorn::init()
{
    bool success = false;

    detectDevices();

    if (m_device.value().empty())
    {
        std::cout << "[Unicorn] No device selected. Pair a Unicorn device first." << std::endl;
    }
    else if (m_channels.value().empty())
    {
        std::cout << "[Unicorn] No channel group selected." << std::endl;
    }
    else if (UNICORN_OpenDevice(m_device.value().c_str(), &m_handle) != UNICORN_ERROR_SUCCESS)
    {
        std::cout << "[Unicorn] Unable to connect to device: " << m_device.value() << std::endl;
    }
    else
    {
        m_isOpen = true;
        enableAllDeviceChannels();

        if (UNICORN_GetNumberOfAcquiredChannels(m_handle, &m_scanChannelCount) != UNICORN_ERROR_SUCCESS)
        {
            std::cout << "[Unicorn] Unable to get the number of acquired channels." << std::endl;
        }
        else if (!buildChannelMap()) { /* buildChannelMap() reported the reason */ }
        else
        {
            m_samplingRate.setValue(UNICORN_SAMPLING_RATE);

            m_rawScan.assign(size_t(m_chunkSize) * m_scanChannelCount, 0.0F);
            m_ringScratch.assign(size_t(m_chunkSize) * m_scanChannelCount, 0.0F);

            success = m_ringBuffer.Initialize(
                uint32_t(BUFFER_SIZE_SECONDS * UNICORN_SAMPLING_RATE * m_scanChannelCount));
            if (!success) { std::cout << "[Unicorn] Could not allocate the ring buffer." << std::endl; }
        }
    }

    if (!success) { closeDevice(); }
    else
    {
        std::cout << "[Unicorn] Connected to " << m_device.value() << ": "
                  << m_selectedIndices.size() << " of " << m_scanChannelCount
                  << " channels at " << UNICORN_SAMPLING_RATE << " Hz." << std::endl;
    }

    return success;
}

void CDriverGTecUnicorn::enableAllDeviceChannels()
{
    // The amplifier configuration is persistent device state, so a channel someone disabled in
    // another application would otherwise drop out of the scan -- UNICORN_GetChannelIndex() would
    // then fail for a group the user selected here. Acquire all 17 and pick in read() instead.
    UNICORN_AMPLIFIER_CONFIGURATION configuration;
    if (UNICORN_GetConfiguration(m_handle, &configuration) != UNICORN_ERROR_SUCCESS)
    {
        std::cout << "[Unicorn] Could not read the amplifier configuration; using it as-is." << std::endl;
        return;
    }

    bool changed = false;
    for (int i = 0; i < UNICORN_TOTAL_CHANNELS_COUNT; ++i)
    {
        if (!configuration.Channels[i].enabled)
        {
            configuration.Channels[i].enabled = TRUE;
            changed                           = true;
        }
    }

    if (changed && UNICORN_SetConfiguration(m_handle, &configuration) != UNICORN_ERROR_SUCCESS)
    {
        std::cout << "[Unicorn] Could not enable all device channels; some groups may be unavailable." << std::endl;
    }
}

bool CDriverGTecUnicorn::buildChannelMap()
{
    m_selectedIndices.clear();
    m_counterIdx = -1;

    const std::vector<std::string>& selection = m_channels.value();
    std::vector<std::string> names;

    // Iterate over the groups rather than over the selection, so the output channel order is
    // the declaration order and does not depend on how the selection was assembled.
    for (const auto& group : channelGroups())
    {
        if (std::find(selection.begin(), selection.end(), group.label) == selection.end()) { continue; }

        for (size_t i = 0; i < group.vendorNames.size(); ++i)
        {
            uint32_t index = 0;
            if (UNICORN_GetChannelIndex(m_handle, group.vendorNames[i], &index) != UNICORN_ERROR_SUCCESS)
            {
                std::cout << "[Unicorn] Could not resolve channel '" << group.vendorNames[i] << "'." << std::endl;
                return false;
            }
            m_selectedIndices.push_back(index);
            names.push_back(group.outputNames[i]);
        }
    }

    if (m_selectedIndices.empty())
    {
        std::cout << "[Unicorn] Channel selection resolved to no channel." << std::endl;
        return false;
    }

    // read() runs its continuity check on the counter whether or not the group is published.
    uint32_t counterIdx = 0;
    if (UNICORN_GetChannelIndex(m_handle, "Counter", &counterIdx) == UNICORN_ERROR_SUCCESS)
    {
        m_counterIdx = int(counterIdx);
    }

    m_channelCount.setValue(int(m_selectedIndices.size()));
    m_channelNames.setValue(names);

    return true;
}

bool CDriverGTecUnicorn::unInit()
{
    closeDevice();
    return true;
}

void CDriverGTecUnicorn::closeDevice()
{
    if (m_isOpen)
    {
        if (UNICORN_CloseDevice(&m_handle) != UNICORN_ERROR_SUCCESS)
        {
            std::cout << "[Unicorn] Unable to close device: " << m_device.value() << std::endl;
        }
        m_isOpen = false;
        m_handle = 0;
    }

    m_rawScan.clear();
    m_ringScratch.clear();
    m_selectedIndices.clear();
    m_scanChannelCount = 0;
    m_counterIdx       = -1;
}

// ─────────────────────────────────────────────────────────────────────────────

bool CDriverGTecUnicorn::start()
{
    m_totalRingBufferOverruns = 0;
    m_totalCounterErrors      = 0;
    m_lastCounter             = -1.0F;

    {
        std::lock_guard<std::mutex> lock(m_io_mutex);
        m_ringBuffer.Reset();
    }

    const BOOL testSignal = m_testSignal.value() ? TRUE : FALSE;
    if (UNICORN_StartAcquisition(m_handle, testSignal) != UNICORN_ERROR_SUCCESS)
    {
        std::cout << "[Unicorn] Could not start acquisition on " << m_device.value() << std::endl;
        return false;
    }

    m_isThreadRunning = true;
    m_bufferOverrun   = false;

    m_threadPtr.reset(new std::thread([this] { acquire(); }));

    return true;
}

bool CDriverGTecUnicorn::stop()
{
    m_isThreadRunning = false;
    m_itemAvailable.notify_all(); // unblock read() if it is waiting

    if (m_threadPtr && m_threadPtr->joinable()) { m_threadPtr->join(); }
    m_threadPtr.reset();

    std::cout << "[Unicorn] Acquisition complete."
              << " Ring buffer overruns: " << m_totalRingBufferOverruns
              << ", counter errors: "      << m_totalCounterErrors << std::endl;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

int CDriverGTecUnicorn::read(sampleType* data, uint64_t /*size*/)
{
    const uint32_t scanFloats  = uint32_t(m_chunkSize) * m_scanChannelCount;
    const size_t   outChannels = m_selectedIndices.size();

    if (outChannels == 0) { return 0; } // never initialised, or already torn down

    {
        std::unique_lock<std::mutex> lock(m_io_mutex);
        m_itemAvailable.wait(lock, [this, scanFloats]
        {
            return m_ringBuffer.GetSize() >= int(scanFloats) || !m_isThreadRunning;
        });

        if (!m_isThreadRunning && m_ringBuffer.GetSize() < int(scanFloats)) { return 0; }

        if (m_bufferOverrun)
        {
            m_ringBuffer.Reset();
            m_bufferOverrun = false;
            m_totalRingBufferOverruns++;
            m_lastCounter = -1.0F; // scans were dropped, so the next gap is expected
            m_itemAvailable.notify_one();
            return 0;
        }

        m_ringBuffer.Read(m_ringScratch.data(), scanFloats);
        m_itemAvailable.notify_one();
    }

    // Both layouts are sample-major: the device delivers scan-contiguous floats, and the
    // acquisition server expects data[sample * channelCount + channel]. So this is a gather
    // over the selected offsets, not a transpose.
    for (uint32_t scan = 0; scan < m_chunkSize; ++scan)
    {
        const float* src = &m_ringScratch[size_t(scan) * m_scanChannelCount];
        sampleType*  dst = &data[size_t(scan) * outChannels];
        for (size_t channel = 0; channel < outChannels; ++channel)
        {
            dst[channel] = src[m_selectedIndices[channel]];
        }

        // The device counter increments once per scan; any other step means scans were lost
        // between the amplifier and us.
        if (m_counterIdx >= 0)
        {
            const float counter = src[m_counterIdx];
            if (m_lastCounter >= 0.0F && counter != m_lastCounter + 1.0F) { m_totalCounterErrors++; }
            m_lastCounter = counter;
        }
    }

    return int(m_chunkSize);
}

void CDriverGTecUnicorn::setChunkSize(const uint32_t chunkSize)
{
    m_chunkSize = chunkSize;
}

// ─────────────────────────────────────────────────────────────────────────────
// acquire() — runs in m_threadPtr
// ─────────────────────────────────────────────────────────────────────────────

bool CDriverGTecUnicorn::acquire()
{
    const uint32_t scanFloats = uint32_t(m_chunkSize) * m_scanChannelCount;

    while (m_isThreadRunning)
    {
        try
        {
            // The last argument counts floats, not bytes -- the pre-port driver passed a byte
            // count here, overstating the buffer fourfold and defeating the SDK's size check.
            if (UNICORN_GetData(m_handle, m_chunkSize, m_rawScan.data(), scanFloats) != UNICORN_ERROR_SUCCESS)
            {
                std::cout << "[Unicorn] UNICORN_GetData failed." << std::endl;
                m_isThreadRunning = false;
                break;
            }

            {
                std::lock_guard<std::mutex> lock(m_io_mutex);
                try
                {
                    // If this write would overrun, flag it; the reader handles the recovery.
                    m_bufferOverrun = (m_ringBuffer.GetFreeSize() < int(scanFloats));
                    m_ringBuffer.Write(m_rawScan.data(), scanFloats);
                }
                catch (std::exception& e)
                {
                    std::cout << "[Unicorn] Error writing to ring buffer: " << e.what() << std::endl;
                }
                m_itemAvailable.notify_one();
            }
        }
        catch (std::exception& e)
        {
            std::cout << "[Unicorn] Error in acquire thread: " << e.what() << std::endl;
            m_isThreadRunning = false;
        }
    }

    std::cout << "[Unicorn] Stopping device..." << std::endl;
    if (UNICORN_StopAcquisition(m_handle) != UNICORN_ERROR_SUCCESS)
    {
        std::cout << "[Unicorn] Stopping device failed: " << m_device.value() << std::endl;
    }

    // read() may be parked on the condition variable: Acquisition::stop() calls the driver's
    // stop() before joining its reader thread, so wake it once more on the way out.
    m_itemAvailable.notify_all();

    return true;
}

REGISTER_DRIVER(CDriverGTecUnicorn, "g.tec Unicorn Gipsa-lab", false);
