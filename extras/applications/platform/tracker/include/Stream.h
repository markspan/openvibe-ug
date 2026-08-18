///-------------------------------------------------------------------------------------------------
/// 
/// \file Stream.h
/// \author J. T. Lindgren / Inria.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
///
///-------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include <ebml/IWriterHelper.h>
#include <ebml/IWriter.h>
#include <ebml/TWriterCallbackProxy.h>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "StreamBase.h"

namespace OpenViBE {
namespace Tracker {
///<summary> A container class representing a stream of OpenViBE. </summary>
///
/// Stream is basically a specific kind of time-ordered container of timestamped, typed elements.
///
/// Unlike OpenViBE streams in general, the Stream content in Tracker is stored in memory unencoded, 
/// and hence if its content is written to an .ov file, passed to a box, or Designer, it must be encoded first.
///
/// Stream must have a 'header' representing the stream parameters, a sequence of 'buffers'
/// containing the data, and an 'end' indicating the end of the stream.
///
///         Time ----------------------->
///            0    
/// "front" [header][chk1][chk2]....[end] "tail"
///
/// The stream additionally has a 'position' that always points to one of the above elements and starts from front.
/// Stepping the stream moves the stream position +1 chunk forward towards the tail.
///
/// The stream API can be used in a way that chunks can be pushed to the tail while the position counter is advancing from the head towards the tail.
/// Although this would allow interleaving pushes and reads, currently the Tracker works in a way that when a track is loaded or recorded,
/// only push() operations are done until the finish.
/// On the other hand, when the track is sent out, only peek() and step() are used to retrieve the chunks.
///
/// Currently Tracker Plugins are allowed to access the tracks in a random access fashion, as well as the GUI.
template <class T>
class Stream final : public StreamBase
{
public:
    explicit Stream(const Kernel::IKernelContext& ctx) : StreamBase(ctx) { m_end.m_StartTime = m_end.m_EndTime = CTime::max(); }

    ~Stream() override
    {
        /* if(m_Header) { delete m_Header; } */
        clear();
    }

    CIdentifier getTypeIdentifier() const override { return T::getTypeIdentifier(); }

    // Note: There is no corresponding setters, use the non-const versions to modify the header
    const typename T::Header& getHeader() const { return m_header; }
    const typename T::End& getEnd() const { return m_end; }
    typename T::Header& getHeader() { return m_header; }
    typename T::End& getEnd() { return m_end; }

    bool push(typename T::Buffer* chunk)
    {
        m_chunks.push_back(chunk);
        return true;
    }

    // Return the timestamps of the current chunk
    bool peek(CTime& startTime, CTime& endTime) const override { return peek(m_position, startTime, endTime); }

    // Return timestamps of a specific chunk
    bool peek(size_t index, CTime& startTime, CTime& endTime) const override
    {
        if (index == size_t(-1)) { startTime = endTime = 0; }
        else if (index < m_chunks.size()) {
            startTime = m_chunks[index]->m_StartTime;
            endTime   = m_chunks[index]->m_EndTime;
        }
        else if (index == m_chunks.size() && m_chunks.size() > 0) {
            startTime = m_chunks[index - 1]->m_EndTime;
            endTime   = m_chunks[index - 1]->m_EndTime;
        }
        else { return false; }
        return true;
    }

    size_t getChunkCount() const override { return m_chunks.size(); }

    bool getChunk(size_t idx, typename T::Buffer** ptr) const
    {
        if (idx < m_chunks.size()) {
            *ptr = m_chunks[idx];
            return true;
        }
        *ptr = nullptr;
        return false;
    }

    const typename T::Buffer* getChunk(size_t idx) const
    {
        if (idx < m_chunks.size()) { return m_chunks[idx]; }
        return nullptr;
    }

    bool clear() override
    {
        std::for_each(m_chunks.begin(), m_chunks.end(), [](typename T::Buffer* ptr) { delete ptr; });
        m_chunks.clear();
        m_position = size_t(-1);
        return true;
    }

    CTime getDuration() const override
    {
        const size_t chunkCount = getChunkCount();
        if (chunkCount == 0) { return CTime::min(); }
        return m_chunks[chunkCount - 1]->m_EndTime;
    }

    CTime getStartTime() const override
    {
        const size_t chunkCount = getChunkCount();
        if (chunkCount == 0) { return CTime::min(); }
        return m_chunks[0]->m_StartTime;
    }

    // @fixme efficiency
    bool getOverlapping() const override
    {
        for (size_t i = 1; i < m_chunks.size(); ++i) { if (m_chunks[i]->m_StartTime < m_chunks[i - 1]->m_EndTime) { return true; } }
        return false;
    }

    // @fixme efficiency
    bool getNoncontinuous() const override
    {
        for (size_t i = 1; i < m_chunks.size(); ++i) { if (m_chunks[i]->m_StartTime > m_chunks[i - 1]->m_EndTime) { return true; } }
        return false;
    }

    // @fixme efficiency
    uint64_t countChunks(CTime startTime, CTime endTime) const
    {
        // Count the chunks @fixme not very efficient
        uint64_t chunkCount = 0;
        for (size_t i = 0; i < m_chunks.size(); ++i) {
            if (m_chunks[i]->m_StartTime >= startTime) {
                if (m_chunks[i]->m_EndTime <= endTime) { chunkCount++; }
                else { break; }
            }
        }
        return chunkCount;
    }

    // Iterators and operators
    typename std::vector<typename T::Buffer*>::iterator begin() { return m_chunks.begin(); }
    typename std::vector<typename T::Buffer*>::iterator end() { return m_chunks.end(); }

    typename T::Buffer& operator[](int idx) { return m_chunks[idx]; }

    // @fixme
    // bool setHeader(TypeError::Header *) { return true; };
    // bool setBuffer(TypeError::Buffer *) { return true; };

    // @fixme doesn't work yet
    bool copy(const StreamBase& other) override
    {
        if (T::getTypeIdentifier() != other.getTypeIdentifier()) { return false; }

        const Stream<T>& otherStream = reinterpret_cast<const Stream<T>&>(other);

        m_chunks.resize(otherStream.getChunkCount(), nullptr);

        // @fixme this can be implemented when CMatrix has a working copy/assignment operators
        /*
        m_Header = otherStream.getHeader();

        for (size_t c = 0; c < otherStream.getChunkCount(); ++c)
        {
            m_Chunks[c] = new T::Buffer(otherStream.getChunk(c));
        }

        m_End = otherStream.getEnd();
        */

        log() << Kernel::LogLevel_Error << "Unimplemented method\n";

        return false;
    }

protected:
    typename T::Header m_header;                // Header of a stream
    std::vector<typename T::Buffer*> m_chunks;  // Buffers
    typename T::End m_end;                      // End of a stream
};
}  // namespace Tracker
}  // namespace OpenViBE
