/**
 * SPDX-FileCopyrightText: Inria / Gipsa-lab
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the Gipsa-lab g.USBamp driver (originally by Anton Andreev).
 */
#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

#include <Windows.h>
#include "ringbuffer.h"

#include <openvibe/parameters/parameterInt.h>
#include <openvibe/parameters/parameterBool.h>
#include <openvibe/parameters/parameterString.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <string>

struct GDevice
{
    HANDLE handle;
    std::string serial;
};

class CDriverGTecGUSBamp final : public AbstractDriver
{
public:
    CDriverGTecGUSBamp();

    bool init() override;
    bool unInit() override;
    bool start() override;
    bool stop() override;
    int  read(sampleType* data, uint64_t size) override;
    void setChunkSize(uint32_t chunkSize) override;

    bool acquire(); // acquisition thread body

private:
    static const int BUFFER_SIZE_SECONDS = 2;
    static const int GTEC_NUM_CHANNELS   = 16;
    static const int QUEUE_SIZE          = 8;

    DWORD bufferSizeBytes() const;

    size_t numDevices() const { return m_devices.size(); }

    uint32_t m_chunkSize = 32;
    uint32_t m_nAcquiredChannel = 0; // EEG channels only (before trigger expansion)
    bool     m_reconfigurationRequired = false;

    // Configurable parameters
    OpenViBE::Parameters::ParameterInt    m_commonGndAndRefBitmap;
    OpenViBE::Parameters::ParameterInt    m_notchFilterIdx;
    OpenViBE::Parameters::ParameterInt    m_bandPassFilterIdx;
    OpenViBE::Parameters::ParameterBool   m_triggerInputEnabled;

    std::vector<std::string> m_availableMasterSerialValues = {};

    OpenViBE::Parameters::ParameterListString m_masterSerial;
    OpenViBE::Parameters::ParameterBool   m_bipolarEnabled;
    OpenViBE::Parameters::ParameterBool   m_calibrationSignalEnabled;
    OpenViBE::Parameters::ParameterBool   m_showDeviceName;

    // Buffer & ring-buffer state
    int          m_validPoints       = 0;
    BYTE***      m_buffers           = nullptr;
    OVERLAPPED** m_overlapped        = nullptr;
    bool         m_flagIsFirstLoop   = true;
    bool         m_bufferOverrun     = false;
    CRingBuffer<float> m_ringBuffer;
    uint32_t     m_currentQueueIdx   = 0;

    std::unique_ptr<std::thread> m_threadPtr;
    bool                         m_isThreadRunning = false;
    std::mutex                   m_io_mutex;
    float*                       m_bufferReceivedData = nullptr;
    std::condition_variable      m_itemAvailable;

    // Multi-device
    uint32_t             m_mastersCnt = 0;
    uint32_t             m_slavesCnt  = 0;
    std::vector<GDevice> m_devices;

    struct SChannel
    {
        int  idx;            // output channel index in openvibe (-1 = unused)
        int  oldIdx;         // EEG channel index in user channel list
        int  gtecDeviceIdx;
        int  gtecChannelIdx;
        bool isEventChannel;
    };
    std::vector<SChannel> m_channels;

    // Statistics
    uint32_t m_totalDriverChunksLost    = 0;
    uint32_t m_totalDriverTimeouts      = 0;
    uint32_t m_totalRingBufferOverruns  = 0;

    void        detectDevices();
    bool        configureDevice(uint32_t deviceNumber);
    bool        setMasterDevice(const std::string& targetSerial);
    bool        verifySyncMode();
    void        configFiltering(HANDLE device);
    void        buildChannelMap();
    void        updateChannelNames();
    std::string getSerialByHandler(HANDLE device) const;
};

// REGISTER_DRIVER lives in CDriverGTecGUSBamp.cpp: in plugin builds it defines the exported
// getInstance() factory, which must exist in exactly one translation unit.
