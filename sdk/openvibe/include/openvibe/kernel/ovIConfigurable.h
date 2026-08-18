#pragma once

#include "ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class IParameter;

/**
 * \brief Parameter type enumeration for a configurable object
 * \sa IConfigurable
 * \sa IParameter
 */
enum EParameterType
{
    ParameterType_None,           ///< No parameter
    ParameterType_Integer,        ///< Integer parameter, 64bit
    ParameterType_UInteger,       ///< Unsigned integer parameter, 64bit
    ParameterType_Enumeration,    ///< Enumeration integer parameter
    ParameterType_Boolean,        ///< Boolean parameter
    ParameterType_Float,          ///< Float parameter, 64bit
    ParameterType_String,         ///< String parameter
    ParameterType_Identifier,     ///< Identifier parameter
    ParameterType_Matrix,         ///< Matrix pointer parameter
    ParameterType_StimulationSet, ///< Stimulation set pointer parameter
    ParameterType_MemoryBuffer,   ///< Memory buffer pointer parameter
    ParameterType_Object,         ///< Object pointer parameter
    ParameterType_Pointer,        ///< Raw pointer parameter
};

/**
 * \class IConfigurable
 * \brief Configurable object interface
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-11-21
 * \sa IParameter
 * \ingroup Group_Kernel
 *
 * An instance of this class is able to self create several type of parameters
 * in order to be used by external code. The parameter handling is very abstract
 * but its use is made easier thanks to the parameter handler. See IParameter and
 * TParameterHandler for more details.
 */
class OV_API IConfigurable : public IKernelObject
{
public:

    /**
     * \brief Enumerates parameter identifiers for this configurable.
     * \param previousID [in] : the identifier which next identifier has to be returned
     * \return the parameter identifier following the provided parameter identifier.
     * \note if \c previousID is \e CIdentifier::undefined() , the first parameter identifier is returned
     * \note getting \e CIdentifier::undefined() has result means there are no more identifier after \c previousID
     *
     * Sample code to iterate on parameter identifiers :
     *
     * \code
     * IConfigurable* configurable= // ...
     * CIdentifier currentID=CIdentifier::undefined();
     * while((currentID=configurable->getNextParameterIdentifier(currentID))!=CIdentifier::undefined())
     * {
     *   IParameter* parameter=configurable->getParameter(currentID);
     *   // ...
     * }
     * \endcode
     */
    virtual CIdentifier getNextParameterIdentifier(const CIdentifier& previousID) const = 0;

    /**
     * \brief Gets a specific parameter given its id
     * \param id [in] : the identifier of the parameter to get
     * \return a pointer to the corresponding parameter in case of success.
     * \return \c NULL in case of error.
     */
    virtual IParameter* getParameter(const CIdentifier& id) = 0;
    /**
     * \brief Replaces the parameter with a client handled object
     * \param id [in] : the identifier of the parameter to replace
     * \param parameter [in] : the parameter object to put in place of the old parameter
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note The parameter should have been created before and should exist.
     * \note Even if a new paramter is affected to a configurable, the memory management
     *       of this parameter remains to the responsability of the client code. Thus
     *       none of \c removeParameter nor configurable destruction will release
     *       this parameter object. The caller should take care of this when needed.
     */
    virtual bool setParameter(const CIdentifier& id, IParameter& parameter) = 0;
    /**
     * \brief Creates a new parameter of a specific type
     * \param id [in] : the parameter id which has to be created
     * \param type [in] : the type of this parameter
     * \param subTypeID [in] : the optional sub type of this parameter (e.g. for enumerations)
     * \sa EParameterType
     * \sa IParameter
     *
     * This function creates a new parameter with its associated object.
     */
    virtual IParameter* createParameter(const CIdentifier& id, const EParameterType type, const CIdentifier& subTypeID = CIdentifier::undefined()) = 0;
    /**
     * \brief Removes an existing parameter
     * \param id [in] : the identifier of the parameter to remove
     * \return \e true in case of success
     * \return \e false in case of error
     * \note if the parameter object is not released if it was replaced by a custom
     *       parameter object thanks to \c setParameter function.
     */
    virtual bool removeParameter(const CIdentifier& id) = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Configurable)
};
}  // namespace Kernel
}  // namespace OpenViBE
