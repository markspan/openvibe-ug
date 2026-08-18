// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

#include <openvibe/acquisition-messaging/samples.h>

#include <deque>


/**
 * \class CDriverGenericOscillator
 * \author Thomas Prampat (INRIA)
 */
class CDriverGenericOscillator final : public AbstractDriver
{
public:
    CDriverGenericOscillator();
    void release();

    bool init() override;
    bool unInit() override;

    bool start() override;
    bool stop() override;

    int read(sampleType* data, uint64_t size) override;
    int readStimulations(ovStimulation* /*buff*/, uint32_t /*maxCount*/) override;

    bool supportsImpedanceCheck() const override { return true; }
    bool startImpedanceCheck() override;
    int  readImpedances(double* values, size_t count) override;
    bool stopImpedanceCheck() override { return true; }

    void setChunkSize(uint32_t chunkSize) override;

protected:
    uint32_t m_nSamplePerSentBlock = 0;

    uint32_t m_nTotalSample = 0;
    uint64_t m_nTotalStim   = 0;
    uint64_t m_startTime    = 0;

    std::deque<ovStimulation> m_stimQueue;

private:
    OpenViBE::Parameters::ParameterBool   m_paramSendPeriodicStimulations;
    OpenViBE::Parameters::ParameterDouble m_paramStimulationInterval;
    uint64_t m_impedanceStart = 0;
};


REGISTER_DRIVER(CDriverGenericOscillator, "Generic Oscillator", false);
