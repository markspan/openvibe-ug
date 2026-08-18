#pragma once

#include <string>
#include <vector>
#include <memory> // std::shared_ptr

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "Contexted.h"

// #include "ovkTAttributable.h"
// class StreamBase : public OpenViBE::Kernel::TAttributable<Contexted> {

namespace OpenViBE {
namespace Tracker {


/**
 * \class StreamBase 
 * \brief Abstract, non-typed base class for Streams. 
 * \details For details, please see the type-specific derived class Stream<T>.
 * \author J. T. Lindgren
 *
 */
class StreamBase : protected Contexted
{
public:
    explicit StreamBase(const Kernel::IKernelContext& ctx) : Contexted(ctx) { }
    ~StreamBase() override { }

    virtual CIdentifier getTypeIdentifier() const = 0;

    virtual bool peek(CTime& startTime, CTime& endTime) const = 0;
    virtual bool peek(size_t index, CTime& startTime, CTime& endTime) const = 0;

    virtual bool step()
    {
        if (m_position <= getChunkCount() || m_position == size_t(-1)) {
            m_position++;
            return true;
        }
        return false;
    }

    virtual bool reset()
    {
        m_position = size_t(-1);
        return true;
    }

    virtual bool clear() = 0;

    virtual size_t getChunkCount() const = 0;

    virtual CTime getDuration() const = 0;
    virtual CTime getStartTime() const = 0;

    // Current play position
    size_t getPosition() const { return m_position; }

    bool setPosition(const size_t position)
    {
        m_position = position;
        return true;
    }

    // Is the stream currently selected? (nb. this is not in the .ov file)
    bool getSelected() const { return m_selected; }

    bool setSelected(const bool newState)
    {
        m_selected = newState;
        return true;
    }

    // Stream characteristics
    virtual bool getOverlapping() const = 0;
    virtual bool getNoncontinuous() const = 0;

    virtual bool copy(const StreamBase& /*other*/) { return false; }

protected:
    size_t m_position = size_t(-1);  // -1 == beginning of the stream, header

    // Is the stream currently selected?
    bool m_selected = true;
};

typedef std::shared_ptr<StreamBase> StreamPtr;
typedef std::shared_ptr<const StreamBase> StreamPtrConst;
}  // namespace Tracker
}  // namespace OpenViBE
