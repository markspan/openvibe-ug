#pragma once
#include "../../CIdentifier.hpp"

namespace OpenViBE {
namespace Kernel {
/**
 * \class CMessageClock
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-05-30
 * \brief Clock message
 * \ingroup Group_Player
 * \ingroup Group_Kernel
 * \todo This class can be suppress, The only time, this class is used is with processClock. In this function, only getTime() is used some times.
 * We must verify if this time is the same as getPlayerContext().getCurrentTime(). If it's true we can suppress completly this class.
 * remark : In this case we modify the API for plugin
 */
class OV_API CMessageClock
{
public:
    CMessageClock() = default;

    CIdentifier getIdentifier() const { return m_id; }
    uint64_t getTime() const { return m_time; }

    void setIdentifier(const CIdentifier& id) { m_id = id; }
    void setTime(const uint64_t time) { m_time = time; }
protected:
    CIdentifier m_id = CIdentifier::undefined();
    uint64_t m_time  = 0;
};

typedef CMessageClock IMessageClock;

}  // namespace Kernel
}  // namespace OpenViBE
