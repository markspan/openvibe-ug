///-------------------------------------------------------------------------------------------------
/// 
/// \file StreamBundle.h
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

#include <memory> // shared_ptr

#include <openvibe/ov_all.h>

#include "StreamBase.h"

#include "Contexted.h"

namespace OpenViBE {
namespace Tracker {
/// <summary> StreamBundle is a container of one or more typed streams. It corresponds to a 'track' in Tracker and can represent an .ov file. </summary>
///
/// StreamBundle can be queried for streams in time order: getNextStream() call can be used to find out the stream which has the earliest chunk position pointer(in time).
/// Stepping the stream may make some other stream to be returned on the next get call.
class StreamBundle final : protected Contexted
{
public:
    explicit StreamBundle(const Kernel::IKernelContext& ctx) : Contexted(ctx) {}

    // Copy everything, allocate new memory for content
    bool deepCopy(const StreamBundle& other);
    // Copy selected subset of streams from "other".
    // Other will retain ownership of any internal pointers.
    // Since the copy gets write access to content of other, "other" is not const.
    bool copyFrom(StreamBundle& other);

    bool initialize();
    bool uninitialize();

    // Rewind all streams
    bool rewind();

    // Returns the stream which has a position with the earliest beginning timestamp
    StreamPtr getNextStream(size_t& index);
    bool getNextStreamIndex(size_t& index) const;

    size_t getNumStreams() const { return m_streams.size(); }
    StreamPtrConst getStream(const size_t idx) const { return (idx < m_streams.size()) ? m_streams[idx] : nullptr; }
    StreamPtr getStream(const size_t idx) { return (idx < m_streams.size()) ? m_streams[idx] : nullptr; }
    std::vector<StreamPtr>& getAllStreams() { return m_streams; }

    // A factory method that creates a stream of a specific type into the slot index
    bool createStream(size_t index, const CIdentifier& typeID);
    bool deleteStream(size_t index);
    bool setStream(size_t index, const std::shared_ptr<StreamBase>& ptr);
    bool swapStreams(size_t idx1, size_t idx2);
    bool moveStream(size_t srcIdx, size_t dstIdx);

    // Returns the duration of the longest stream in the bundle
    CTime getMaxDuration() const;

    // Have all streams ended in the bundle?
    bool isFinished() const
    {
        size_t dummy;
        return !getNextStreamIndex(dummy);
    }

    // The name of the .ov file this bundle corresponds to
    const std::string& getSource() const { return m_source; }
    void setSource(const std::string& src) { m_source = src; }
    bool getDirtyBit() const { return m_dirty; }
    void setDirtyBit(const bool newState) { m_dirty = newState; }

protected:
    // As StreamBundles can consist of streams from diverse origins where some of them would
    // like to pass the ownership and some to keep it, instead of writing messy ownership tracking code, 
    // we use shared pointers for streams.
    std::vector<StreamPtr> m_streams;

    std::string m_source;   ///< Identifies the .ov file of the bundle on disk
    bool m_dirty = true;    ///< True if the stream has not been saved to disk after last modification
};
}  // namespace Tracker
}  // namespace OpenViBE
