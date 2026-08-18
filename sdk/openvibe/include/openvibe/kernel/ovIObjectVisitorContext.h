#pragma once

#include "ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
/**
 * \class IObjectVisitorContext
 * \author Yann Renard (INRIA/IRISA)
 * \date 2008-02-01
 * \brief Exectution context for visitor objects
 * \ingroup Group_Kernel
 */
class OV_API IObjectVisitorContext : public IKernelObject
{
public:

    /**
     * \brief Gets a reference on the current algorithm manager
     * \return a reference on the current algorithm manager
     */
    virtual IAlgorithmManager& getAlgorithmManager() const = 0;
    /**
     * \brief Gets a reference on the current configuration manager
     * \return a reference on the current configuration manager
     */
    virtual IConfigurationManager& getConfigurationManager() const = 0;
    /**
     * \brief Gets a reference on the current type manager
     * \return a reference on the current type manager
     */
    virtual ITypeManager& getTypeManager() const = 0;
    /**
     * \brief Gets a reference on the current log manager
     * \return a reference on the current log manager
     */
    virtual ILogManager& getLogManager() const = 0;
    /**
     * \brief Gets a reference on the current error manager
     * \return a reference on the current error manager
     */
    virtual CErrorManager& getErrorManager() const = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_ObjectVisitorContext)
};
}  // namespace Kernel
}  // namespace OpenViBE
