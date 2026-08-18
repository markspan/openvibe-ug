///-------------------------------------------------------------------------------------------------
/// 
/// \file IKernelDesc.hpp
/// \brief A kernel description.
///
/// \author Yann Renard (INRIA/IRISA).
/// \version 1.0.
/// \date 26/09/2006.
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
class IKernelContext;

///<summary> A kernel description. </summary>
///
/// This interface is implemented to provide information on a specific kernel implementation
/// and to create instances of this specific kernel implementation.
/// \ingroup Group_Kernel
class OV_API IKernelDesc : public IKernelObject
{
public:
    /** \name Creation process */
    //@{

    /// <summary> Creates the kernel itself. </summary>
    /// <param name="applicationName"> The name of the application requesting kernel creation (an configuration token will be created
    /// so the configuration file can be tweaked according to the targeted application). </param>
    /// <param name="configFilename"> A bootstrap configuration file.</param>
    /// <returns> The created kernel. </returns>
    /// <remarks> This method creates the kernel itself and returns it. </remarks>
    virtual IKernelContext* createKernel(const CString& applicationName, const CString& configFilename) = 0;

    /// <summary> Creates the kernel itself and make it sub kernel of a master kernel. </summary>
    /// <param name="masterKernel"> The master kernel. </param>
    /// <param name="applicationName"> The name of the application requesting kernel creation (an configuration token will be created
    /// so the configuration file can be tweaked according to the targeted application). </param>
    /// <param name="configFilename"> A bootstrap configuration file.</param>
    /// <returns> The created kernel. </returns>
    /// <remarks> This method creates the kernel itself and returns it. </remarks>
    virtual IKernelContext* createKernel(const IKernelContext& masterKernel, const CString& applicationName, const CString& configFilename) = 0;

    /// <summary> Releases the kernel itself. </summary>
    /// <param name="kernel"> The kernel to release.</param>
    /// <remarks> This method releases an existing kernel. </remarks>
    virtual void releaseKernel(IKernelContext* kernel) = 0;

    //@}
    /** \name Textual plugin object description and information */
    //@{

    /// <summary> Gets the plugin name. </summary>
    /// <returns> The plugin name. </returns>
    /// <remarks> Default implementation simply returns empty string. </remarks>
    virtual CString getName() const { return ""; }

    /// <summary> Gets the author name for this plugin. </summary>
    /// <returns> The plugin name for this plugin. </returns>
    /// <remarks> Default implementation simply returns empty string. </remarks>
    virtual CString getAuthorName() const { return ""; }

    /// <summary> Gets the author company name for this plugin. </summary>
    /// <returns> The author company name for this plugin. </returns>
    /// <remarks> Default implementation simply returns empty string. </remarks>
    virtual CString getAuthorCompanyName() const { return ""; }

    /// <summary> Gets a short description of the plugin. </summary>
    /// <returns> A short description of the plugin. </returns>
    /// <remarks> Default implementation simply returns empty string. </remarks>
    virtual CString getShortDescription() const { return ""; }

    /// <summary> Gets a detailed description of the plugin. </summary>
    /// <returns> A detailed description of the plugin. </returns>
    /// <remarks> Default implementation simply returns empty string.
    /// You can use std::endl to have the description on several lines when needed. </remarks>
    virtual CString getDetailedDescription() const { return ""; }

    /// <summary> Gets a version of the plugin. </summary>
    /// <returns> The version of the plugin. </returns>
    /// <remarks> Default implementation simply returns empty string. </remarks>
    virtual CString getVersion() const { return ""; }

    //@}

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_KernelDesc)
};
}  // namespace Kernel
}  // namespace OpenViBE
