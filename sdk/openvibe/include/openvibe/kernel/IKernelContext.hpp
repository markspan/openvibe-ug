///-------------------------------------------------------------------------------------------------
/// 
/// \file IKernelContext.hpp
/// \brief Kernel context interface, gives access to each manager the kernel owns.
///
/// \author Yann Renard (INRIA/IRISA).
/// \version 1.0.
/// \date 24/10/2007.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class IAlgorithmManager;
class IConfigurationManager;
class IKernelObjectFactory;
class IPlayerManager;
class IPluginManager;
class IMetaboxManager;
class IScenarioManager;
class ITypeManager;
class ILogManager;
class CErrorManager;

/// <summary> Kernel context interface, gives access to each manager the kernel owns. </summary>
///
/// This class simply provides access to each manager the kernel owns.
/// This is the top level object that can be used by a custom OpenViBE application
/// and this is the common object all kernel object have in order to access all the functionnalities.
///
/// See each manager's own documentation for more detail on a specific manager goal and usage.
/// <seealso cref="IKernelObject" />
/// \ingroup Group_Kernel
class OV_API IKernelContext : public IKernelObject
{
public:
    /// <summary> Initializes the kernel context. </summary>
    /// <returns> <c>true</c> in case of success <c>false</c> otherwise. </returns>
    virtual bool initialize(const char* const* /*tokenList*/ = nullptr, size_t /*tokenCount*/  = 0) { return true; }

    /// <summary> Uninitializes the kernel context. </summary>
    /// <returns> <c>true</c> in case of success <c>false</c> otherwise. </returns>
    virtual bool uninitialize() { return true; }

    /// <summary> Gets a reference on the kernel's algorithm manager. </summary>
    /// <returns> A reference on the kernel's algorithm manager. </returns>
    virtual IAlgorithmManager& getAlgorithmManager() const = 0;

    /// <summary> Gets a reference on the kernel's configuration manager. </summary>
    /// <returns> A reference on the kernel's configuration manager. </returns>
    virtual IConfigurationManager& getConfigurationManager() const = 0;

    /// <summary> Gets a reference on the kernel's player manager. </summary>
    /// <returns> A reference on the kernel's player manager. </returns>
    virtual IPlayerManager& getPlayerManager() const = 0;

    /// <summary> Gets a reference on the kernel's plugin manager. </summary>
    /// <returns> A reference on the kernel's plugin manager. </returns>
    virtual IPluginManager& getPluginManager() const = 0;

    /// <summary> Gets a reference on the kernel's metabox manager. </summary>
    /// <returns> A reference on the kernel's metabox manager. </returns>
    virtual IMetaboxManager& getMetaboxManager() const = 0;

    /// <summary> Gets a reference on the kernel's object factory. </summary>
    /// <returns> A reference on the kernel's object factory. </returns>
    virtual IKernelObjectFactory& getKernelObjectFactory() const = 0;

    /// <summary> Gets a reference on the kernel's scenario manager. </summary>
    /// <returns> A reference on the kernel's scenario manager. </returns>
    virtual IScenarioManager& getScenarioManager() const = 0;

    /// <summary> Gets a reference on the kernel's type manager. </summary>
    /// <returns> A reference on the kernel's type manager. </returns>
    virtual ITypeManager& getTypeManager() const = 0;

    /// <summary> Gets a reference on the kernel's log manager. </summary>
    /// <returns> A reference on the kernel's log manager. </returns>
    virtual ILogManager& getLogManager() const = 0;

    /// <summary> Gets a reference on the kernel's error manager. </summary>
    /// <returns> A reference on the kernel's error manager. </returns>
    virtual CErrorManager& getErrorManager() const = 0;

    /// <summary> Gets a reference on the kernel's object factory. </summary>
    /// <returns> A reference on the kernel's object factory. </returns>
    /// \deprecated Use the getKernelObjectFactory() instead.
    OV_Deprecated("Use the getKernelObjectFactory() instead")
    virtual IKernelObjectFactory& getObjectFactory() const { return getKernelObjectFactory(); }

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_KernelContext)
};
}  // namespace Kernel
}  // namespace OpenViBE
