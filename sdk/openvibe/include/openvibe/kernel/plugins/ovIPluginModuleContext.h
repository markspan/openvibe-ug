#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class ILogManager;
class ITypeManager;
class IScenarioManager;
class CErrorManager;
class IConfigurationManager;

/**
 * \class IPluginModuleContext
 * \brief Plugin context
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-06-20
 * \ingroup Group_Plugins
 * \ingroup Group_Kernel
 * \ingroup Group_Extend
 */
class OV_API IPluginModuleContext : public IKernelObject
{
public:

    /**
     * \brief Gets the current scenario manager
     * \return a reference on the current scenario manager
     */
    virtual IScenarioManager& getScenarioManager() const = 0;
    /**
     * \brief Gets the current type manager
     * \return a reference on the current type manager
     */
    virtual ITypeManager& getTypeManager() const = 0;
    /**
     * \brief Gets the current log manager
     * \return a reference on the current log manager
     */
    virtual ILogManager& getLogManager() const = 0;
    /**
     * \brief Gets the current error manager
     * \return a reference on the current error manager
     */
    virtual CErrorManager& getErrorManager() const = 0;
    /**
     * \brief Gets the current configuration manager
     * \return a reference on the current configuration manager
     */
    virtual IConfigurationManager& getConfigurationManager() const = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Plugins_PluginModuleContext)
};
}  // namespace Kernel
}  // namespace OpenViBE
