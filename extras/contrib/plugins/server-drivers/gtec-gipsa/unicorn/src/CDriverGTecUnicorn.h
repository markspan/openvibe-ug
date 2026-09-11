/**
 * SPDX-FileCopyrightText: Inria / Gipsa-lab
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the Gipsa-lab g.tec Unicorn Hybrid Black driver (originally by Anton Andreev,
 * VIBS team). Single device, threaded acquisition through the g.tec ring buffer.
 */
#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

#include <Windows.h>
#include "ringbuffer.h"

#include <unicorn.h>

#include <openvibe/parameters/parameterBool.h>
#include <openvibe/parameters/parameterListString.h>
#include <openvibe/parameters/parameterMultiSelect.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <string>

/**
 * \brief Acquisition driver for the g.tec Unicorn Hybrid Black.
 *
 * The device always digitises the same 17 channels (8 EEG, 3 accelerometer, 3 gyroscope,
 * Counter, Battery Level, Validation Indicator) at a fixed 250 Hz. The Channels parameter
 * selects which of those groups are forwarded to the acquisition server; the rest are read
 * from the device and dropped in read().
 */
class CDriverGTecUnicorn final : public AbstractDriver
{
public:
    CDriverGTecUnicorn();

    bool init() override;
    bool unInit() override;
    bool start() override;
    bool stop() override;
    int  read(sampleType* data, uint64_t size) override;
    void setChunkSize(uint32_t chunkSize) override;

    /** Re-enumerates paired devices so the Device dropdown is current whenever the UI reads it. */
    OpenViBE::Parameters::Parameters getParameters() override;

    bool acquire(); // acquisition thread body

private:
    static const int BUFFER_SIZE_SECONDS = 2;

    /**
     * \brief One entry of the Channels multi-select.
     *
     * \c vendorNames are the keys UNICORN_GetChannelIndex() understands (documented in
     * unicorn.h); \c outputNames are the shorter names published to the acquisition server,
     * kept identical to the ones the pre-port driver put in its CHeader.
     */
    struct SChannelGroup
    {
        const char*              label;
        std::vector<const char*> vendorNames;
        std::vector<const char*> outputNames;
    };

    /** The selectable groups, in the order their channels appear in the output signal. */
    static const std::vector<SChannelGroup>& channelGroups();
    /** Labels of the above, i.e. the available values of the Channels multi-select. */
    static std::vector<std::string> groupLabels();

    void detectDevices();
    void enableAllDeviceChannels();
    bool buildChannelMap();
    void closeDevice();

    // Configurable parameters
    OpenViBE::Parameters::ParameterListString  m_device;
    OpenViBE::Parameters::ParameterMultiSelect m_channels;
    OpenViBE::Parameters::ParameterBool        m_testSignal;

    std::vector<std::string> m_availableSerials;

    UNICORN_HANDLE m_handle   = 0;
    bool           m_isOpen   = false;

    uint32_t m_chunkSize        = 32;
    uint32_t m_scanChannelCount = 0;  ///< Floats per scan as delivered by the device (17).

    std::vector<uint32_t> m_selectedIndices; ///< Offsets within a scan to publish, in output order.
    int                   m_counterIdx = -1; ///< Offset of "Counter" in a scan, -1 when unavailable.

    // Buffers & ring-buffer state
    std::vector<float> m_rawScan;     ///< Device -> ring buffer, written by the acquisition thread.
    std::vector<float> m_ringScratch; ///< Ring buffer -> read(), owned by the reader.
    CRingBuffer<float> m_ringBuffer;
    bool               m_bufferOverrun = false;

    std::unique_ptr<std::thread> m_threadPtr;
    bool                         m_isThreadRunning = false;
    std::mutex                   m_io_mutex;
    std::condition_variable      m_itemAvailable;

    // Statistics
    uint32_t m_totalRingBufferOverruns = 0;
    uint32_t m_totalCounterErrors      = 0;
    float    m_lastCounter             = -1.0F; ///< Counter of the last scan handed out by read().
};

// REGISTER_DRIVER lives in CDriverGTecUnicorn.cpp: in plugin builds it defines the exported
// getInstance() factory, which must exist in exactly one translation unit.
