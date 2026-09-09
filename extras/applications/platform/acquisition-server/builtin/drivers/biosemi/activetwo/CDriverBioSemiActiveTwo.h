/**-------------------------------------------------------------------------------------------------
 * \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 *-------------------------------------------------------------------------------------------------
 */

#pragma once

#include "CBridgeBioSemiActiveTwo.h"

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

#include <openvibe/acquisition-messaging/samples.h>
#include <openvibe/parameters/parameterBool.h>

#include <deque>
#include <vector>

/**
 * \class CDriverBioSemiActiveTwo
 * \author Mensia Technologies
 *
 * Ported from the legacy ovasIDriver/GTK driver. The GTK configuration dialog became two
 * parameters, and the GTK device-status window became console warnings plus the stimulation
 * tagging that already existed.
 */
class CDriverBioSemiActiveTwo final : public AbstractDriver
{
public:
    CDriverBioSemiActiveTwo();

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

    CBridgeBioSemiActiveTwo m_bridge;

    OpenViBE::Parameters::ParameterBool m_paramUseEXChannels;

    uint32_t m_nSamplePerSentBlock = 0;
    /**
     * Channel count the user asked for, i.e. EEG channels only. m_channelCount grows by the EX
     * channels for the duration of an acquisition and is restored from this in unInit().
     */
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


REGISTER_DRIVER(CDriverBioSemiActiveTwo, "BioSemi Active Two (MkI and MkII)", false);
