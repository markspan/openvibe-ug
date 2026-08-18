#include "../ovkTKernelObject.h"

#include "ovkCMetadata.h"
#include "ovkCScenario.h"

#include "../ovkCObjectVisitorContext.h"

namespace OpenViBE {
namespace Kernel {

bool CMetadata::setIdentifier(const CIdentifier& identifier)
{
    OV_ERROR_UNLESS_KRF(m_id == CIdentifier::undefined(),
                        "Metadata [" << m_id.str() << "] in scenario [" << m_ownerScenario.getIdentifier() << "]  already has an identifier.",
                        ErrorType::BadCall);

    OV_ERROR_UNLESS_KRF(identifier != CIdentifier::undefined(),
                        "Attempted to assign undefined identifier to Metadata in scenario [" << m_ownerScenario.getIdentifier() << "].",
                        ErrorType::BadArgument);

    m_id = identifier;
    return true;
}

bool CMetadata::setType(const CIdentifier& typeID)
{
    OV_ERROR_UNLESS_KRF(typeID != CIdentifier::undefined(),
                        "Attempted to assign undefined typeID to Metadata [" << m_id.str() << "] in scenario [" << m_ownerScenario.getIdentifier() << "].",
                        ErrorType::BadArgument);

    m_type = typeID;
    return true;
}

bool CMetadata::setData(const CString& data)
{
    m_data = data;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CMetadata::initializeFromExistingMetadata(const IMetadata& existingMetadata)
{
    m_data = existingMetadata.getData();
    m_type = existingMetadata.getType();
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CMetadata::acceptVisitor(IObjectVisitor& objectVisitor)
{
    CObjectVisitorContext objectVisitorContext(this->getKernelContext());
    return objectVisitor.processBegin(objectVisitorContext, *this) && objectVisitor.processEnd(objectVisitorContext, *this);
}

}  // namespace Kernel
}  // namespace OpenViBE
