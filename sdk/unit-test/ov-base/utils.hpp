///-------------------------------------------------------------------------------------------------
/// 
/// \file utils.hpp
/// \brief Some constants and functions for google tests
/// \author Thibaut Monseigne (Inria).
/// \version 0.1.
/// \date 26/10/2018.
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

#include "openvibe/CIdentifier.hpp"
#include "openvibe/CTime.hpp"
#include <openvibe/CMatrix.hpp>

#include <cmath>
#include <sstream>

#define SEP "\n====================\n"  //const std::string cause Clang warning (with new compiler constexpr change defines)

//---------------------------------------------------------------------------------------------------
/// <summary> Check if double are almost equals. </summary>
/// <param name="a"> The first number. </param>
/// <param name="b"> The second number. </param>
/// <param name="epsilon"> The tolerance. </param>
/// <returns> <c>true</c> if almmost equals, <c>false</c> otherwise. </returns>
inline bool AlmostEqual(const double a, const double b, const double epsilon = OV_EPSILON) { return std::fabs(a - b) < std::fabs(epsilon); }

//*****************************************************************
//********** Error Message Standardization for googltest **********
//*****************************************************************
//---------------------------------------------------------------------------------------------------
/// <summary>   Error message for numeric value. </summary>
/// <param name="name"> The name of the test. </param>
/// <param name="ref">  The reference value. </param>
/// <param name="calc"> The calculate value. </param>
/// <returns>   Error message. </returns>
/// <typeparam name="T">    Generic numeric type parameter. </typeparam>
template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
std::string ErrorMsg(const std::string& name, const T ref, const T calc)
{
    std::stringstream ss;
    ss << SEP << name << " : Reference : " << ref << ", \tCompute : " << calc << SEP;
    return ss.str();
}

//---------------------------------------------------------------------------------------------------
/// <summary>   Error message for string value. </summary>
/// <inheritdoc cref="ErrorMsg(const std::string&, const T, const T)"/>
inline std::string ErrorMsg(const std::string& name, const std::string& ref, const std::string& calc)
{
    std::stringstream ss;
    ss << SEP << name << " : Reference : " << ref << ", \tCompute : " << calc << SEP;
    return ss.str();
}

//---------------------------------------------------------------------------------------------------
/// <summary>   Error message for CTime value. </summary>
/// <inheritdoc cref="ErrorMsg(const std::string&, const T, const T)"/>
inline std::string ErrorMsg(const std::string& name, const OpenViBE::CTime& ref, const OpenViBE::CTime& calc)
{
    std::stringstream ss;
    ss << SEP << name << " : Reference : " << ref.str(true, true) << ", \tCompute : " << calc.str(true, true) << SEP;
    return ss.str();
}

//---------------------------------------------------------------------------------------------------
/// <summary>   Error message for CIdentifier value. </summary>
/// <inheritdoc cref="ErrorMsg(const std::string&, const T, const T)"/>
inline std::string ErrorMsg(const std::string& name, const OpenViBE::CIdentifier& ref, const OpenViBE::CIdentifier& calc)
{
    std::stringstream ss;
    ss << SEP << name << " : Reference : " << ref << ", \tCompute : " << calc << SEP;
    return ss.str();
}

//---------------------------------------------------------------------------------------------------
/// <summary>   Error message for CIdentifier value. </summary>
/// <inheritdoc cref="ErrorMsg(const std::string&, const T, const T)"/>
inline std::string ErrorMsg(const std::string& name, const OpenViBE::CMatrix& ref, const OpenViBE::CMatrix& calc)
{
    std::stringstream ss;
    ss << SEP << name << " : " << std::endl << "********** Reference **********\n" << ref << std::endl << "********** Compute **********\n" << calc << SEP;
    return ss.str();
}
