///-------------------------------------------------------------------------------------------------
/// 
/// \file IKernelObjectFactory.hpp
/// \brief Kernel object factory, creates all kernel objects.
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

/// <summary> Kernel object factory, creates all kernel objects. </summary>
///
/// This class allows to create kernel objects as needed.
/// <seealso cref="IKernelObject" />
/// \ingroup Group_Kernel
/// \todo Can be removed with the CKernelObjectFactory class ?
class OV_API IKernelObjectFactory : public IKernelObject
{
public:
    /// <summary> Creates a new kernel object givent its class identifier. </summary>
    /// <param name="classID"> the class identifier of the object to create. </param>
    /// <returns> A pointer on the created object in case of success, <c>nullptr</c> in case of error. </returns>
    virtual IObject* createObject(const CIdentifier& classID) = 0;

    /// <summary> Releases an object created by this factory. </summary>
    /// <param name="obj"> The object to release. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> in case of error. </returns>
    /// <remarks> The factory should have created the object in order to release it. </remarks>
    virtual bool releaseObject(IObject* obj) = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_KernelObjectFactory)
};
}  // namespace Kernel
}  // namespace OpenViBE
