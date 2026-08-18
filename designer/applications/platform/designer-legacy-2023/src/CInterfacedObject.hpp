///-------------------------------------------------------------------------------------------------
/// 
/// \file CInterfacedObject.hpp
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

#include "base.hpp"

namespace OpenViBE {
namespace Designer {
class CInterfacedObject
{
public:
    CInterfacedObject() = default;
    explicit CInterfacedObject(const CIdentifier& identifier) : m_ID(identifier) { }

    CInterfacedObject(const CIdentifier& identifier, const size_t connectorType, const size_t connectorIndex)
        : m_ID(identifier), m_ConnectorType(connectorType), m_ConnectorIdx(connectorIndex) { }

    CIdentifier m_ID       = CIdentifier::undefined();
    size_t m_ConnectorType = 0;
    size_t m_ConnectorIdx  = 0;
};
}  // namespace Designer
}  // namespace OpenViBE
