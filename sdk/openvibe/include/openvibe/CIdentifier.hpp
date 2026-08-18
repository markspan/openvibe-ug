///-------------------------------------------------------------------------------------------------
/// 
/// \file CIdentifier.hpp
/// \brief Globally used identification class.
/// \author  Yann Renard (INRIA/IRISA) & Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 16/06/2006.
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

#include "ov_defines.h"
#include <limits>
#include <string>
#include "ovCString.h"

namespace OpenViBE {

/// <summary> Globally used identification class.\n
///
/// This class is the basic class to use in order to identify objects in the OpenViBE platform.\n
/// It can be used for class identification, for object identification and any user needed identification process.\n
///
/// The identification of the OpenViBE platform is based on 64 bits integers.\n
///
/// This class is heavily used in the OpenViBE::IObject class.\n
/// Also, the OpenViBE specification gives several already defined class identifiers the developer should know of.\n
/// For this, let you have a look to the documentation of defines.hpp !
/// 
/// </summary>
/// <seealso cref="defines.hpp"/>
class OV_API CIdentifier
{
public:
    //--------------------------------------------------
    //------------ Constructor / Destructor ------------
    //--------------------------------------------------

    /// <summary> Default constructor.\n Builds up the 64 bits identifier initialized to <c>undefined</c>. </summary>
    CIdentifier() : m_id(std::numeric_limits<uint64_t>::max()) {}

    /// <summary> 32 bits integer based constructor.\n Builds up the 64 bits identifier given its two 32 bits components. </summary>
    /// <param name="id1">The first part of the identifier.</param>
    /// <param name="id2">The second part of the identifier.</param>
    CIdentifier(const size_t id1, const size_t id2) : m_id((uint64_t(id1) << 32) + id2) {}

    /// <summary> 64 bits integer based constructor. </summary>
    /// <param name="id"> The identifier. </param>
    CIdentifier(const uint64_t id) : m_id(id) {}

    /// <summary> string based constructor. </summary>
    /// <param name="str"> The string with identifier. </param>
    /// <remarks> If string is invalid undefined ID is set. </remarks>
    explicit CIdentifier(const std::string& str) { if (!fromString(str)) { m_id = std::numeric_limits<uint64_t>::max(); } }

    /// <summary> Copy constructor.\n Builds up the 64 bits identifier exacly the same as given identifier parameter. </summary>
    /// <param name="id"> the identifier to initialize this identifier from. </param>
    CIdentifier(const CIdentifier& id) : m_id(id.m_id) {}

    /// <summary> Undefined Identifier (the same as default constructor). </summary>
    /// <returns> Identifier define as undefined. </returns>
    static CIdentifier undefined() { return CIdentifier(std::numeric_limits<uint64_t>::max()); }

    //--------------------------------------------------
    //------------------- Operators --------------------
    //--------------------------------------------------

    /// <summary> Copy Assignment Operator. </summary>
    /// <param name="id"> The identifier. </param>
    /// <returns> Himself. </returns>
    CIdentifier& operator=(const CIdentifier& id);

    /// <summary> Increments this identifier by 1. </summary>
    /// <returns> Himself. </returns>
    /// 
    /// <remarks> If this identifier is <see cref="CIdentifier::undefined()"/>, it is not incremented.\n
    /// If this idenfitier is not <see cref="CIdentifier::undefined()"/>, it can not become <see cref="CIdentifier::undefined()"/> after being incremented. </remarks>
    CIdentifier& operator++();

    /// <summary> Decrements this identifier by 1. </summary>
    /// <returns> Himself. </returns>
    /// <remarks> If this identifier is \c CIdentifier::undefined(), it is not decremented.\n
    /// If this idenfitier is not \c CIdentifier::undefined(), it can not become \c CIdentifier::undefined() after being decremented. </remarks>
    CIdentifier& operator--();

    /// <summary> "Equal" test operator. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if equals, <c>false</c> otherwise. </returns>
    bool operator==(const CIdentifier& id) const { return m_id == id.id(); }

    /// <summary> "Difference" test operator. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if different, <c>false</c> otherwise. </returns>
    bool operator!=(const CIdentifier& id) const { return m_id != id.id(); }

    /// <summary> "Less than" test operator. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if less than the test, <c>false</c> otherwise. </returns>
    bool operator<(const CIdentifier& id) const { return m_id < id.id(); }

    /// <summary> "Greater than" test operator. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if greater than the test, <c>false</c> otherwise. </returns>
    bool operator>(const CIdentifier& id) const { return m_id > id.id(); }

    /// <summary> "Less or equal than" test operator. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if  less or equal than the test, <c>false</c> otherwise. </returns>
    bool operator<=(const CIdentifier& id) const { return m_id <= id.id(); }

    /// <summary> "Greater or equal than" test operator. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if greater or equal than the test, <c>false</c> otherwise. </returns>
    bool operator>=(const CIdentifier& id) const { return m_id >= id.id(); }

    //---------- With Template ----------
    /// <summary> Copy Assignment Operator for integral all types. </summary>
    /// <param name="id"> The identifier. </param>
    /// <returns> Himself. </returns>
    /// <remarks> Template function must be define in header to keep the template system in extern program. </remarks>
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    CIdentifier& operator=(const T id)
    {
        m_id = id;
        return *this;
    }

    /// <summary> "Equal" test operator for integral all types. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if equals, <c>false</c> otherwise. </returns>
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    bool operator==(const T id) const { return m_id == id; }

    /// <summary> "Difference" test operator for integral all types. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if different, <c>false</c> otherwise. </returns>
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    bool operator!=(const T id) const { return m_id != id; }

    /// <summary> "Less than" test operator for integral all types. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if less than the test, <c>false</c> otherwise. </returns>
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    bool operator<(const T id) const { return m_id < id; }

    /// <summary> "Greater than" test operator for integral all types. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if greater than the test, <c>false</c> otherwise. </returns>
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    bool operator>(const T id) const { return m_id > id; }

    /// <summary> "Less or equal than" test operator for integral all types. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if  less or equal than the test, <c>false</c> otherwise. </returns>
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    bool operator<=(const T id) const { return m_id <= id; }

    /// <summary> "Greater or equal than" test operator for integral all types. </summary>
    /// <param name="id"> The identifier to compare. </param>
    /// <returns> <c>true</c> if greater or equal than the test, <c>false</c> otherwise. </returns>
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    bool operator>=(const T id) const { return m_id >= id; }

    //--------------------------------------------------
    //---------------------- Misc ----------------------
    //--------------------------------------------------

    /// <summary> Converts this identifier into a string. </summary>
    /// <param name="hexa"> if the str is in hexadecimal with two uint32 number or in decimal mode with only one uint64 number. </param>
    /// <returns> This identifier represented as a <c>std::string</c>. </returns>
    std::string str(const bool hexa = true) const;

    /// <summary> Converts this identifier into an OpenViBE string. </summary>
    /// <returns> This identifier represented as an OpenViBE string. </returns>
    /// <remarks> Avoid this, this function keep previous compatibility with CString. </remarks>
    CString toString() const { return str().c_str(); }

    /// <summary> Reads a a string to extract this identifier. </summary>
    /// <param name="str"> the string to convert. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    bool fromString(const std::string& str);


    /// <summary> Reads a a string to extract this identifier. </summary>
    /// <param name="str"> the string to convert. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    /// \deprecated Use the same method with std::string parameter instead.
    bool fromString(const CString& str) { return fromString(std::string(str.toASCIIString())); }

    /// <summary> Get the ID. </summary>
    /// <returns> The unsigned integer identifier. </returns>
    /// <remarks> Use this function with care, identifiers should not be considered as integers.
    /// Actually, the internal 64 bits representation may change, resulting in code port needs if you use this function. </remarks>
    uint64_t id() const { return m_id; }

    /// <summary> Get the ID. </summary>
    /// <returns> The unsigned integer identifier. </returns>
    /// <remarks> Use this function with care, identifiers should not be considered as integers.
    /// Actually, the internal 64 bits representation may change, resulting in code port needs if you use this function.
    /// Avoid this, this function keep previous compatibility with heavy name. </remarks>
    /// \deprecated Use id() method instead.
    OV_Deprecated("Use id() method instead.")
    uint64_t toUInteger() const { return m_id; }

    /// <summary> Creates a random identifier. </summary>
    /// <returns> A random identifier. </returns>
    /// <remarks> The returned identifier can not be \c CIdentifier::undefined(). </remarks>
    static CIdentifier random();

    /// <summary> Override the ostream operator. </summary>
    /// <param name="os"> The ostream. </param>
    /// <param name="obj"> The object. </param>
    /// <returns> Return the modified ostream. </returns>
    friend std::ostream& operator<<(std::ostream& os, const CIdentifier& obj)
    {
        os << obj.str();
        return os;
    }

protected:

    uint64_t m_id = 0;  ///< the 64 bit identifier value
};
}  // namespace OpenViBE

/// <remarks> Avoid this, #define is replace by constexpr for modern compiler and undefined is set in CIdentifier Class. </remarks>
#define OV_UndefinedIdentifier  OpenViBE::CIdentifier::undefined()
