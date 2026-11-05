/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "AbstractWriterBuilder.h"
#include <lsl_cpp.h>

#include <string>
#include <vector>

class LSLOutputWriter : public AbstractWriter
{
public:
    LSLOutputWriter(const std::string& writerName);
    ~LSLOutputWriter();

    // UG fork: gets the montage too, for the stream's metadata
    void onAcquisitionSetup(uint32_t sampleRate, const ovChannelInfo& channels) override;
    void uninit() override;

protected:
    size_t send(ovMsg* msg, uint64_t clientId = -1) override;

private:
    void destroyOutlets();

    OpenViBE::Parameters::ParameterString m_signalStreamName;
    OpenViBE::Parameters::ParameterString m_markerStreamName;

    double m_clockOffset = 0.0;         ///< UG fork: added to the server's clock in seconds, this gives LSL's clock.
    std::vector<double> m_timestamps;   ///< UG fork: the times of a chunk in LSL's clock; kept to avoid an allocation per chunk.

    lsl::stream_outlet* m_signalOutlet   = nullptr;
    lsl::stream_outlet* m_stimulusOutlet = nullptr;
};
