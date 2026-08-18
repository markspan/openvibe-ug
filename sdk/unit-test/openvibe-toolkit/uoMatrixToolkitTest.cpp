///-------------------------------------------------------------------------------------------------
/// 
/// \file uoMatrixToolkitTest.cpp
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
#include <random>

#include <toolkit/ovtk_all.h>

#include "ovtAssert.h"

static std::default_random_engine gen(777);
static std::uniform_real_distribution<double> dist(0.0, 100.0);

void fillMatrix(OpenViBE::CMatrix& matrix)
{
    for (size_t i = 0; i < matrix.getDimensionCount(); ++i) {
        for (size_t j = 0; j < matrix.getDimensionSize(i); ++j) {
            std::stringstream label;
            label << "Label " << j + 1 << " of Dimension " << i + 1;
            matrix.setDimensionLabel(i, j, label.str());
        }
    }

    for (size_t i = 0; i < matrix.getBufferElementCount(); ++i) { matrix.getBuffer()[i] = dist(gen); }
}
bool testMatrix(OpenViBE::CMatrix& expectedMatrix, const std::string& textFile, const size_t precision = 6)
{
    const double threshold = 1.0 / std::pow(10.0, double(precision - 2));

    fillMatrix(expectedMatrix);

    if (!OpenViBE::Toolkit::Matrix::saveToTextFile(expectedMatrix, textFile.c_str(), precision)) {
        std::cerr << "Error: saving matrix to file " << textFile << "\n";
        return false;
    }

    OpenViBE::CMatrix resultMatrix;

    if (!OpenViBE::Toolkit::Matrix::loadFromTextFile(resultMatrix, textFile.c_str())) {
        std::cerr << "Error: loading matrix from file " << textFile << "\n";
        return false;
    }
    if (!expectedMatrix.isDescriptionEqual(resultMatrix)) {
        std::cerr << "Error: Descriptions differ between expected matrix and result matrix after save/load\n";
        return false;
    }

    for (size_t i = 0; i < expectedMatrix.getBufferElementCount(); ++i) {
        const double error = std::fabs(expectedMatrix.getBuffer()[i] - resultMatrix.getBuffer()[i]);

        if (error > threshold) {
            std::cerr << "Error: Data differs at index " << i << ", error " << error << " (thresold = " << threshold << ")\n";
            return false;
        }
    }

    return true;
}

int uoMatrixToolkitTest(int argc, char* argv[])
{
    OVT_ASSERT(argc == 2, "Failure to retrieve tests arguments. Expecting: output_dir");

    const std::string oMatrixFile = std::string(argv[1]) + "uoMatrixToolkitTest.txt";

    OpenViBE::CMatrix source;

    source.resize(1);
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [1; {0,1}]");

    source.resize(5);
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [1; {0,5}]");

    source.resize(1, 1);
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [2; {0,1},{1,1}]");

    source.resize(1, 7);
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [2; {0,1},{1,7}]");

    source.resize(9, 1);
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [2; {0,9},{1,1}]");

    source.resize(2, 4);
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [2; {0,2},{1,4}]");

    source.resize(3, 15);
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [2; {0,3},{1,15}]");

    source.resize({ 1, 1, 1 });
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [3; {0,1},{1,1},{2,1}]");

    source.resize({ 1, 1, 5 });
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [3; {0,1},{1,1},{2,5}]");

    source.resize({ 2, 3, 6 });
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [3; {0,2},{1,3},{2,6}]");

    source.resize({ 9, 5, 2, 3 });
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [4; {0,9},{1,5},{2,2},{3,3}]");

    // special cases at boundaries
    source.resize(0, 0);
    OVT_ASSERT(testMatrix(source, oMatrixFile), "Failed to test matrix with parameters [dimension_count; dimension_size] = [2; {0,0},{1,0}]");

    OpenViBE::CMatrix emptySource;
    OVT_ASSERT(!testMatrix(emptySource, oMatrixFile), "Failed to test matrix with no parameter");

    return EXIT_SUCCESS;
}

//==========================End OF File==============================
