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

#include "CDriverBioSemiActiveTwo.h"

#include "toolkit/ovtk_defines.h"
#include "toolkit/ovtk_stimulations.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {
/// Milliseconds between two repeats of the same device warning.
constexpr uint64_t WARNING_PERIOD_MS = 2000;
/// Milliseconds a recovered condition must hold before it is announced.
constexpr uint64_t RECOVERY_DEBOUNCE_MS = 100;
/// Milliseconds to wait after start() for the first packet, from which the bridge deduces the
/// channel count, the speed mode and the AD-box generation.
constexpr uint32_t FIRST_PACKET_WAIT_MS = 500;
}  // namespace

CDriverBioSemiActiveTwo::CDriverBioSemiActiveTwo()
    : m_paramUseEXChannels("UseEXChannels", "Acquire the 8 EX channels", true)
{
    // The amplifier can send up to 256+8 channels. As requested by BioSemi we expose the maximum
    // channel count: the user selects from 1 to MAX, and channels with no data in the current
    // speed mode are zero-filled.
    m_channelCount.setMax(int(BIOSEMI_ACTIVETWO_MAXCHANNELCOUNT));
    m_channelCount = int(BIOSEMI_ACTIVETWO_MAXCHANNELCOUNT);
    // AbstractDriver caps m_samplingRate at 10000 Hz, but speed modes 3 and 7 run at 16384 Hz.
    // Without widening the range first, init() would clamp and report the wrong rate.
    m_samplingRate.setMax(BIOSEMI_MAX_SAMPLING_RATE);
    m_samplingRate = 2048;

    m_parameters.push_back(&m_channelCount);
    m_parameters.push_back(&m_paramUseEXChannels);

    m_triggers.resize(16, false);
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBioSemiActiveTwo::init()
{
    m_acquisitionStopped = false;
    m_bridge.setUseEXChannels(m_paramUseEXChannels.value());

    if (!m_bridge.open())
    {
        std::cout << "[BioSemi] Could not open the device.\n";
        return false;
    }

    if (!m_bridge.start())
    {
        std::cout << "[BioSemi] Could not start the device.\n";
        m_bridge.close();
        return false;
    }

    // wait to be sure we get the first packet from which we deduce the actual channel count and other initial configuration.
    std::this_thread::sleep_for(std::chrono::milliseconds(FIRST_PACKET_WAIT_MS));

    if (m_bridge.read() < 0)
    {
        std::cout << "[BioSemi] An error occured while reading first data packet from device!\n";
        m_bridge.stop();
        m_bridge.close();
        return false;
    }

    if (!m_bridge.discard()) // we discard the samples.
    {
        std::cout << "[BioSemi] An error occured while dropping unused samples at initialization time.\n";
        m_bridge.stop();
        m_bridge.close();
        return false;
    }

    //Check speed mode: speed modes 1, 2 and 3 should not be used for acquisition
    if (m_bridge.getSpeedMode() < 4)
    {
        std::cout << "[BioSemi] Speed modes 1 to 3 are designed to realize daisy chained montages, they should not be used for acquisition.\n";
        m_bridge.stop();
        m_bridge.close();
        return false;
    }

    std::cout << "[BioSemi] Bridge initialized with: [SF:" << m_bridge.getSamplingFrequency()
              << "] [CH:" << m_bridge.getChannelCount()
              << "] [MKII:" << m_bridge.isDeviceMarkII()
              << "] [CMInRange:" << m_bridge.isCMSInRange()
              << "] [LowBat:" << m_bridge.isBatteryLow() << "]\n";

    if (m_bridge.isBatteryLow()) { std::cout << "[BioSemi] Device battery is low!\n"; }

    m_samplingRate.setValue(int(m_bridge.getSamplingFrequency()));

    // m_channelCount holds the EEG channel count the user asked for. Widen it to the real output
    // width for the duration of the acquisition; unInit() restores it. Growing it also resizes
    // m_channelNames, through the notifier AbstractDriver installs in its constructor.
    m_eegChannelCount           = uint32_t(m_channelCount.value());
    const uint32_t exChannels   = m_paramUseEXChannels.value() ? BIOSEMI_ACTIVETWO_EXCHANNELCOUNT : 0;
    m_channelCount.setValue(int(m_eegChannelCount + exChannels));

    // The device may carry fewer channels than requested in the current speed mode; the bridge
    // zero-fills the rest.
    if (m_eegChannelCount > m_bridge.getChannelCount())
    {
        std::cout << "[BioSemi] The required channel count cannot be reached in current device configuration (data stream contains "
                  << m_bridge.getChannelCount()
                  << " channels). Please check the device speed mode and setup capabilities. Channels with no data will be filled with zeros.\n";
    }

    // Only fill in the blanks, so that channels the user renamed keep their name.
    for (uint32_t i = 0; i < m_eegChannelCount; ++i) {
        if (m_channelNames.getAt(i).empty()) { m_channelNames.setAt(i, "Channel " + std::to_string(i + 1)); }
    }
    for (uint32_t i = 0; i < exChannels; ++i) {
        m_channelNames.setAt(m_eegChannelCount + i, "EX " + std::to_string(i + 1));
    }

    return true;
}

bool CDriverBioSemiActiveTwo::unInit()
{
    const bool stopped = m_bridge.stop();
    if (!stopped) { std::cout << "[BioSemi] Could not stop the device.\n"; }

    const bool closed = m_bridge.close();
    if (!closed) { std::cout << "[BioSemi] Could not close the device.\n"; }

    // Drop the EX channels from the channel count. This shrinks m_channelNames with it, so the EX
    // names are not persisted as if the user had chosen them.
    if (m_eegChannelCount > 0) { m_channelCount.setValue(int(m_eegChannelCount)); }

    return stopped && closed;
}

bool CDriverBioSemiActiveTwo::start()
{
    m_acquisitionStopped  = false;
    m_startTime           = timestamp() / 1'000'000ULL;
    m_lastWarningTime     = 0;
    m_lastCmsBackInRange  = 0;
    m_lastBatteryLow      = 0;
    m_bCMCurrentlyInRange = true;
    m_bBatteryCurrentlyOk = true;
    m_bCMSBackInRange     = true;
    m_bBatteryBackOk      = true;

    std::fill(m_triggers.begin(), m_triggers.end(), false);
    m_stimQueue.clear();

    // Drop whatever piled up in the ring buffer between init() and here.
    while (m_bridge.read() > 0) {
        if (!m_bridge.discard())
        {
            std::cout << "[BioSemi] An error occured while dropping samples.\n";
            m_acquisitionStopped = true;
            return false;
        }
    }

    return true;
}

bool CDriverBioSemiActiveTwo::stop()
{
    // Nothing device-side: Acquisition::stop() calls this before joining the reader thread, which
    // may still be inside read(). The bridge is torn down in unInit(), once that thread is gone.
    // m_acquisitionStopped is cleared by start(), not here, so a late read() stays inert.
    return true;
}

//___________________________________________________________________//
//                                                                   //

int CDriverBioSemiActiveTwo::read(sampleType* data, const uint64_t size)
{
    if (m_acquisitionStopped) { return 0; }

    const int byteRead = m_bridge.read();
    if (byteRead < 0)
    {
        std::cout << "[BioSemi] An error occured while reading data from device!\n";
        m_acquisitionStopped = true;
        // Deliberately not the negative error code AbstractDriver::read() documents:
        // AbstractDriver::getData() assigns the result into the uint64_t buff->count and then
        // writes timestampsBuff[count - 1], so a negative return is an out-of-bounds write.
        // Report no samples instead -- the failure is logged and acquisition is already flagged.
        return 0;
    }

    const uint32_t channels = uint32_t(m_channelCount.value());
    const uint64_t toRead   = std::min<uint64_t>(m_bridge.getAvailableSampleCount(), size);

    uint64_t written = 0;
    while (written < toRead)
    {
        // we consume one sample per channel, values are given in uV
        if (!m_bridge.consumeOneSamplePerChannel(data + written * channels, channels))
        {
            std::cout << "[BioSemi] Something bad happened while consuming samples from device.\n";
            if (m_bridge.getLastError() == BioSemiError_SyncLost)
            {
                std::cout << "[BioSemi] \tSynchronization lost during acquisition. Fiber optic may be loose or damaged.\n";
            }
            if (m_bridge.getLastError() == BioSemiError_BufferOverflow)
            {
                std::cout << "[BioSemi] \tBuffer overflow. Please check that you have enough CPU and memory available to run"
                             " the acquisition server at full speed before retrying.\n";
            }
            m_acquisitionStopped = true;
            break;
        }

        ++written;
        updateDeviceState();
    }

    return int(written);
}

int CDriverBioSemiActiveTwo::readStimulations(ovStimulation* buff, const uint32_t maxCount)
{
    int stimAdded = 0;
    while (!m_stimQueue.empty() && uint32_t(stimAdded) < maxCount)
    {
        buff[stimAdded] = m_stimQueue.front();
        m_stimQueue.pop_front();
        stimAdded++;
    }
    return stimAdded;
}

//___________________________________________________________________//
//                                                                   //

void CDriverBioSemiActiveTwo::updateDeviceState()
{
    const uint64_t now = timestamp() / 1'000'000ULL;

    // triggers:
    // we simply send OVTK_StimulationId_Label_X where X is the trigger index between 1 and 16
    // We don't handle rising and falling edges.
    for (uint32_t j = 0; j < m_triggers.size(); ++j) {
        if (m_bridge.getTrigger(j) != m_triggers[j])
        {
            m_triggers[j] = m_bridge.getTrigger(j);
            // date 0: AbstractDriver::getData() stamps it with the read time of this block.
            m_stimQueue.push_back(ovStimulation{ uint64_t(OVTK_StimulationId_Label(j + 1)), 0, 0 });
        }
    }

    /*
    From: Coen (BioSemi):

    The current flow via the DRL is constantly monitored by the safety circuitry inside the AD-box. The current flow is limited to 50 uA (IEC 601 safety limit).
    If the current runs into the limit, the CMS/DRL circuit cannot keep the Common Mode value within its normal working range, and the blue LED turns off.

    The safety circuitry reacts on this error situation by shutting the power supply to ALL active electrodes off. Consequently, no meaningful signals can be measured so long as the blue LED is off.
    The circuit operation described above implies that any electrode can be the cause of a CM out of range error.
    Examples of errors are broken wires, bad connector contacts (pollution of connector with gel), defect IC inside the electrode,
    bare electrode wire contacting the subject (damaged cable isolation) etc.. For example, if one of the active electrode wires is broken,
    the electrode input circuit is not anymore biased correctly, and the input resistance may fall below its specified value of 10^12 Ohm.
    The resultant extra input current is detected by the CMS/DRL circuit, and the blue LED goes off.
    Save operation of the system is ensured because the power supply to the active electrodes is only restored if ALL electrodes connected to the subject work correctly.
    In other words, both cap en EX electrodes can in principle cause CM out of range errors.
    */

    bool warningDisplayed = false;
    if (!m_bridge.isCMSInRange())
    {
        // we print a warning message once every 2secs maximum
        if (now > m_lastWarningTime + WARNING_PERIOD_MS)
        {
            warningDisplayed = true;
            std::cout << "[BioSemi] (" << ((now - m_startTime) / 1000)
                      << "') CMS/DRL is not in range. For safety purpose, any active electrode connected has been shut down"
                         " and signals should not be used.\n"
                         "[BioSemi] \tThe corresponding sample range has been tagged with OVTK_StimulationId_SegmentStart"
                         " and OVTK_StimulationId_SegmentStop.\n"
                         "[BioSemi] \tPossible causes include broken wires, damaged cable isolation, bad connector contacts,"
                         " defect IC inside the electrode.\n";
        }

        // the possibly incorrect sample range is tagged using OVTK_StimulationId_SegmentStart and OVTK_StimulationId_SegmentStop
        // CM is now not in range
        if (m_bCMCurrentlyInRange) { m_stimQueue.push_back(ovStimulation{ uint64_t(OVTK_StimulationId_SegmentStart), 0, 0 }); }
        m_bCMCurrentlyInRange = false;
        m_bCMSBackInRange     = false;
    }
    else
    {
        // CMS/DRL is now back in range
        if (!m_bCMCurrentlyInRange)
        {
            m_lastCmsBackInRange = now;
            m_stimQueue.push_back(ovStimulation{ uint64_t(OVTK_StimulationId_SegmentStop), 0, 0 });
            m_bCMCurrentlyInRange = true;
        }
            // CMS is considered "back in range" if it stayed in range for more than 100ms
            // -> avoids announcing a flapping contact on every sample
        else if (!m_bCMSBackInRange && (now > m_lastCmsBackInRange + RECOVERY_DEBOUNCE_MS))
        {
            m_bCMSBackInRange = true;
            std::cout << "[BioSemi] CMS/DRL is back in range.\n";
        }
    }

    if (m_bridge.isBatteryLow())
    {
        // we print a warning message once every 2secs maximum
        if (now > m_lastWarningTime + WARNING_PERIOD_MS)
        {
            warningDisplayed = true;
            std::cout << "[BioSemi] (" << ((now - m_startTime) / 1000) << "') Device battery is low!\n";
        }
        m_bBatteryCurrentlyOk = false;
        m_bBatteryBackOk      = false;
    }
    else
    {
        if (!m_bBatteryCurrentlyOk)
        {
            m_lastBatteryLow      = now;
            m_bBatteryCurrentlyOk = true;
        }
        else if (!m_bBatteryBackOk && (now > m_lastBatteryLow + RECOVERY_DEBOUNCE_MS))
        {
            m_bBatteryBackOk = true;
            std::cout << "[BioSemi] (" << ((now - m_startTime) / 1000) << "') Device battery seems ok.\n";
        }
    }

    if (warningDisplayed) { m_lastWarningTime = now; }
}
