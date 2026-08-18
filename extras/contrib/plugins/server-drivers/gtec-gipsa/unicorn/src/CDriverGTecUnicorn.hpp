/**
 *  Software License Agreement (AGPL-3 License)
 *
 * \file CDriverGTecUnicorn.hpp
 * \author Anton Andreev, Gipsa-lab, VIBS team
 * \date 21/08/2020
 * \brief GTEC Unicorn Black Driver
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

#pragma once

#if defined TARGET_HAS_ThirdPartyGtecUnicron

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <Windows.h>

#include "ringbuffer.h"

#include <vector>

//threading
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory> // unique_ptr

#include <deque>

#include "unicorn.h"

namespace OpenViBE {
namespace AcquisitionServer {


#ifndef __GDEVICE_H__
#define __GDEVICE_H__
struct GDevice
{
    UNICORN_HANDLE handle;
    std::string serial;
};
#endif

class CDriverGTecUnicorn : public OpenViBE::AcquisitionServer::IDriver
{
public:
    CDriverGTecUnicorn(OpenViBE::AcquisitionServer::IDriverContext& rDriverContext);

    void release() { delete this; };
    const char* getName() override { return "g.tec Unicorn Gipsa-lab"; };

    bool initialize(const uint32_t sampleCountPerSentBlock, OpenViBE::AcquisitionServer::IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override  { return false; }
    bool configure() override { return true; };
    const OpenViBE::AcquisitionServer::IHeader* getHeader() override { return &m_header; }

    bool acquire();

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

protected:

    static const uint64_t kBufferSizeSeconds = 2;  // The size of the GTEC ring buffer in seconds
    static const uint32_t kGTecNumChannels = 17;  // The number of channels

    static const uint32_t kFrameLength = 8;  // The number of samples acquired per Get_Data() call and the number of samples supplied to OpenVibe (per loop)
    static const bool kTestSignalEnabled = FALSE;  // Flag to enable or disable testsignal.
    static const size_t kSelectedDevice = 0;  // If several Unicorn devices are in range, the first one will be automatically selected

    SettingsHelper m_settings;

    OpenViBE::AcquisitionServer::IDriverCallback* m_callback = nullptr;
    OpenViBE::AcquisitionServer::CHeader m_header;

    size_t m_sampleCountPerSentBlock = 0;

    // START declaration buffers
    float* m_bufferRawUnicornDevice = nullptr;  // buffer 1 : data from device to ring buffer

    float* m_bufferReceivedDataFromRing = nullptr;  // buffer 2 : data from ring buffer

    float* m_bufferForOpenVibe = nullptr;  // buffer 3 : data converted to OpenVibe format
    // END declaration buffers

    uint32_t m_acquiredChannelCount = kGTecNumChannels;  //number of channels specified by the user, never counts the event channels

    size_t m_totalHardwareStimulations = 0;  //since start button clicked
    size_t m_totalRingBufferOverruns = 0;
    size_t m_totalCounterErrors = 0;

    bool m_flagIsFirstLoop = true;
    bool m_bufferOverrun = false;

    // ring buffer provided by Guger
    CRingBuffer<float> m_ringBuffer;

    std::unique_ptr<std::thread> m_thread;
    bool m_isThreadRunning = false;
    std::mutex m_mutex;
    std::condition_variable  m_itemAvailable;

    // List of amplifiers
    std::vector<GDevice> m_devices;

    uint32_t m_lengthBufferRawUnicornDevice = 0;

    size_t m_channelCounterIndex = 0;


    bool configureDevice(size_t deviceNumber);
    void detectDevices();
    size_t numDevices() const { return m_devices.size(); };
    size_t getChannelIndex(UNICORN_HANDLE hDevice, const char *name);

};

}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGtecUnicron
