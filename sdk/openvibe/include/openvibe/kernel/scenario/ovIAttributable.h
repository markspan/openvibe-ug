///-------------------------------------------------------------------------------------------------
/// 
/// \file ovIAttributable.h
/// \author Yann Renard (IRISA/INRIA).
/// \version 1.0.
/// \date 07/12/2006.
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

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
/**
 * \ingroup Group_Scenario
 * \ingroup Group_Kernel
 */
class OV_API IAttributable : public IKernelObject
{
public:
    virtual bool addAttribute(const CIdentifier& id, const CString& value) = 0;
    virtual bool removeAttribute(const CIdentifier& id) = 0;
    virtual bool removeAllAttributes() = 0;

    virtual CString getAttributeValue(const CIdentifier& id) const = 0;
    virtual bool setAttributeValue(const CIdentifier& id, const CString& value) = 0;

    virtual bool hasAttribute(const CIdentifier& id) const = 0;
    virtual bool hasAttributes() const = 0;

    virtual CIdentifier getNextAttributeIdentifier(const CIdentifier& previousID) const = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Scenario_Attributable)
};
}  // namespace Kernel
}  // namespace OpenViBE
