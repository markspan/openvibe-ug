#include "ovkCLink.h"
#include "ovkCScenario.h"

#include "../ovkCObjectVisitorContext.h"

namespace OpenViBE {
namespace Kernel {

bool CLink::initializeFromExistingLink(const ILink& link)
{
    m_id             = link.getIdentifier();
    m_srcBoxID       = link.getSourceBoxIdentifier();
    m_dstBoxID       = link.getTargetBoxIdentifier();
    m_srcBoxOutputID = link.getSourceBoxOutputIdentifier();
    m_dstBoxInputID  = link.getTargetBoxInputIdentifier();
    m_srcOutputIdx   = link.getSourceBoxOutputIndex();
    m_dstInputIdx    = link.getTargetBoxInputIndex();
    return true;
}


//___________________________________________________________________//
//                                                                   //

bool CLink::setIdentifier(const CIdentifier& identifier)
{
    m_id = identifier;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CLink::setSource(const CIdentifier& boxId, const size_t boxOutputIdx, const CIdentifier boxOutputID)
{
    m_srcBoxID       = boxId;
    m_srcOutputIdx   = boxOutputIdx;
    m_srcBoxOutputID = boxOutputID;
    return true;
}

bool CLink::setTarget(const CIdentifier& boxId, const size_t boxInputIdx, const CIdentifier boxInputID)
{
    m_dstBoxID      = boxId;
    m_dstInputIdx   = boxInputIdx;
    m_dstBoxInputID = boxInputID;
    return true;
}

bool CLink::getSource(CIdentifier& boxId, size_t& boxOutputIdx, CIdentifier& boxOutputID) const
{
    boxId        = m_srcBoxID;
    boxOutputIdx = m_srcOutputIdx;
    boxOutputID  = m_srcBoxOutputID;
    return true;
}

bool CLink::getTarget(CIdentifier& dstBoxID, size_t& boxInputIndex, CIdentifier& dstBoxInputID) const
{
    dstBoxID      = m_dstBoxID;
    boxInputIndex = m_dstInputIdx;
    dstBoxInputID = m_dstBoxInputID;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CLink::acceptVisitor(IObjectVisitor& visitor)
{
    CObjectVisitorContext context(getKernelContext());
    return visitor.processBegin(context, *this) && visitor.processEnd(context, *this);
}

}  // namespace Kernel
}  // namespace OpenViBE
