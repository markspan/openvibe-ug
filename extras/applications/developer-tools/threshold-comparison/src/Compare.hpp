///-------------------------------------------------------------------------------------------------
/// 
/// \file Compare.hpp
/// \author Thibaut Monseigne / Inria.
/// \version 2.0.
/// \date 19/04/2022.
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
#include <vector>
#include <boost/algorithm/string.hpp>

class Compare
{
protected:
    double m_threshold = 0.0001;

public:
    virtual ~Compare() = default;

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Initializes a new instance of the <see cref="Compare"/> class. </summary>
    /// <param name="threshold"> The threshold for comparison. </param>
    explicit Compare(const double threshold) : m_threshold(threshold) {}

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Test two files and compare content (with threshold). </summary>
    /// <param name="file1"> The first file. </param>
    /// <param name="file2"> The second file. </param>
    /// <returns> <c>True</c> if elements are near equals, <c>false</c> otherwise. </returns>
    virtual bool Test(const std::string& file1, const std::string& file2) = 0;

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Splits the specified string with selected separator. </summary>
    /// <param name="s">The string.</param>
    /// <param name="sep">The separator.</param>
    /// <returns> Vector of splitted elements </returns>
    static std::vector<std::string> Split(const std::string& s, const std::string& sep)
    {
        std::vector<std::string> result;
        std::string::size_type i       = 0, j;
        const std::string::size_type n = sep.size();

        while ((j = s.find(sep, i)) != std::string::npos) {
            result.emplace_back(s, i, j - i);           // Add part
            i = j + n;                                  // Update pos
        }
        result.emplace_back(s, i, s.size() - 1 - i);    // Last without \n
        return result;
    }

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Replace by space in the string all line break tabulation and double space. </summary>
    /// <param name="s">The string to change. </param>
    static void RemoveSpecial(std::string& s)
    {
        boost::replace_all(s, "\r\n", " "); // Windows Line Break
        boost::replace_all(s, "\n", " ");   // Classic Line Break
        boost::replace_all(s, "\t", " ");   // Tabulation
        size_t size = 0;
        while (size != s.size()) {
            size = s.size();
            boost::replace_all(s, "  ", " ");   // Double Space
        }
    }

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Determines whether the specified string is number or group of number (with space). </summary>
    /// <param name="s"> The string to test. </param>
    /// <returns> <c>true</c> if the specified s is number; otherwise, <c>false</c>. </returns>
    static bool IsNumber(const std::string& s) { return s.find_first_not_of("0123456789e+-. ") == std::string::npos; }
};
