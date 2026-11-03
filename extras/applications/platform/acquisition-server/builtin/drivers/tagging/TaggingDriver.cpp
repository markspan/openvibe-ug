/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#include "TaggingDriver.h"
#include "TagDate.h"

#include <TcpTaggingService.h>
#include <system/ovCTime.h>

#include <algorithm>

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

    // UG fork: a tag is dated at the time it carries, which is when the stimulus was shown, not when it is read here.
    // Upstream leaves the date at 0, for AbstractDriver::getData() to date it now.
    const uint64_t now     = System::Time::zgetTimeRaw(false);
    const timestampType nowNs = timestamp();
    while (count < maxCount && OpenViBE::Services::TcpTaggingService::instance().popTag(tag)) {
        buff[count] = ovStimulation{tag.identifier, tagDate(tag.timestamp, now, nowNs), 0};
        ++count;
    }

    // The writer expects them in date order, and several programs can send tags
    std::stable_sort(buff, buff + count, [](const ovStimulation& a, const ovStimulation& b) { return a.date < b.date; });
    return int(count);
}

void TaggingDriver::setChunkSize(uint32_t /*chunkSize*/)
{
}
