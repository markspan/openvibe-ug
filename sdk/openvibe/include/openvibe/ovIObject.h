#pragma once

#include "ov_defines.h"
#include "CIdentifier.hpp"

namespace OpenViBE {
class IObjectVisitor;

namespace Kernel {
class CKernelObjectFactory;
}  // namespace Kernel

#define _IsDerivedFromClass_(_SuperClassName_,_ClassIdentifier_) \
    bool isDerivedFromClass(  const OpenViBE::CIdentifier& classId) const override { return ((classId==(_ClassIdentifier_)) || _SuperClassName_::isDerivedFromClass(classId)); }

#define _IsDerivedFromClass_Final_(_SuperClassName_,_ClassIdentifier_) \
    _IsDerivedFromClass_(_SuperClassName_,_ClassIdentifier_) \
    OpenViBE::CIdentifier getClassIdentifier() const override { return _ClassIdentifier_; }

/**
 * \class IObject
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-06-16
 * \brief Base class for all the OpenViBE platform objects
 *
 * Most of the complex objects existing in the OpenViBE platform should derive this base interface. Thus, several operations can
 * be performed in order to work on the object. The most important may be the isDerivedFromClass method that allows the caller to
 * know it the object has specific interface implementation and if the object could finally be casted in this interface or subclass.
 * Several interfaces are provided in the OpenViBE specficiation but custom class could also be created. It is the responsability
 * of the developer to notify the user of what interfaces are implemented in a concrete class.
 *
 * See isDerivedFromClass to have a sample of how this function
 * could be used.
 */
class OV_API IObject
{
public:

    friend class Kernel::CKernelObjectFactory;

    /** \name Class identification */
    //@{

    /**
     * \brief Returns the final class identifier of the concrete class
     * \return The class identifier of this object.
     *
     * This method should return the class identifier of the concrete instanciated class.
     */
    virtual CIdentifier getClassIdentifier() const = 0;
    /**
     * \brief Checks if this object is compatible with a class identifier
     * \param classId [in] : the class identifier you want to test this object compatibility with
     * \return \e true if this object si compatible with the given class identifier (this means the concrete class
     *         overloads the class with given class identifier) and \e false when this object is not compatible.
     *
     * This method should be used to check object compatibility with super classes and interfaces. For any concrete class
     * instance derived from IObject, one can check if plugin functions are implemented and so on... see
     * Plugins::IPluginObject for an example...
     */
    virtual bool isDerivedFromClass(const CIdentifier& classId) const { return (classId == OV_ClassId_Object); }

    //@}
    /** \name Visiting processes */
    //@{

    /**
     * \brief Requests this object to accept a visitor
     * \param visitor [in] : the visitor reference which should act on this object
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool acceptVisitor(IObjectVisitor& visitor) { return true; }

    //@}

protected:

    virtual ~IObject() { }
};

class CNullObject final : public IObject
{
public:

    _IsDerivedFromClass_Final_(IObject, OV_ClassId_Object)
};
}  // namespace OpenViBE
