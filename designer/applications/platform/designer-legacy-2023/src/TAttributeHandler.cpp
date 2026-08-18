///-------------------------------------------------------------------------------------------------
/// 
/// \file TAttributeHandler.cpp
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

#include "TAttributeHandler.hpp"

#include <cstdlib>

namespace OpenViBE {
namespace Designer {

bool TAttributeHandler::RemoveAttribute(const CIdentifier& id) const
{
    if (!m_attributable) { return false; }
    return m_attributable->removeAttribute(id);
}

bool TAttributeHandler::RemoveAllAttributes() const

{
    if (!m_attributable) { return false; }
    return m_attributable->removeAllAttributes();
}

template <>
bool TAttributeHandler::addAttribute(const CIdentifier& id, const int& value) const
{
    if (!m_attributable) { return false; }
    const std::string str = std::to_string(value);  // pass directly  std::to_string().c_str() with int value can return anything
    return m_attributable->addAttribute(id, str.c_str());
}

template <>
bool TAttributeHandler::addAttribute(const CIdentifier& id, const bool& value) const
{
    if (!m_attributable) { return false; }

    return m_attributable->addAttribute(id, (value ? "true" : "false"));
}

template <>
int TAttributeHandler::getAttributeValue<int>(const CIdentifier& id) const { return strtol(m_constAttributable->getAttributeValue(id), nullptr, 10); }

template <>
bool TAttributeHandler::getAttributeValue<bool>(const CIdentifier& id) const
{
    bool res = false;
    const CString value(m_constAttributable->getAttributeValue(id));
    if (value == CString("true")) { res = true; }

    return res;
}

template <>
bool TAttributeHandler::setAttributeValue(const CIdentifier& id, const int& value)
{
    if (!m_attributable) { return false; }
    const std::string str = std::to_string(value);  // pass directly  std::to_string().c_str() with int value can return anything
    return m_attributable->setAttributeValue(id, str.c_str());
}

template <>
bool TAttributeHandler::setAttributeValue(const CIdentifier& id, const bool& value)
{
    if (!m_attributable) { return false; }
    return m_attributable->setAttributeValue(id, (value ? "true" : "false"));
}
}  // namespace Designer
}  // namespace OpenViBE
