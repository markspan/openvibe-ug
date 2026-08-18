///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Lesser General Public
/// License as published by the Free Software Foundation; either
/// version 2.1 of the License, or (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
/// MA 02110-1301  USA
///-------------------------------------------------------------------------------------------------

#pragma once

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#include <vector>
#include <cstdint>

#define BIOSEMI_ACTIVETWO_MAXCHANNELCOUNT 256
#define BIOSEMI_ACTIVETWO_EXCHANNELCOUNT  8

namespace OpenViBE {
enum EBioSemiError
{
    BioSemiError_NoError,

    BioSemiError_OSOpenFailed,
    BioSemiError_OSCloseFailed,

    BioSemiError_EnableUSBHandshakeFailed,
    BioSemiError_DisableUSBHandshakeFailed,

    BioSemiError_ReadPointerFailed,

    BioSemiError_DeviceTypeChanged,
    BioSemiError_SpeedmodeChanged,
    BioSemiError_InvalidSpeedmode,

    BioSemiError_NoData,
    BioSemiError_NotEnoughDataInBuffer,
    BioSemiError_NoSync,
    BioSemiError_SyncLost,
    BioSemiError_BufferOverflow
};


class CBridgeBioSemiActiveTwo final
{
public:

    CBridgeBioSemiActiveTwo();
    virtual ~CBridgeBioSemiActiveTwo();

    bool isDeviceConnected();
    bool open();
    bool start();
    int read();
    bool discard();
    uint32_t getAvailableSampleCount();
    uint32_t getElectrodeChannelCount() const;
    uint32_t getEXChannelCount() const;
    uint32_t getSampleCount() const;
    uint32_t getSpeedMode() const { return m_speedMode; }
    size_t getChannelCount() const { return m_nChannel; }
    bool consumeOneSamplePerChannel(float* pSampleBuffer, uint32_t uiBufferValueCount);
    bool stop();
    bool close();

    bool isSynced() const { return m_bridgeSyncedWithDevice; }

    bool getTrigger(const uint32_t index) const { return (index > m_triggers.size() ? false : m_triggers[index]); }
    bool isCMSInRange() const { return m_cmsInRange; }
    bool isBatteryLow() const { return m_batteryLow; }
    bool isDeviceMarkII() const { return m_activeTwoMarkII; }
    uint32_t getSamplingFrequency() const;

    uint32_t getLastError() const { return m_lastError; }

    bool isUseEXChannels() const { return m_useEXChannels; }
    void setUseEXChannels(const bool useEXChannels) { m_useEXChannels = useEXChannels; }

protected:
    bool updateStatusFromValue(int value);
    uint32_t getAvailableByteCount() const;
    void consumeBytes(uint32_t count);

    // Handle to the device
    void* m_hDevice = nullptr;
    // Ring buffer
    std::vector<char> m_buffers;
    // 64bits buffer for USB_WRITE operations
    std::vector<char> m_controlBuffers;

    /* deduced from the SYNC channel on first read */
    bool m_firstRead              = false;
    bool m_bridgeSyncedWithDevice = false;
    uint32_t m_nChannel           = 0;
    uint32_t m_nInitialChannel    = 0;

    /* From Status channel */
    std::vector<bool> m_triggers;
    bool m_epochStarted           = false;
    bool m_cmsInRange             = false;
    bool m_batteryLow             = false;
    bool m_activeTwoMarkII        = false;
    bool m_initialActiveTwoMarkII = false;
    uint32_t m_speedMode          = 0;
    uint32_t m_initialSpeedmode   = 0;

    // ring buffer indices
#if TARGET_ARCHITECTURE_x64
    long long m_lastRingBufferByteIdx = 0;  //Buffer filled up to this point in last read()
#else
    uint32_t m_lastRingBufferByteIdx = 0;   //Buffer filled up to this point in last read()]
#endif
    uint32_t m_consumptionByteIdx = 0;      //Next consume() should start here, it should ALWAYS be a SYNC byte.

    // just for stats
    uint32_t m_nTotalByteRead = 0;
    uint32_t m_lastError      = 0;
    bool m_useEXChannels      = false;
};
}  // namespace OpenViBE


#endif // TARGET_HAS_ThirdPartyBioSemiAPI
