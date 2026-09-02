// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CBridgeBioSemiActiveThree.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <bsif.h>
#include <labview_dll.h>

namespace {
/// Turns on the vendor DLL's own tracing when OV_BIOSEMI_DEBUG is set in the environment.
///
/// A failed OPEN_DRIVER() only ever says "No driver found!", which does not distinguish "the
/// AD-box is off" from "another process holds the WinUSB interface" from "WinUsb.dll could not be
/// resolved". BSIF's trace does, but the DLL writes its own diagnostics to stderr, which a GUI
/// process discards -- so route them to bsif_winusb.log, written in the working directory.
///
/// Off by default: the log is appended to on every open and the vendor writes it synchronously.
void enableVendorTracingIfRequested()
{
    static const bool requested = (std::getenv("OV_BIOSEMI_DEBUG") != nullptr);
    if (!requested) { return; }

    static bool applied = false;
    if (applied) { return; }
    applied = true;

    BSIF_SET_DEBUG(TRUE);
    BSIF_SET_LOG(TRUE);
    std::cout << "[BioSemi] Vendor tracing enabled, see bsif_winusb.log in the working directory.\n";
}

/// Tells whether OPEN_DRIVER() handed back a usable device handle, and reports why if it did not.
///
/// OPEN_DRIVER returns NULL -- not INVALID_HANDLE_VALUE -- when neither the BioSemi kernel driver
/// nor the WinUSB backend could be opened, so both values have to be rejected. Testing only
/// INVALID_HANDLE_VALUE lets a dead handle through, and the failure then surfaces much later as
/// ERROR_INVALID_HANDLE out of USB_WRITE.
///
/// GET_DRIVER_INFO is the only channel that says which backend was found: the vendor DLL writes
/// its own diagnostic to stderr, which is invisible in the GUI application.
bool openedDeviceHandleIsValid(void* handle)
{
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE) { return true; }

    char bufferInfo[256] = { 0 };
    GET_DRIVER_INFO(bufferInfo, sizeof(bufferInfo));
    std::cout << "[BioSemi] Failed to open driver: " << bufferInfo << "\n";
    return false;
}
}  // namespace

//___________________________________________________________________//
//                                                                   //

bool CBridgeBioSemiActiveThree::isDeviceConnected()
{
    enableVendorTracingIfRequested();

    m_hDevice = OPEN_DRIVER();
    if (!openedDeviceHandleIsValid(m_hDevice))
    {
        m_hDevice   = nullptr;
        m_lastError = BioSemiError_OSOpenFailed;
        return false;
    }

    Sleep(100);

    if (!CLOSE_DRIVER(m_hDevice))
    {
        std::cout << "[BioSemi] Device driver cannot be closed!\n";
        m_lastError = BioSemiError_OSCloseFailed;
        return false;
    }

    return true;
}


bool CBridgeBioSemiActiveThree::open()
{
    enableVendorTracingIfRequested();

    // The ring buffer is 64 MiB. It is allocated here rather than in the constructor so that a
    // driver instance the user never connects to costs nothing.
    m_buffers.assign(RingBufferByteCount, 0);
    m_controlBuffers.assign(ControlBufferByteCount, 0);

    m_lastRingBufferByteIdx = 0;
    m_consumptionByteIdx    = 0;
    m_firstRead             = true;

    m_nTotalByteRead = 0;

    m_EEGChannelCount        = 0;
    m_EXChannelCount         = 0;
    m_AUXChannelCount        = 0;
    m_channelCount           = 0;
    m_nInitialChannel        = 0;
    m_bridgeSyncedWithDevice = false;

    m_triggers.clear();
    m_triggers.resize(16, false);
    m_cmsInRange = false;
    m_batteryLow = false;

    m_hDevice = OPEN_DRIVER();
    if (!openedDeviceHandleIsValid(m_hDevice))
    {
        m_hDevice   = nullptr;
        m_lastError = BioSemiError_OSOpenFailed;
        m_buffers.clear();
        m_buffers.shrink_to_fit();
        return false;
    }

    char bufferInfo[256] = { 0 };
    GET_DRIVER_INFO(bufferInfo, sizeof(bufferInfo));
    std::cout << "[BioSemi] Driver info: " << bufferInfo << "\n";

    READ_MULTIPLE_SWEEPS(m_hDevice, &m_buffers[0], RingBufferByteCount);

    return true;
}

bool CBridgeBioSemiActiveThree::start()
{
    m_controlBuffers.assign(ControlBufferByteCount, 0);
    m_controlBuffers[0] = char(-1); // Odd number to start the filling of ring buffer

    if (!USB_WRITE(m_hDevice, &m_controlBuffers[0]))
    {
        std::cout << "[BioSemi] usb_write for enable handshake failed with [" << GetLastError() << "]\n";
        m_lastError = BioSemiError_EnableUSBHandshakeFailed;
        return false;
    }

    // Past this point, the ring buffer is constantly filled.
    return true;
}

int CBridgeBioSemiActiveThree::read()
{
    int bytesRead;
#ifdef _WIN64
    long long currentRingBufferByteIdx;
#else
    int currentRingBufferByteIdx;
#endif
    if (READ_POINTER(m_hDevice, &currentRingBufferByteIdx) == FALSE)
    {
        std::cout << "[BioSemi] READ_POINTER failed!\n";
        m_lastError = BioSemiError_ReadPointerFailed;
        return -1;
    }

    if (currentRingBufferByteIdx != m_lastRingBufferByteIdx)
    {
        bytesRead = int(currentRingBufferByteIdx - m_lastRingBufferByteIdx);

        // ring buffer: we can loop indices
        if (bytesRead < 0) { bytesRead += int(RingBufferByteCount); }

        const uint32_t unconsumedBytes = getAvailableByteCount();
        if (uint32_t(bytesRead) + unconsumedBytes > RingBufferByteCount)
        {
            std::cout << "[BioSemi] Buffer overflow! Lost " << (uint32_t(bytesRead) + unconsumedBytes - RingBufferByteCount) << " bytes of data\n";
            m_lastError              = BioSemiError_BufferOverflow;
            m_bridgeSyncedWithDevice = false; // Need to resync.
        }

        m_nTotalByteRead += uint32_t(bytesRead);
        m_lastRingBufferByteIdx = currentRingBufferByteIdx;

        uint32_t value;
        // for loop over LONG values, not bytes
        for (size_t offset = 0; offset < size_t(bytesRead) && !m_bridgeSyncedWithDevice; offset += sizeof(value))
        {
            // Needs at least header size to extract anything.
            if (m_buffers.size() >= offset + PacketHeaderSize * sizeof(value))
            {
                std::memcpy(&value, &m_buffers[offset], sizeof(value));
                /**
                 * Start of package first 32 bits
                 *
                 *       31 30  29  28  27  26  25  24  23  22  21  20  19  18  17  16  15  14  13  12  11  10  9  8  7  6  5  4  3  2  1  0
                 * 1 |		1	[RESERVED]  [                   FLAGS                    ]	[     FIRMWARE-VERSION     ]	0	 0	0	 0	0	 0 	0	 1
                 *
                 * Bit 0-7:  0x01 = SYNC
                 * Bit 31:  spec says 1, but 0 in practice.
                 */
                if ((value & SyncByteMask) == SyncByte)
                {
                    const uint16_t flags = uint16_t((value & FlagsMask) >> FlagsShift);
                    this->updateStatusFromFlags(flags);

                    // Get header word containing sample rate & channel count
                    std::memcpy(&value, &m_buffers[offset + 3 * sizeof(value)], sizeof(value));
                    // sample rate is constant, no need to read it
                    m_AUXChannelCount = (value & SensorCountMask) >> SensorCountShift;
                    m_EXChannelCount  = (value & TouchProofCountMask) >> TouchProofCountShift;
                    m_EEGChannelCount = uint32_t(((value & ModuleCountMask) >> ModuleCountShift) * ChannelsPerModule);
                    m_channelCount    = m_AUXChannelCount + m_EXChannelCount + m_EEGChannelCount;

                    m_bridgeSyncedWithDevice = true;

                    // The handshake is complete; we can safely consume the corresponding data
                    // channel# + SYNC (Status is consumed in updateStatusFromFlags function)
                    const uint32_t packetSize = uint32_t((PacketOverheadSize + m_channelCount) * sizeof(uint32_t));
                    this->consumeBytes(packetSize);

                    std::memcpy(&value, &m_buffers[m_consumptionByteIdx], sizeof(value));
                    if ((value & SyncByteMask) != SyncByte)
                    {
                        std::cout << "[BioSemi] Lost sync after init!\n";
                        m_lastError = BioSemiError_SyncLost;
                        return -1;
                    }
                }
            }
        }

        if (!m_bridgeSyncedWithDevice)
        {
            std::cout << "[BioSemi] Could not synchronize with the device!\n";
            m_lastError = BioSemiError_NoSync;
            return -1;
        }

        return bytesRead;
    }

    // nothing read
    return 0;
}

//___________________________________________________________________//
//                                                                   //

bool CBridgeBioSemiActiveThree::updateStatusFromFlags(const uint16_t value)
{
    m_cmsInRange = (value & CMSInRangeMask) != 0;
    m_batteryLow = (value & LowBatteryMask) != 0;

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CBridgeBioSemiActiveThree::discard()
{
    const uint32_t availableByteCount = getAvailableByteCount();
    const uint32_t packetByteCount    = uint32_t((PacketOverheadSize + m_channelCount) * sizeof(uint32_t));
    if (packetByteCount == 0) { return true; }

    // Check how many bytes are left after the last packet
    const uint32_t leftover = availableByteCount % packetByteCount;
    // Discard as many whole packets as possible
    const uint32_t byteCountToDiscard = availableByteCount - leftover;

    this->consumeBytes(byteCountToDiscard);
    return true;
}

uint32_t CBridgeBioSemiActiveThree::getAvailableByteCount() const
{
    if (m_lastRingBufferByteIdx >= m_consumptionByteIdx) { return uint32_t(m_lastRingBufferByteIdx - m_consumptionByteIdx); }

    return uint32_t(m_lastRingBufferByteIdx + (RingBufferByteCount - m_consumptionByteIdx));
}

uint32_t CBridgeBioSemiActiveThree::getAvailableSampleCount()
{
    if (!m_bridgeSyncedWithDevice) { return 0; }

    // there is one sample per packet
    return uint32_t(this->getAvailableByteCount() / ((PacketOverheadSize + m_channelCount) * sizeof(uint32_t)));
}

void CBridgeBioSemiActiveThree::consumeBytes(const uint32_t count) { m_consumptionByteIdx = (m_consumptionByteIdx + count) % RingBufferByteCount; }

bool CBridgeBioSemiActiveThree::consumeOnePacket(float* pSampleBuffer)
{
    // fail case: we can't consume if not yet sync'ed
    if (!m_bridgeSyncedWithDevice)
    {
        std::cout << "[BioSemi] Bridge is not synced with the device, cannot consume.\n";
        m_lastError = BioSemiError_NoSync;
        return false;
    }

    //fail case: we do nothing if we cannot completly fill the user buffer
    if (this->getAvailableByteCount() < (PacketHeaderSize + m_channelCount) * sizeof(uint32_t))
    {
        m_lastError = BioSemiError_NotEnoughDataInBuffer;
        return false;
    }

    // READ Sync/status word
    uint32_t value;
    std::memcpy(&value, &m_buffers[m_consumptionByteIdx], sizeof(value));

    if ((value & SyncByteMask) != SyncByte)
    {
        std::cout << "[BioSemi] Lost sync during consume!\n";
        m_lastError = BioSemiError_SyncLost;
        return false;
    }
    const uint16_t flags = uint16_t((value & FlagsMask) >> FlagsShift);
    if (!this->updateStatusFromFlags(flags))
    {
        std::cout << "[BioSemi] Failed to update status during consume!\n";
        return false;
    }

    // If a buffer was provided, fill it.
    if (pSampleBuffer != nullptr)
    {
        // jump to EX & EEG samples (AUX not supported)
        consumeBytes(uint32_t((PacketHeaderSize + MaxAUXChannelCount) * sizeof(uint32_t)));

        /*
        The receiver converts every 24-bit word from the AD-box into a 32-bit Signed Integer,
        by adding an extra zero Least Significant Byte to the ADC data.
        The 24-bit ADC output has an LSB value of 1/32th uV.
        The 32-bit Integer received for the USB interface has an LSB value of 1/32*1/256 = 1/8192th uV
        */
        const float factor = 1.f / 8192;

        int32_t data;

        if (m_useEXChannels)
        {
            // Reading all EX channels
            for (uint32_t i = 0; i < MaxEXChannelCount; ++i) {
                std::memcpy(&data, &m_buffers[m_consumptionByteIdx], sizeof(data));
                pSampleBuffer[m_effectiveEEGChannelCount + i] = data * factor;

                consumeBytes(sizeof(uint32_t));
            }
        }
        else
        {
            // Discarding all EX channels
            consumeBytes(MaxEXChannelCount * sizeof(uint32_t));
        }

        // Read requested EEG Channels
        for (uint32_t i = 0; i < m_effectiveEEGChannelCount; ++i) {
            std::memcpy(&data, &m_buffers[m_consumptionByteIdx], sizeof(data));
            pSampleBuffer[i] = data * factor;

            consumeBytes(sizeof(data));
        }

        // Discard non requested EEG Channels
        consumeBytes((m_EEGChannelCount - m_effectiveEEGChannelCount) * sizeof(uint32_t));

        // Consume footer
        consumeBytes(uint32_t(PacketFooterSize * sizeof(uint32_t)));
    }
    else
    {
        // Discard the whole packet
        consumeBytes(uint32_t((PacketOverheadSize + m_channelCount) * sizeof(uint32_t)));
    }

    return true;
}


uint32_t CBridgeBioSemiActiveThree::getEEGChannelCount() const
{
    if (!m_bridgeSyncedWithDevice) { return 0; }
    return m_EEGChannelCount;
}

uint32_t CBridgeBioSemiActiveThree::getEXChannelCount() const
{
    if (!m_useEXChannels || !m_bridgeSyncedWithDevice) { return 0; }
    return m_EXChannelCount;
}

//___________________________________________________________________//
//                                                                   //

bool CBridgeBioSemiActiveThree::stop()
{
    // To stop, we disable the handshake
    m_controlBuffers.assign(ControlBufferByteCount, 0); // stop byte

    if (!USB_WRITE(m_hDevice, &m_controlBuffers[0]))
    {
        std::cout << "[BioSemi] usb_write for disabling handshake failed with [" << GetLastError() << "]\n";
        m_lastError = BioSemiError_DisableUSBHandshakeFailed;
        return false;
    }

    return true;
}

bool CBridgeBioSemiActiveThree::close()
{
    const bool closed = (CLOSE_DRIVER(m_hDevice) != FALSE);
    if (!closed)
    {
        std::cout << "[BioSemi] Device driver cannot be closed!\n";
        m_lastError = BioSemiError_OSCloseFailed;
    }

    // Release the 64 MiB ring buffer whether or not the vendor driver acknowledged the close:
    // nothing may read from it once the handle is gone.
    m_hDevice                = nullptr;
    m_bridgeSyncedWithDevice = false;
    m_buffers.clear();
    m_buffers.shrink_to_fit();

    return closed;
}
