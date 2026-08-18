///-------------------------------------------------------------------------------------------------
/// 
/// \file CompareCSV.cpp
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

#include <cmath> // std::fabs
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "CompareCSV.hpp"

///-------------------------------------------------------------------------------------------------
bool CompareCSV::compareHeader(std::ifstream& file1, std::ifstream& file2)
{
    std::string line1, line2;
    getline(file1, line1);
    getline(file2, line2);

    //********** Check Separator **********
    m_separator                     = ",";
    std::vector<std::string> vLine1 = Split(line1, m_separator), vLine2 = Split(line2, m_separator);

    if (vLine1.size() == 1) {
        m_separator = ";";
        vLine1      = Split(line1, m_separator);
        vLine2      = Split(line2, m_separator);
    }
    if (vLine1.size() == 1) {
        std::cout << "Error : CSV with an unknown separator (different from ',' and ';') or single data." << std::endl;
        return false;
    }

    //********** Check Size **********
    if (vLine1.size() != vLine2.size()) {
        std::cout << "Error : Not the same number of column (" << vLine1.size() << " VS " << vLine2.size() << ")." << std::endl;
        return false;
    }


    //********** Check Columns **********
    // Column possible format
    // Signal       : Time:512Hz,Epoch,Noise 1,Noise 2,Noise 3,Noise 4,Event Id,Event Date,Event Duration
    // Feature      : Time:8,End Time,Feature 1,Feature 2,Feature 3,Feature 4,Feature 5,Feature 6,Feature 7,Feature 8,Event Id,Event Date,Event Duration
    // Spectrum     : Time:3x17:512,End Time,0:0.000000,0:16.000000,0:32.000000,16:0.000000,16:16.000000,16:32.000000,32:0.000000,32:16.000000,32:32.000000,Event Id,Event Date,Event Duration
    // Matrix       : Time:2x4,End Time,Noise 1:,Noise 1:,Noise 1:,Noise 1:,Noise 1:,Noise 1:,Noise 1:,Noise 1:,Noise 2:,Noise 2:,Noise 2:,Noise 2:,Noise 2:,Noise 2:,Noise 2:,Noise 2:,Event Id,Event Date,Event Duration
    // Stimulation  : Event Id,Event Date,Event Duration

    m_last = vLine1.size();

    if (m_last < 3) {
        std::cout << "Error : No Valid column number in CSV (minimum 3). " << std::endl;
        return false;
    }
    // Parsing Row Without 3 end stim column (actual method because gdf read write stimulation bug)
    if (m_last > 3) { m_last -= 3; } // All except stim for signal message

    bool res = true;
    for (size_t i = 0; i < vLine1.size(); ++i) {
        if (vLine1[i] != vLine2[i]) {
            std::cout << "Error : The name for column " << i << " (\"" << vLine1[i] << "\" VS \"" << vLine2[i] << "\")." << std::endl;
            res = false;
        }
    }
    return res;
}

///-------------------------------------------------------------------------------------------------
bool CompareCSV::compareDatas(std::ifstream& file1, std::ifstream& file2)
{
    bool res = true;

    std::string line;
    std::vector<std::vector<std::string>> datas1, datas2;


    //********** Store Datas **********
    while (getline(file1, line)) { datas1.push_back(Split(line, m_separator)); }
    while (getline(file2, line)) { datas2.push_back(Split(line, m_separator)); }
    file1.close();
    file2.close();

    //********** Compare Row Number **********
    if (datas1.size() != datas2.size()) {
        std::cout << "Error : Files have different number of rows, check input data" << std::endl;
        return false;
    }

    //********** Parsing Rows **********
    m_buffer1.clear();
    m_buffer2.clear();
    for (size_t i = 0; i < datas1.size(); ++i) {
        auto& row1 = datas1[i];
        auto& row2 = datas2[i];
        if (row1.size() != row2.size()) {
            std::cout << "Error : Row " << i << " have different sizes (" << row1.size() << " VS " << row2.size() << ")." << std::endl;
            res = false;
        }
        else if (!row1.empty()) {
            for (size_t j = 0; j < m_last; ++j) {
                const bool hasValue1 = !row1[j].empty();
                const bool hasValue2 = !row2[j].empty();
                if (hasValue1 != hasValue2) {
                    std::cout << "Error : Value " << i << ", " << j << " is " << (hasValue1 ? "not" : "") << " empty in first file"
                            << " and is " << (hasValue2 ? "not" : "") << " empty in second file." << std::endl;
                    res = false;
                }
                else if (hasValue1) {
                    m_buffer1.push_back(stod(row1[j]));
                    m_buffer2.push_back(stod(row2[j]));
                }
            }
        }
    }

    /*
    // Parsing Entire Row
    for (const auto& row : data) {
        for (size_t i = 0; i < columnLast - 3; ++i) { if (!row[i].empty()) { output.push_back(stod(row[i])); } } // All except stim
        for (size_t i = columnLast - 3; i < columnLast; ++i) {
            if (!row[i].empty()) {
                auto tmp = split(row[i], ":");  //Line for all except stimulation file can have multiple stimulation
                for (size_t j = 0; j < tmp.size(); ++j) { output.push_back(stod(tmp[j])); }
            }
        }
    }
    */

    std::cout << "Found " << datas1.size() << " rows with " << datas1[0].size() << " columns (" << m_last << " use) give " << m_buffer1.size() << " values." <<
            std::endl;

    return res;
}

///-------------------------------------------------------------------------------------------------
bool CompareCSV::compareBuffers() const
{
    size_t errorCount = 0, errorIdx = 0;
    double errorMax   = 0;
    std::vector<double> difference;

    //********** Check Differences **********
    for (size_t i = 0, n = m_buffer1.size(); i < n; ++i) {
        const double err = std::fabs(m_buffer1[i] - m_buffer2[i]);
        if (err > m_threshold) {
            errorCount++;
            if (err > errorMax) {
                errorMax = err;
                errorIdx = i;
            }
            difference.push_back(err);
        }
    }

    //********** Compute Stats in case of errors **********
    if (errorCount != 0) {
        double mean = 0, var = 0;
        for (const auto& diff : difference) {
            mean += diff;
            var += diff * diff;
        }
        mean /= double(errorCount);
        var /= double(errorCount);
        var -= mean * mean;
        std::cout << "Comparison failed, " << errorCount << " datas differ, the largest difference is " << errorMax
                << " at value [" << errorIdx << "]: " << m_buffer1[errorIdx] << " VS " << m_buffer2[errorIdx] << std::endl
                << "Error Mean : " << mean << "\t Error Variance : " << var << std::endl;
        return false;
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CompareCSV::Test(const std::string& file1, const std::string& file2)
{
    // Open files
    std::ifstream iFile1(file1, std::ifstream::in);
    std::ifstream iFile2(file2, std::ifstream::in);

    if (!iFile1.is_open()) {
        std::cout << "Error opening file [" << file1 << "] for reading" << std::endl;
        return false;
    }
    if (!iFile2.is_open()) {
        std::cout << "Error opening file [" << file2 << "] for reading" << std::endl;
        return false;
    }

    // Compare Header
    if (!compareHeader(iFile1, iFile2)) { return false; }

    // Compare Datas
    if (!compareDatas(iFile1, iFile2)) { return false; }

    // Compare Buffers
    if (!compareBuffers()) { return false; }

    return true;
}
