///-------------------------------------------------------------------------------------------------
/// 
/// \file CNameValuePairList.cpp
/// \brief OpenViBE Pair Name/Value List Class (It handles a hidden map associating string/keys to string/values).
/// 
/// This interface offers functionalities to handle a collection of OpenViBE stimulations.
/// This collection basicaly consists in a list of stimulation information.
/// Each stimulation has three information : an identifier, a dateand a duration.
/// \author  Vincent Delannoy (INRIA/IRISA).
/// \version 1.0.
/// \date 01/07/2008.
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

#include "CNameValuePairList.hpp"

#include <map>
#include <string>
#include <sstream>

namespace OpenViBE {
//--------------------------------------------------
//----------------- Getter/Setter ------------------
//--------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::getValue(const std::string& name, std::string& value) const
{
    if (m_map->find(name) == m_map->end()) { return false; }
    value = m_map->at(name);
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::getValue(const std::string& name, double& value) const
{
    if (m_map->find(name) == m_map->end()) { return false; }
    double temp;

    try { temp = std::stod(m_map->at(name)); }
    catch (const std::exception&) { return false; }

    value = temp;
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::getValue(const std::string& name, bool& value) const
{
    if (m_map->find(name) == m_map->end()) { return false; }
    const std::string str = m_map->at(name);
    if (str == "0" || str == "FALSE" || str == "false") { value = false; }
    else if (str == "1" || str == "TRUE" || str == "true") { value = true; }
    else { return false; }
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::getValue(const size_t index, std::string& name, std::string& value) const
{
    if (index >= this->size()) { return false; }
    auto it = m_map->begin();
    std::advance(it, index);
    name  = it->first;
    value = it->second;
    return true;
}
//--------------------------------------------------------------------------------

//-------------------------------------------------------
//---------------------- Operators ----------------------
//-------------------------------------------------------

//--------------------------------------------------------------------------------
CNameValuePairList& CNameValuePairList::operator=(const CNameValuePairList& pairs)
{
    if (this != &pairs) { *m_map = *pairs.m_map; }// Deep Copy
    return *this;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
std::string CNameValuePairList::str(const std::string& sep) const
{
    std::stringstream ss;
    bool first = true;
    for (const auto& pair : *m_map) {
        ss << (first ? "" : sep) << "[" << pair.first << ", " << pair.second << "]";
        first = false;
    }
    return ss.str();
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------
//---------------------- Deprecated ----------------------
//--------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::setValue(const CString& name, const CString& value) const
{
    setValue(std::string(name), value);
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::setValue(const CString& name, const char* value) const
{
    if (value == nullptr) { return false; }
    setValue(std::string(name), std::string(value));
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::setValue(const CString& name, const double& value) const
{
    setValue(std::string(name), value);
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::setValue(const CString& name, const bool value) const
{
    setValue(std::string(name), value);
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::getValue(const CString& name, CString& value) const
{
    std::string out;
    const bool res = getValue(std::string(name), out);
    value          = out.c_str();
    return res;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::getValue(const CString& name, double& value) const { return getValue(std::string(name), value); }
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::getValue(const CString& name, bool& value) const { return getValue(std::string(name), value); }
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CNameValuePairList::getValue(const size_t index, CString& name, CString& value) const
{
    std::string outName, outValue;
    const bool res = getValue(index, outName, outValue);
    name           = outName.c_str();
    value          = outValue.c_str();
    return res;
}
//--------------------------------------------------------------------------------

}  // namespace OpenViBE
