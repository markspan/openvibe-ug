//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <iostream>
#include <algorithm>

#include "StreamBundle.h"

#include "Stream.h"

#include "StreamFactory.h"

namespace OpenViBE {
namespace Tracker {

bool StreamBundle::deepCopy(const StreamBundle& other)
{
    bool retVal = true;

    // Clear streams
    initialize();

    // Copy each stream
    for (size_t i = 0; i < other.getNumStreams(); ++i) {
        retVal &= createStream(i, other.getStream(i)->getTypeIdentifier());
        retVal &= getStream(i)->copy(*other.getStream(i));
    }

    m_source = other.getSource();
    setDirtyBit(other.getDirtyBit());

    return retVal;
}

bool StreamBundle::copyFrom(StreamBundle& other)
{
    initialize();

    for (const auto& str : other.getAllStreams()) { if (str->getSelected()) { this->setStream(m_streams.size(), str); } }

    setSource(other.getSource());
    setDirtyBit(other.getDirtyBit());

    // n.b. this will affect the streams we copied
    rewind();

    return true;
}

bool StreamBundle::initialize()
{
    //reset first
    uninitialize();
    m_dirty = true;
    //  log() << Kernel::LogLevel_Debug << "Streams initialized ok\n";
    return true;
}

bool StreamBundle::uninitialize()
{
    // Since m_Streams are shared pointers, no need to delete them
    m_streams.clear();
    return true;
}

bool StreamBundle::rewind()
{
    bool returnValue = true;
    std::for_each(m_streams.begin(), m_streams.end(), [&returnValue](const StreamPtr& entry) { if (entry) { returnValue &= entry->reset(); } });
    return returnValue;
}

bool StreamBundle::createStream(const size_t index, const CIdentifier& typeID)
{
    if (index >= m_streams.size()) { m_streams.resize(index + 1, nullptr); }

    if (m_streams[index] == nullptr) {
        const StreamPtr stream = StreamFactory::getStream(m_kernelCtx, typeID);
        if (!stream) { return false; }
        m_streams[index] = stream;
        setDirtyBit(true);
        return true;
    }

    log() << Kernel::LogLevel_Error << "Error: Slot " << index << " is already used\n";

    return false;
}

bool StreamBundle::deleteStream(const size_t index)
{
    if (index > m_streams.size()) {
        log() << Kernel::LogLevel_Error << "Error: Stream index exceeds array size\n";
        return false;
    }

    // m_Streams is shared ptrs, no need to delete
    m_streams.erase(m_streams.begin() + index);
    setDirtyBit(true);
    return true;
}

bool StreamBundle::getNextStreamIndex(size_t& index) const
{
    if (m_streams.empty()) { return false; }

    // Find the stream with the earliest chunk, return the stream
    CTime earliestTime  = CTime::max();
    bool foundSomething = false;

    for (size_t i = 0; i < m_streams.size(); ++i) {
        const StreamPtr ptr = m_streams[i];

        CTime startTime = CTime::min(), endTime = CTime::min();
        if (ptr && ptr->peek(startTime, endTime) && startTime < earliestTime) {
            earliestTime   = startTime;
            index          = int(i);
            foundSomething = true;
        }
    }

    if (!foundSomething) {
        //log() << Kernel::LogLevel_Info << "All streams exhausted\n";
        return false;
    }
    return true;
}

StreamPtr StreamBundle::getNextStream(size_t& index)
{
    index = -1;
    if (getNextStreamIndex(index)) { return m_streams[index]; }
    return nullptr;
}

CTime StreamBundle::getMaxDuration() const
{
    CTime maxDuration = CTime::min();
    for (size_t i = 0; i < m_streams.size(); ++i) {
        if (m_streams[i]) {
            CTime streamDuration = m_streams[i]->getDuration();
            maxDuration          = std::max<CTime>(maxDuration, streamDuration);
        }
    }
    return maxDuration;
}


bool StreamBundle::setStream(const size_t index, const std::shared_ptr<StreamBase>& ptr)
{
    if (index >= m_streams.size()) { m_streams.resize(index + 1, nullptr); }
    m_streams[index] = ptr;
    setDirtyBit(true);
    return true;
}

bool StreamBundle::swapStreams(const size_t idx1, const size_t idx2)
{
    if (idx1 >= m_streams.size() || idx2 >= m_streams.size()) { return false; }

    const auto it1 = m_streams.begin() + idx1;
    const auto it2 = m_streams.begin() + idx2;

    std::iter_swap(it1, it2);

    setDirtyBit(true);

    return true;
}

bool StreamBundle::moveStream(const size_t srcIdx, const size_t dstIdx)
{
    if (srcIdx >= getNumStreams() || dstIdx >= getNumStreams()) { return false; }
    if (srcIdx == dstIdx) { return true; }

    const auto oldPtr = m_streams[srcIdx];
    m_streams.erase(m_streams.begin() + srcIdx);
    m_streams.insert(m_streams.begin() + dstIdx, oldPtr);

    setDirtyBit(true);

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
