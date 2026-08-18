#pragma once

#include "../ovIConfigurable.h"

namespace OpenViBE {
namespace Kernel {
/**
 * \class IAlgorithmProto
 * \brief Prototype interface for algorithm
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-11-21
 * \ingroup Group_Algorithm
 * \ingroup Group_Kernel
 * \ingroup Group_Extend
 * \sa Plugins::IAlgorithm
 * \sa Plugins::IAlgorithmDesc
 */
class OV_API IAlgorithmProto : public IKernelObject
{
public:

    /**
     * \brief Adds an input parameter
     * \param id [in] : the identifier for this parameter
     * \param name [in] : the name for this parameter
     * \param type [in] : the type for this parameter
     * \param subTypeID [in] : the optional sub type of this parameter (e.g. for enumerations)
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \sa IParameter
     */
    virtual bool addInputParameter(const CIdentifier& id, const CString& name, EParameterType type,
                                   const CIdentifier& subTypeID = CIdentifier::undefined()) = 0;
    /**
     * \brief Adds an output parameter
     * \param id [in] : the identifier for this parameter
     * \param name [in] : the name for this parameter
     * \param type [in] : the type for this parameter
     * \param subTypeID [in] : the optional sub type of this parameter (e.g. for enumerations)
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \sa IParameter
     */
    virtual bool addOutputParameter(const CIdentifier& id, const CString& name, EParameterType type,
                                    const CIdentifier& subTypeID = CIdentifier::undefined()) = 0;
    /**
     * \brief Adds an input trigger
     * \param id [in] : the identifier for this trigger
     * \param name [in] : the name for this trigger
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool addInputTrigger(const CIdentifier& id, const CString& name) = 0;
    /**
     * \brief Adds an output trigger
     * \param id [in] : the identifier for this trigger
     * \param name [in] : the name for this trigger
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool addOutputTrigger(const CIdentifier& id, const CString& name) = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Algorithm_AlgorithmProto)
};
}  // namespace Kernel
}  // namespace OpenViBE
