///-------------------------------------------------------------------------------------------------
/// 
/// \file CompareCSV.hpp
/// \author Thibaut Monseigne / Inria.
/// \version 2.0.
/// \date 19/04/2022.
/// \remarks Based on code of Alison Cellard / Inria.
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
#include "Compare.hpp"

class CompareCSV final : public Compare
{
    size_t m_last           = 0;
    std::string m_separator = ",";

    std::vector<double> m_buffer1, m_buffer2;

public:
    ///-------------------------------------------------------------------------------------------------
    /// <summary> Initializes a new instance of the <see cref="CompareCSV"/> class. </summary>
    /// <param name="threshold"> The threshold for comparison. </param>
    explicit CompareCSV(const double threshold) : Compare(threshold) {}

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Test two files and compare content (with threshold). </summary>
    /// <param name="file1"> The first file. </param>
    /// <param name="file2"> The second file. </param>
    /// <returns> <c>True</c> if elements are near equals, <c>false</c> otherwise. </returns>
    bool Test(const std::string& file1, const std::string& file2) override;

private:
    ///-------------------------------------------------------------------------------------------------
    /// <summary> Compares the header of the csv. </summary>
    /// <param name="file1"> The first file. </param>
    /// <param name="file2"> The second file. </param>
    /// <returns> <c>True</c> if elements are equals, <c>false</c> otherwise. </returns>
    bool compareHeader(std::ifstream& file1, std::ifstream& file2);

    /// <summary> Compares the datas of the csv. </summary>
    /// <param name="file1"> The first file. </param>
    /// <param name="file2"> The second file. </param>
    /// <returns> <c>True</c> if elements are near equals, <c>false</c> otherwise. </returns>
    bool compareDatas(std::ifstream& file1, std::ifstream& file2);

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Compares two buffers. </summary>
    /// <returns> <c>True</c> if elements are near equals, <c>false</c> otherwise.  </returns>
    bool compareBuffers() const;
};
