// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "../CommonBioSemi.h"

#include <cstdint>
#include <vector>

/**
 * @brief USB bridge to a BioSemi ActiveThree AD-box, through the vendor LabView DLL.
 *
 * Unlike the ActiveTwo, the ActiveThree describes itself in a packet header: sampling rate is
 * fixed and the channel counts (AUX / EX / EEG modules) are read out of the fourth word of the
 * first packet seen after start().
 *
 * The vendor header (labview_dll.h) is deliberately confined to the .cpp -- it defines bare macros
 * such as OPEN_DRIVER, READ_POINTER and USB_WRITE, and this bridge is compiled into the acquisition
 * server executable alongside everything else.
 */
class CBridgeBioSemiActiveThree final
{
public:
    CBridgeBioSemiActiveThree() = default;
    ~CBridgeBioSemiActiveThree() = default;

    bool isDeviceConnected();
    bool open();
    bool start();
    int read();
    bool discard();
    uint32_t getAvailableSampleCount();
    uint32_t getEEGChannelCount() const;
    uint32_t getEXChannelCount() const;
    size_t getChannelCount() const { return m_channelCount; }
    bool consumeOnePacket(float* pSampleBuffer);
    bool stop();
    bool close();

    bool isSynced() const { return m_bridgeSyncedWithDevice; }

    bool getTrigger(const uint32_t index) const { return (index > m_triggers.size() ? false : m_triggers[index]); }
    bool isCMSInRange() const { return m_cmsInRange; }
    bool isBatteryLow() const { return m_batteryLow; }
    uint32_t getSamplingFrequency() const { return m_samplingFrequency; }

    uint32_t getLastError() const { return m_lastError; }

    bool isUseEXChannels() const { return m_useEXChannels; }
    void setUseEXChannels(const bool useEXChannels) { m_useEXChannels = useEXChannels; }
    void setEffectiveEEGChannelCount(const uint32_t count) { m_effectiveEEGChannelCount = count; }

    // Channels info
    static constexpr uint32_t MaxEEGChannelCount = 128;
    static constexpr uint32_t MaxEXChannelCount = 8;
    static constexpr uint32_t MaxAUXChannelCount = 6;

protected:

    //___________________________________________________________________//
    //                                                                   //
    // Ring buffer must be large enough to hold at least a complete sample round
    // otherwise we wont be able to detect the length between 2 sync bytes and deduce channel count.
    // test gives 131072 bytes (32768 long) per read
    // it needs to be a multiple of 512
    static constexpr uint32_t RingBufferLongCount = (32768 * 512);
    static constexpr uint32_t RingBufferByteCount = (RingBufferLongCount * sizeof(uint32_t)); //67108864 bytes

    // The control buffer is sent through USB_WRITE. It is a 64 bytes buffer.
    // First byte controls the start/stop filling of ring buffer; if Odd, it starts.
    // 2nd and 3rd byte are used for outgoing triggers.
    // byte 4-64 unused.
    static constexpr uint32_t ControlBufferByteCount = 64;

    // Packet structure in 32bit words
    static constexpr size_t PacketHeaderSize = 4;
    static constexpr size_t PacketFooterSize = 1; // Batt voltage + Packet number
    static constexpr size_t PacketOverheadSize = PacketHeaderSize + PacketFooterSize;

    // SYNC bytes pads the stream that sends a fixed number of bytes per read() independently of the channel count.
    static constexpr uint32_t SyncByte = 0x01;
    static constexpr uint32_t SyncByteMask = 0xFF;

    // Flags in the status word
    static constexpr uint32_t FlagsMask = 0xFFF0000;
    static constexpr uint32_t FlagsShift = 16;
    static constexpr uint32_t LowBatteryMask = 1 << 11;
    static constexpr uint32_t CMSInRangeMask = 1 << 7;

    // Channels count in header word
    static constexpr uint32_t SensorCountMask = 0x00F00000;
    static constexpr uint32_t SensorCountShift = 20;
    static constexpr uint32_t TouchProofCountMask = 0x000FC000;
    static constexpr uint32_t TouchProofCountShift = 14;
    static constexpr uint32_t ModuleCountMask = 0x00003F00;
    static constexpr uint32_t ModuleCountShift = 8;

    static constexpr size_t ChannelsPerModule = 8;


    bool updateStatusFromFlags(uint16_t value);
    uint32_t getAvailableByteCount() const;
    void consumeBytes(uint32_t count);

    // Handle to the device
    void* m_hDevice = nullptr;
    // Ring buffer. 64 MiB, so it is allocated by open() and released by close() rather than held
    // for the lifetime of the driver: the acquisition server instantiates every built-in driver it
    // knows about, including the ones the user never selects.
    std::vector<char> m_buffers;
    // 64 bytes buffer for USB_WRITE operations
    std::vector<char> m_controlBuffers;

    /* deduced from the SYNC channel on first read */
    bool m_firstRead                    = false;
    bool m_bridgeSyncedWithDevice       = false;
    uint32_t m_EEGChannelCount          = 0; // Available from device
    uint32_t m_effectiveEEGChannelCount = 0; // Asked by user
    uint32_t m_EXChannelCount           = 0;
    uint32_t m_AUXChannelCount          = 0;
    uint32_t m_channelCount             = 0;
    uint32_t m_nInitialChannel          = 0;

    /* From Status channel */
    std::vector<bool> m_triggers;
    bool m_cmsInRange            = false;
    bool m_batteryLow            = false;
    uint32_t m_samplingFrequency = 16384;

    // ring buffer indices
#ifdef _WIN64
    long long m_lastRingBufferByteIdx = 0;	//Buffer filled up to this point in last read()
#else
    uint32_t m_lastRingBufferByteIdx = 0;	//Buffer filled up to this point in last read()]
#endif
    uint32_t m_consumptionByteIdx = 0;		//Next consume() should start here, it should ALWAYS be a SYNC byte.

    // just for stats
    uint32_t m_nTotalByteRead = 0;
    uint32_t m_lastError      = BioSemiError_NoError;
    bool m_useEXChannels      = false;
};
