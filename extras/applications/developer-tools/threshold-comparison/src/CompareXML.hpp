///-------------------------------------------------------------------------------------------------
/// 
/// \file CompareXML.hpp
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
#include "Compare.hpp"
#include "tinyxml2.h"

class CompareXML final : public Compare
{
public:
    ///-------------------------------------------------------------------------------------------------
    /// <summary> Initializes a new instance of the <see cref="CompareXML"/> class. </summary>
    /// <param name="threshold"> The threshold for comparison. </param>
    explicit CompareXML(const double threshold) : Compare(threshold) {}

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Test two files and compare content (with threshold). </summary>
    /// <param name="file1"> The first file. </param>
    /// <param name="file2"> The second file. </param>
    /// <returns> <c>True</c> if elements are near equals, <c>false</c> otherwise. </returns>
    bool Test(const std::string& file1, const std::string& file2) override;

private:
    ///-------------------------------------------------------------------------------------------------
    /// <summary> Compares the tag name, attributes and values or childs. </summary>
    /// <param name="node1">The tag1.</param>
    /// <param name="node2">The tag2.</param>
    /// <returns></returns>
    bool compareElement(tinyxml2::XMLElement* node1, tinyxml2::XMLElement* node2);

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Compares the value inside tag. </summary>
    /// <param name="value1">The tag1.</param>
    /// <param name="value2">The tag2.</param>
    /// <returns></returns>
    bool compareValue(const std::string& value1, const std::string& value2) const;
};
