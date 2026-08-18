/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#include "TaggingDriver.h"

#include <TcpTaggingService.h>

bool TaggingDriver::init()
{
    return true;
}

bool TaggingDriver::unInit()
{
    return true;
}

bool TaggingDriver::start()
{
    // Discard tags received before this acquisition started polling.
    OpenViBE::Services::TcpTaggingService::instance().clearQueue();
    return true;
}

bool TaggingDriver::stop()
{
    return true;
}

int TaggingDriver::read(sampleType* /*data*/, uint64_t /*size*/)
{
    return 0; // This driver produces no channel samples.
}

int TaggingDriver::readStimulations(ovStimulation* buff, uint32_t maxCount)
{
    uint32_t count = 0;
    OpenViBE::Services::TcpTagging::Tag tag;
    while (count < maxCount && OpenViBE::Services::TcpTaggingService::instance().popTag(tag)) {
        // date left at 0: auto-stamped by AbstractDriver::getData() using the
        // driver's own clock domain (see plan notes on clock-domain mismatch).
        buff[count] = ovStimulation{tag.identifier, 0, 0};
        ++count;
    }
    return int(count);
}

void TaggingDriver::setChunkSize(uint32_t /*chunkSize*/)
{
}
