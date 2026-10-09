// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CDriverBioSemiActiveThree.h"

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
/**
 * Milliseconds to wait after start() for the first packet, from which the bridge deduces the
 * channel counts.
 */
constexpr uint32_t FIRST_PACKET_WAIT_MS = 500;
}  // namespace

CDriverBioSemiActiveThree::CDriverBioSemiActiveThree()
    : m_paramUseEXChannels("UseEXChannels", "Acquire the 8 EX channels", true)
{
    m_channelCount.setMax(int(CBridgeBioSemiActiveThree::MaxEEGChannelCount));
    m_channelCount = int(CBridgeBioSemiActiveThree::MaxEEGChannelCount);
    // AbstractDriver caps m_samplingRate at 10000 Hz, below what this AD-box runs at. Without
    // widening the range first, setValue() clamps and the server is told the wrong rate.
    m_samplingRate.setMax(BIOSEMI_MAX_SAMPLING_RATE);
    m_samplingRate = BIOSEMI_MAX_SAMPLING_RATE;

    m_parameters.push_back(&m_channelCount);
    m_parameters.push_back(&m_paramUseEXChannels);

    m_triggers.resize(16, false);
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBioSemiActiveThree::init()
{
    m_acquisitionStopped = false;
    m_bridge.setUseEXChannels(m_paramUseEXChannels.value());

    if (!m_bridge.open())
    {
        std::cout << "[BioSemi] Could not open the device." << std::endl;
        return false;
    }

    if (!m_bridge.start())
    {
        std::cout << "[BioSemi] Could not start the device." << std::endl;
        m_bridge.close();
        return false;
    }

    // wait to be sure we get the first packet from which we deduce the actual channel count and other initial configuration.
    std::this_thread::sleep_for(std::chrono::milliseconds(FIRST_PACKET_WAIT_MS));

    const int byteRead = m_bridge.read();
    if (byteRead < 0)
    {
        std::cout << "[BioSemi] Failed to connect: an error occured while reading from the device!" << std::endl;
        m_bridge.stop();
        m_bridge.close();
        return false;
    }
    if (byteRead == 0)
    {
        std::cout << "[BioSemi] Failed to connect: no data received from the device!" << std::endl;
        m_bridge.stop();
        m_bridge.close();
        return false;
    }

    if (!m_bridge.discard()) // we discard the samples.
    {
        std::cout << "[BioSemi] An error occured while dropping unused samples at initialization time." << std::endl;
        m_bridge.stop();
        m_bridge.close();
        return false;
    }

    std::cout << "[BioSemi] Bridge initialized with: [SF:" << m_bridge.getSamplingFrequency()
              << "] [CH:" << m_bridge.getChannelCount()
              << "] [CMInRange:" << m_bridge.isCMSInRange()
              << "] [LowBat:" << m_bridge.isBatteryLow() << "]" << std::endl;

    if (m_bridge.isBatteryLow()) { std::cout << "[BioSemi] Device battery is low!" << std::endl; }

    m_samplingRate.setValue(int(m_bridge.getSamplingFrequency()));

    // Unlike the ActiveTwo bridge, this one does not zero-pad past the channels the AD-box carries
    // -- it would consume a negative number of bytes off the ring buffer. Clamp the request to what
    // the device actually reports instead.
    m_eegChannelCount = uint32_t(m_channelCount.value());
    if (m_eegChannelCount > m_bridge.getEEGChannelCount())
    {
        std::cout << "[BioSemi] The required channel count cannot be reached in current device configuration (data stream contains "
                  << m_bridge.getEEGChannelCount() << " EEG channels). Acquiring " << m_bridge.getEEGChannelCount() << " channels." << std::endl;
        m_eegChannelCount = m_bridge.getEEGChannelCount();
    }
    m_bridge.setEffectiveEEGChannelCount(m_eegChannelCount);

    // m_channelCount holds the EEG channel count the user asked for. Widen it to the real output
    // width for the duration of the acquisition; unInit() restores it. Growing it also resizes
    // m_channelNames, through the notifier AbstractDriver installs in its constructor.
    const uint32_t exChannels = m_paramUseEXChannels.value() ? CBridgeBioSemiActiveThree::MaxEXChannelCount : 0;
    m_channelCount.setValue(int(m_eegChannelCount + exChannels));

    // Only fill in the blanks, so that channels the user renamed keep their name.
    for (uint32_t i = 0; i < m_eegChannelCount; ++i)
    {
        if (m_channelNames.getAt(i).empty()) { m_channelNames.setAt(i, "Channel " + std::to_string(i + 1)); }
    }
    for (uint32_t i = 0; i < exChannels; ++i)
    {
        m_channelNames.setAt(m_eegChannelCount + i, "EX " + std::to_string(i + 1));
    }

    return true;
}

bool CDriverBioSemiActiveThree::unInit()
{
    const bool stopped = m_bridge.stop();
    if (!stopped) { std::cout << "[BioSemi] Could not stop the device." << std::endl; }

    const bool closed = m_bridge.close();
    if (!closed) { std::cout << "[BioSemi] Could not close the device." << std::endl; }

    // Drop the EX channels from the channel count. This shrinks m_channelNames with it, so the EX
    // names are not persisted as if the user had chosen them.
    if (m_eegChannelCount > 0) { m_channelCount.setValue(int(m_eegChannelCount)); }

    return stopped && closed;
}

bool CDriverBioSemiActiveThree::start()
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
    while (m_bridge.read() > 0)
    {
        if (!m_bridge.discard())
        {
            std::cout << "[BioSemi] An error occured while dropping samples." << std::endl;
            m_acquisitionStopped = true;
            return false;
        }
    }

    return true;
}

bool CDriverBioSemiActiveThree::stop()
{
    // Nothing device-side: Acquisition::stop() calls this before joining the reader thread, which
    // may still be inside read(). The bridge is torn down in unInit(), once that thread is gone.
    // m_acquisitionStopped is cleared by start(), not here, so a late read() stays inert.
    return true;
}

//___________________________________________________________________//
//                                                                   //

int CDriverBioSemiActiveThree::read(sampleType* data, const uint64_t size)
{
    if (m_acquisitionStopped) { return 0; }

    const int byteRead = m_bridge.read();
    if (byteRead < 0)
    {
        std::cout << "[BioSemi] An error occured while reading data from device!" << std::endl;
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
        if (!m_bridge.consumeOnePacket(data + written * channels))
        {
            std::cout << "[BioSemi] Something bad happened while consuming samples from device." << std::endl;
            if (m_bridge.getLastError() == BioSemiError_SyncLost)
            {
                std::cout << "[BioSemi] \tSynchronization lost during acquisition. Fiber optic may be loose or damaged." << std::endl;
            }
            if (m_bridge.getLastError() == BioSemiError_BufferOverflow)
            {
                std::cout << "[BioSemi] \tBuffer overflow. Please check that you have enough CPU and memory available to run"
                             " the acquisition server at full speed before retrying." << std::endl;
            }
            m_acquisitionStopped = true;
            break;
        }

        ++written;
        updateDeviceState();
    }

    return int(written);
}

int CDriverBioSemiActiveThree::readStimulations(ovStimulation* buff, const uint32_t maxCount)
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

void CDriverBioSemiActiveThree::updateDeviceState()
{
    const uint64_t now = timestamp() / 1'000'000ULL;

    // triggers:
    // we simply send OVTK_StimulationId_Label_X where X is the trigger index between 1 and 16
    // We don't handle rising and falling edges.
    for (uint32_t j = 0; j < m_triggers.size(); ++j)
    {
        if (m_bridge.getTrigger(j) != m_triggers[j])
        {
            m_triggers[j] = m_bridge.getTrigger(j);
            // date 0: AbstractDriver::getData() stamps it with the read time of this block.
            m_stimQueue.push_back(ovStimulation{ uint64_t(OVTK_StimulationId_Label(j + 1)), 0, 0 });
        }
    }

    // See CDriverBioSemiActiveTwo::updateDeviceState() for BioSemi's explanation of what a
    // CMS/DRL out-of-range condition means and what causes it.
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
                         " defect IC inside the electrode." << std::endl;
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
            std::cout << "[BioSemi] CMS/DRL is back in range." << std::endl;
        }
    }

    if (m_bridge.isBatteryLow())
    {
        // we print a warning message once every 2secs maximum
        if (now > m_lastWarningTime + WARNING_PERIOD_MS)
        {
            warningDisplayed = true;
            std::cout << "[BioSemi] (" << ((now - m_startTime) / 1000) << "') Device battery is low!" << std::endl;
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
            std::cout << "[BioSemi] (" << ((now - m_startTime) / 1000) << "') Device battery seems ok." << std::endl;
        }
    }

    if (warningDisplayed) { m_lastWarningTime = now; }
}
