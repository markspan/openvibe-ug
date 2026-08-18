/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

#include <AbstractDriver.h>
#include <AbstractDriverBuilder.h>

/**
 * @brief Auxiliary driver bridging OpenViBE::Services::TcpTaggingService into
 * the acquisition pipeline as stimulations.
 */
class TaggingDriver final : public AbstractDriver
{
public:
    TaggingDriver() {}

    bool init() override;
    bool unInit() override;

    bool start() override;
    bool stop() override;

    int read(sampleType* data, uint64_t size) override;
    int readStimulations(ovStimulation* buff, uint32_t maxCount) override;

    void setChunkSize(uint32_t chunkSize) override;
};

REGISTER_DRIVER(TaggingDriver, "Tagging Driver", true);
