///-------------------------------------------------------------------------------------------------
/// 
/// \file CNameValuePairList.hpp
/// \brief OpenViBE Pair Name/Value List Class (It handles a hidden map associating string/keys to string/values).
/// 
/// This interface offers functionalities to handle a collection of OpenViBE stimulations.
/// This collection basicaly consists in a list of stimulation information.
/// Each stimulation has three information : an identifier, a date and a duration.
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

#pragma once

#include <ov_common_defines.h>
#include "ovCString.h"

#include <map>
#include <string>

namespace OpenViBE {

/// <summary> OpenViBE Pair Name/Value List Class (It handles a hidden map associating string/keys to string/values). </summary>
/// <remarks> Implementation based on std::map<std::string, std::string>.  
/// Map is a pointer to prevents potential compile/link errors when dynamically loading modules. </remarks>
class OV_API CNameValuePairList
{
public:
    //--------------------------------------------------
    //------------ Constructor / Destructor ------------
    //--------------------------------------------------

    /// <summary> Default constructor of the <see cref="CNameValuePairList"/> class (Initializes the list). </summary>
    CNameValuePairList() { m_map = new std::map<std::string, std::string>; }

    /// <summary> Copy constructor of the <see cref="CNameValuePairList"/> class (Initializes the list). </summary>
    /// <param name="pairs">The list to copy. </param>
    CNameValuePairList(const CNameValuePairList& pairs) { m_map = new std::map<std::string, std::string>(*pairs.m_map); }

    /// <summary> Destructor of the <see cref="CNameValuePairList"/> class (Releases the list). </summary>
    ~CNameValuePairList() { delete m_map; }

    //--------------------------------------------------
    //----------------- Getter/Setter ------------------
    //--------------------------------------------------

    /// <summary> Insert a name/value pair. </summary>
    /// <param name="name"> Name to add to the list. </param>
    /// <param name="value"> Value to associate with the name. </param>
    void setValue(const std::string& name, const std::string& value) const { m_map->operator[](name) = value; }

    /// <summary> Insert a name/value pair. </summary>
    /// <param name="name"> Name to add to the list. </param>
    /// <param name="value"> Value to associate with the name. </param>
    void setValue(const std::string& name, const double& value) const { m_map->operator[](name) = std::to_string(value); }

    /// <summary> Insert a name/value pair. </summary>
    /// <param name="name"> Name to add to the list. </param>
    /// <param name="value"> Value to associate with the name. </param>
    void setValue(const std::string& name, const bool value) const { m_map->operator[](name) = (value ? std::string("1") : std::string("0")); }

    /// <summary> Retrieve a string value from the list. </summary>
    /// <param name="name"> Name whose value is to be retrieved. </param>
    /// <param name="value"> String value to be retrieved from the list. </param>
    /// <returns> <c>true</c> if value could be retrieved, <c>false</c> otherwise. </returns>
    bool getValue(const std::string& name, std::string& value) const;

    /// <summary> Retrieve a double value from the list. </summary>
    /// <param name="name"> Name whose value is to be retrieved. </param>
    /// <param name="value"> Double value to be retrieved from the list. </param>
    /// <returns> <c>true</c> if value could be retrieved, <c>false</c> otherwise. </returns>
    bool getValue(const std::string& name, double& value) const;

    /// <summary> Retrieve a boolean value from the list.
    /// 
    /// In the current implementation a value evaluates to true if its string equals "1" and to false if it equals "0".</summary>
    /// <param name="name"> Name whose value is to be retrieved. </param>
    /// <param name="value"> boolean value to be retrieved from the list. </param>
    /// <returns> <c>true</c> if value could be retrieved and evaluates to a boolean, <c>false</c> otherwise. </returns>
    bool getValue(const std::string& name, bool& value) const;

    /// <summary> Retrieve a value from the list. </summary>
    /// <param name="index"> The index whose value is to be retrieved. </param>
    /// <param name="name"> Name of the value stored in index. </param>
    /// <param name="value"> Value stored in index. </param>
    /// <returns> <c>true</c> if value could be retrieved, <c>false</c> otherwise. </returns>
    bool getValue(const size_t index, std::string& name, std::string& value) const;

    /// <summary> Retrieve the number of stored elements. </summary>
    /// <returns> The number of stored elements. </returns>
    size_t size() const { return m_map->size(); }

    //-------------------------------------------------------
    //---------------------- Operators ----------------------
    //-------------------------------------------------------

    /// <summary> Affectation operator (copy). </summary>
    /// <param name="pairs">The list to copy. </param>
    /// <returns> This list. </returns>
    CNameValuePairList& operator=(const CNameValuePairList& pairs);

    /// <summary> Display the Labels. </summary>
    /// <param name="sep">  separator between value (by default tabulation). </param>
    /// <returns> the Labels. </returns>
    std::string str(const std::string& sep = "\t") const;

    /// <summary> Override the ostream operator. </summary>
    /// <param name="os">   The ostream. </param>
    /// <param name="obj">  The object. </param>
    /// <returns> Return the modified ostream. </returns>
    friend std::ostream& operator<<(std::ostream& os, const CNameValuePairList& obj)
    {
        os << obj.str();
        return os;
    }

    //--------------------------------------------------------
    //---------------------- Deprecated ----------------------
    //--------------------------------------------------------

    /// <summary> Insert a name/value pair. </summary>
    /// <param name="name"> Name to add to the list. </param>
    /// <param name="value"> Value to associate with the name. </param>
    /// <returns> <c>true</c> </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    OV_Deprecated("Use the same method with std::string parameter instead.")
    bool setValue(const CString& name, const CString& value) const;

    /// <summary> Insert a name/value pair. </summary>
    /// <param name="name"> Name to add to the list. </param>
    /// <param name="value"> Value to associate with the name. </param>
    /// <returns> <false> if pointer is <c>nullptr</c>, <c>true</c> otherwise. </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    OV_Deprecated("Use the same method with std::string parameter instead.")
    bool setValue(const CString& name, const char* value) const;

    /// <summary> Insert a name/value pair. </summary>
    /// <param name="name"> Name to add to the list. </param>
    /// <param name="value"> Value to associate with the name. </param>
    /// <returns> <c>true</c> </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    OV_Deprecated("Use the same method with std::string parameter instead.")
    bool setValue(const CString& name, const double& value) const;

    /// <summary> Insert a name/value pair. </summary>
    /// <param name="name"> Name to add to the list. </param>
    /// <param name="value"> Value to associate with the name. </param>
    /// <returns> <c>true</c> </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    OV_Deprecated("Use the same method with std::string parameter instead.")
    bool setValue(const CString& name, bool value) const;

    /// <summary> Retrieve a string value from the list. </summary>
    /// <param name="name"> Name whose value is to be retrieved. </param>
    /// <param name="value"> String value to be retrieved from the list. </param>
    /// <returns> <c>true</c> if value could be retrieved, <c>false</c> otherwise. </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    OV_Deprecated("Use the same method with std::string parameter instead.")
    bool getValue(const CString& name, CString& value) const;

    /// <summary> Retrieve a double value from the list. </summary>
    /// <param name="name"> Name whose value is to be retrieved. </param>
    /// <param name="value"> Double value to be retrieved from the list. </param>
    /// <returns> <c>true</c> if value could be retrieved, <c>false</c> otherwise. </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    OV_Deprecated("Use the same method with std::string parameter instead.")
    bool getValue(const CString& name, double& value) const;

    /// <summary> Retrieve a boolean value from the list.
    /// 
    /// In the current implementation a value evaluates to true if its string equals "1" and to false if it equals "0".</summary>
    /// <param name="name"> Name whose value is to be retrieved. </param>
    /// <param name="value"> boolean value to be retrieved from the list. </param>
    /// <returns> <c>true</c> if value could be retrieved and evaluates to a boolean, <c>false</c> otherwise. </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    OV_Deprecated("Use the same method with std::string parameter instead.")
    bool getValue(const CString& name, bool& value) const;

    /// <summary> Retrieve a value from the list. </summary>
    /// <param name="index"> The index whose value is to be retrieved. </param>
    /// <param name="name"> Name of the value stored in index. </param>
    /// <param name="value"> Value stored in index. </param>
    /// <returns> <c>true</c> if value could be retrieved, <c>false</c> otherwise. </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    OV_Deprecated("Use the same method with std::string parameter instead.")
    bool getValue(const size_t index, CString& name, CString& value) const;

    /// <summary> Retrieve the number of stored elements. </summary>
    /// <returns> The number of stored elements. </returns>
    /// \deprecated Use size() method instead.
    OV_Deprecated("Use size() method instead.")
    size_t getSize() const { return m_map->size(); }

protected:

    std::map<std::string, std::string>* m_map = nullptr; ///< The list implementation
};
}  // namespace OpenViBE
