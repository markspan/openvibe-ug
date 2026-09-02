// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "CBridgeBioSemiActiveThree.h"

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

#include <openvibe/acquisition-messaging/samples.h>
#include <openvibe/parameters/parameterBool.h>

#include <deque>
#include <vector>

/**
 * \class CDriverBioSemiActiveThree
 *
 * Ported from the legacy ovasIDriver/GTK driver. The GTK configuration dialog became two
 * parameters, and the GTK device-status window became console warnings plus the stimulation
 * tagging that already existed.
 */
class CDriverBioSemiActiveThree final : public AbstractDriver
{
public:
    CDriverBioSemiActiveThree();

    bool init() override;
    bool unInit() override;

    bool start() override;
    bool stop() override;

    int read(sampleType* data, uint64_t size) override;
    int readStimulations(ovStimulation* buff, uint32_t maxCount) override;

    void setChunkSize(uint32_t chunkSize) override { m_nSamplePerSentBlock = chunkSize; }

private:
    /// Turns the per-sample device state (triggers, CMS/DRL, battery) into stimulations and warnings.
    void updateDeviceState();

    CBridgeBioSemiActiveThree m_bridge;

    OpenViBE::Parameters::ParameterBool m_paramUseEXChannels;

    uint32_t m_nSamplePerSentBlock = 0;
    /// Channel count the user asked for, i.e. EEG channels only. m_channelCount grows by the EX
    /// channels for the duration of an acquisition and is restored from this in unInit().
    uint32_t m_eegChannelCount = 0;

    std::deque<ovStimulation> m_stimQueue;
    std::vector<bool> m_triggers;

    bool m_acquisitionStopped = false;

    // Device-state tracking, all times in milliseconds.
    uint64_t m_startTime          = 0;
    uint64_t m_lastWarningTime    = 0;
    uint64_t m_lastCmsBackInRange = 0;
    uint64_t m_lastBatteryLow     = 0;
    bool m_bCMCurrentlyInRange    = true;
    bool m_bBatteryCurrentlyOk    = true;
    // True if CMS is in range for more than 100ms
    bool m_bCMSBackInRange = true;
    bool m_bBatteryBackOk  = true;
};


REGISTER_DRIVER(CDriverBioSemiActiveThree, "BioSemi Active Three", false);
