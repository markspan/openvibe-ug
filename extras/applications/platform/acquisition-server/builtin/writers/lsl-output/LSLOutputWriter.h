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

    void onAcquisitionSetup(uint32_t sampleRate, const std::vector<std::string>& channelNames) override;
    void uninit() override;

protected:
    size_t send(ovMsg* msg, uint64_t clientId = -1) override;

private:
    void destroyOutlets();

    OpenViBE::Parameters::ParameterString m_markerStreamName;
    OpenViBE::Parameters::ParameterString m_signalStreamName;
    
    std::vector<std::string> m_channelNames;
    uint32_t m_sampleRate = 0;

    std::string m_signalStreamID;
    std::string m_markerStreamID;

    lsl::stream_outlet* m_signalOutlet   = nullptr;
    lsl::stream_outlet* m_stimulusOutlet = nullptr;
};

REGISTER_WRITER(LSLOutputWriter, "LSL Output");
