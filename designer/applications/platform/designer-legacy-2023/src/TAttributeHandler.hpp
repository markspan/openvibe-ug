///-------------------------------------------------------------------------------------------------
/// 
/// \file TAttributeHandler.hpp
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
class TAttributeHandler
{
public:
    explicit TAttributeHandler(Kernel::IAttributable& attributable) : m_constAttributable(&attributable), m_attributable(&attributable) { }
    explicit TAttributeHandler(const Kernel::IAttributable& attributable) : m_constAttributable(&attributable) { }

    template <class T>
    bool addAttribute(const CIdentifier& id, const T& value) const;

    bool RemoveAttribute(const CIdentifier& id) const;

    bool RemoveAllAttributes() const;

    template <class T>
    T getAttributeValue(const CIdentifier& id) const;

    template <class T>
    bool setAttributeValue(const CIdentifier& id, const T& value);

    bool HasAttribute(const CIdentifier& id) const { return m_constAttributable->hasAttribute(id); }
    bool HasAttributes() const { return m_constAttributable->hasAttributes(); }

protected:
    const Kernel::IAttributable* m_constAttributable;
    Kernel::IAttributable* m_attributable = nullptr;
};
}  // namespace Designer
}  // namespace OpenViBE
