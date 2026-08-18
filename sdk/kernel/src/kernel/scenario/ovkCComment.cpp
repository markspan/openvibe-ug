#include "ovkCComment.h"
#include "ovkCScenario.h"

#include "../ovkCObjectVisitorContext.h"

namespace OpenViBE {
namespace Kernel {

CComment::CComment(const IKernelContext& ctx, CScenario& rOwnerScenario)
    : TAttributable<TKernelObject<IComment>>(ctx), m_rOwnerScenario(rOwnerScenario), m_text("") {}

//___________________________________________________________________//
//                                                                   //

bool CComment::setIdentifier(const CIdentifier& id)
{
    if (m_id != CIdentifier::undefined() || id == CIdentifier::undefined()) { return false; }
    m_id = id;
    return true;
}

bool CComment::setText(const CString& sText)
{
    m_text = sText;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CComment::initializeFromExistingComment(const IComment& rExisitingComment)
{
    m_text = rExisitingComment.getText();

    CIdentifier id = rExisitingComment.getNextAttributeIdentifier(CIdentifier::undefined());
    while (id != CIdentifier::undefined())
    {
        addAttribute(id, rExisitingComment.getAttributeValue(id));
        id = rExisitingComment.getNextAttributeIdentifier(id);
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CComment::acceptVisitor(IObjectVisitor& rObjectVisitor)
{
    CObjectVisitorContext context(getKernelContext());
    return rObjectVisitor.processBegin(context, *this) && rObjectVisitor.processEnd(context, *this);
}

}  // namespace Kernel
}  // namespace OpenViBE
