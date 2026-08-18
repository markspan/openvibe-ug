///-------------------------------------------------------------------------------------------------
/// 
/// \file ovtAssert.h
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

#include <string>

namespace OpenViBE {
namespace Test {
void printError(const char* expression, const char* message, const char* file, const int line);
void printError(const char* expression, const std::string& message, const char* file, const int line);
void printError(const char* expression, const std::ostream& message, const char* file, const int line);
void printExpressionPair(const char* str1, const char* str2);
std::string buildExpressionFromPair(const char* str1, const char* str2);
}  // namespace Test
}  // namespace OpenViBE

/**
* OVT_ASSERT_PRIV: Assess expression and
* return EXIT_FAILURE if expr is false
* - expr: expression to assess
* - origin: original assessed expression
* - msg: custom error message
* WARNING: SHOULD NOT BE USED DIRECTLY
*/
#define OVT_ASSERT_PRIV(expr, origin, msg)                              \
do {                                                                    \
    if (!(expr)) {                                                      \
        OpenViBE::Test::printError(#origin, (msg), __FILE__, __LINE__); \
        return EXIT_FAILURE;                                            \
    }                                                                   \
} while (0)

/**
* OVT_ASSERT: Assess simple expression
* - expr: expression to assess
* - msg: custom error message
*/
#define OVT_ASSERT(expr, msg) OVT_ASSERT_PRIV((expr), (expr), (msg))

/**
* OVT_ASSERT_STR: Assess string equality
* - str1: reference string
* - str2: compared string
* - msg: custom error message
*/
#define OVT_ASSERT_STREQ(str1, str2, msg)                                                                                       \
do {                                                                                                                            \
    if (!((str1) == (str2))) {                                                                                                  \
        OpenViBE::Test::printError(OpenViBE::Test::buildExpressionFromPair(#str1, #str2).c_str(),(msg), __FILE__, __LINE__);    \
        OpenViBE::Test::printExpressionPair((str1).c_str(),(str2).c_str());                                                     \
        return EXIT_FAILURE;                                                                                                    \
    }                                                                                                                           \
} while (0)

/**
* OVT_ASSERT_EX: Assess expr throws an
* exception
* - expr: expression to assess
* - msg: custom error message
*/
#define OVT_ASSERT_EX(expr, msg)        \
do {                                    \
    bool hasTrown{ false };             \
    try { (expr); }                     \
    catch (...) { hasTrown = true; }    \
    OVT_ASSERT_PRIV(hasTrown, (msg));   \
} while (0)
