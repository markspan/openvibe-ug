///-------------------------------------------------------------------------------------------------
/// 
/// \file ovtAssert.cpp
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

#include <iostream>

#include "ovtAssert.h"

namespace OpenViBE {
namespace Test {
static void printErrorCore(const char* expression, const char* file, const int line)
{
    std::cerr << "Failed to evaluate: " << expression << std::endl;
    std::cerr << "File = " << file << std::endl;
    std::cerr << "Line = " << line << std::endl;
}

void printError(const char* expression, const char* message, const char* file, const int line)
{
    printErrorCore(expression, file, line);
    std::cerr << "Error message: " << message << std::endl;
}

void printError(const char* expression, const std::string& message, const char* file, const int line) { printError(expression, message.c_str(), file, line); }

void printError(const char* expression, const std::ostream& message, const char* file, const int line)
{
    printErrorCore(expression, file, line);
    std::cerr << "Error message: " << message.rdbuf() << std::endl;
}

void printExpressionPair(const char* str1, const char* str2)
{
    std::cerr << "Expression 1 is : " << str1 << std::endl;
    std::cerr << "Expression 2 is : " << str2 << std::endl;
}

std::string buildExpressionFromPair(const char* str1, const char* str2)
{
    std::string expression = "( " + std::string(str1);
    expression += " = ";
    expression += std::string(str2) + " )";

    return expression;
}
}  // namespace Test
}  // namespace OpenViBE
