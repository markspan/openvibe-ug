///-------------------------------------------------------------------------------------------------
/// 
/// \file ovIKernelObject.h
/// \brief Base class for all kernel objects.
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

#include "../ovIObject.h"

namespace OpenViBE {
namespace Kernel {
///<summary> Base class for all kernel objects. </summary>
/// \ingroup Group_Kernel
class OV_API IKernelObject : public IObject
{
public:
    _IsDerivedFromClass_(IObject, OV_ClassId_Kernel_KernelObject)
};
}  // namespace Kernel
}  // namespace OpenViBE
