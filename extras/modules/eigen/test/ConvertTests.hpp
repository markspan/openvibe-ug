///-------------------------------------------------------------------------------------------------
/// 
/// \file ConvertTests.hpp
/// \brief Tests for Eigen Convert functions.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 29/07/2020.
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
/// \remarks We use the EEglab Matlab plugin to compare result for validation
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "gtest/gtest.h"
#include "eigen/convert.hpp"

//---------------------------------------------------------------------------------------------------
class Convert_Tests : public testing::Test
{
protected:
    Eigen::MatrixXd m_eigenMatrix;
    Eigen::RowVectorXd m_eigenRowVector;
    OpenViBE::CMatrix m_ovMatrix;
    OpenViBE::CMatrix m_ovRowMatrix;
    std::vector<double> m_vector;
    const Eigen::Index m_row  = 2;
    const Eigen::Index m_col  = 2;
    const Eigen::Index m_size = 4;

    void SetUp() override
    {
        m_eigenMatrix.resize(2, 2);
        m_eigenMatrix << 1, 2, 3, 4;
        m_eigenRowVector.resize(4);
        m_eigenRowVector << 1, 2, 3, 4;
        m_vector = { 1, 2, 3, 4 };
        m_ovMatrix.resize(2, 2);
        m_ovMatrix.setBuffer(m_vector);
        m_ovRowMatrix.resize(4);
        m_ovRowMatrix.setBuffer(m_vector);
    }
};

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Convert_Tests, Conversions)
{
    //----- OpenViBE::CMatrix -> Eigen::MatrixXd -----
    Eigen::MatrixXd resEigenMatrix;
    EXPECT_FALSE(OpenViBE::MatrixConvert(m_ovRowMatrix, resEigenMatrix)) << "Row OpenViBE Matrix can't be convert to Eigen Matrix";
    EXPECT_TRUE(OpenViBE::MatrixConvert(m_ovMatrix, resEigenMatrix)) << "Error During Conversion";
    for (Eigen::Index i = 0, idx = 0; i < m_row; ++i) {
        for (Eigen::Index j = 0; j < m_col; ++j, ++idx) {
            EXPECT_DOUBLE_EQ(m_ovMatrix[idx], resEigenMatrix(i,j))
                << "Matrix conversion OV 2 Eigen buffer " << idx << " doesn't match : " << resEigenMatrix(i, j) << " instead " << m_ovMatrix[idx];
        }
    }

    //----- Eigen::MatrixXd -> OpenViBE::CMatrix -----
    const Eigen::MatrixXd dumpEigenMatrix;
    OpenViBE::CMatrix resOvMatrix;
    EXPECT_FALSE(OpenViBE::MatrixConvert(dumpEigenMatrix, resOvMatrix)) << "Empty Eigen Matrix can't be convert to OpenViBE Matrix.";
    EXPECT_TRUE(OpenViBE::MatrixConvert(m_eigenMatrix, resOvMatrix)) << "Error During Conversion";
    for (Eigen::Index i = 0, idx = 0; i < m_row; ++i) {
        for (Eigen::Index j = 0; j < m_col; ++j, ++idx) {
            EXPECT_DOUBLE_EQ(m_ovMatrix[idx], resEigenMatrix(i, j))
                << "Matrix conversion Eigen 2 OV buffer " << idx << " doesn't match : " << resOvMatrix[idx] << " instead " << m_eigenMatrix(i, j);
        }
    }

    //----- Eigen::RowVectorXd -> OpenViBE::CMatrix -----
    const Eigen::RowVectorXd dumpEigenRowMatrix;
    OpenViBE::CMatrix resOvRowMatrix;
    EXPECT_FALSE(OpenViBE::MatrixConvert(dumpEigenRowMatrix, resOvRowMatrix)) << "Empty Eigen Row Vector can't be convert to OpenViBE Row Matrix.";
    EXPECT_TRUE(OpenViBE::MatrixConvert(m_eigenRowVector, resOvRowMatrix)) << "Error During Conversion";
    for (Eigen::Index i = 0; i < m_size; ++i) {
        EXPECT_DOUBLE_EQ(m_eigenRowVector(i), resOvRowMatrix[i])
            << "Matrix conversion Eigen 2 OV buffer " << i << " doesn't match : " << resOvRowMatrix[i] << " instead " << m_eigenRowVector(i);
    }

    //----- OpenViBE::CMatrix -> Eigen::RowVectorXd -----
    Eigen::RowVectorXd eigenRowMatrix;
    EXPECT_FALSE(OpenViBE::MatrixConvert(m_ovMatrix, eigenRowMatrix)) << "OpenViBE Matrix can't be convert to Eigen Row Vector";
    EXPECT_TRUE(OpenViBE::MatrixConvert(m_ovRowMatrix, eigenRowMatrix)) << "Error During Conversion";
    for (Eigen::Index i = 0; i < m_size; ++i) {
        EXPECT_DOUBLE_EQ(m_ovRowMatrix[i], eigenRowMatrix(i))
            << "Matrix conversion  OV 2 Eigen buffer " << i << " doesn't match : " << eigenRowMatrix(i) << " instead " << m_ovRowMatrix[i];
    }

    //----- std::vector<double> -> OpenViBE::CMatrix -----
    const std::vector<double> dumpVector;
    OpenViBE::CMatrix resOvVectorMatrix;
    EXPECT_FALSE(OpenViBE::MatrixConvert(dumpVector, resOvVectorMatrix)) << "Empty Vector can't be convert to OpenViBE Row Matrix.";
    EXPECT_TRUE(OpenViBE::MatrixConvert(m_vector, resOvVectorMatrix)) << "Error During Conversion";
    for (Eigen::Index i = 0; i < m_size; ++i) {
        EXPECT_DOUBLE_EQ(m_vector[i], resOvVectorMatrix[i])
            << "Matrix conversion Vector 2 OV buffer " << i << " doesn't match : " << resOvVectorMatrix[i] << " instead " << m_vector[i];
    }
}
//---------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------
TEST_F(Convert_Tests, Conversions3D)
{
    //----- Object Initialisation -----
    std::vector<Eigen::MatrixXd> in;
    in.resize(2);
    in[0].resize(3, 4);
    in[0] << 0, 1, 2, 3,
            4, 5, 6, 7,
            8, 9, 10, 11;
    in[1].resize(3, 4);
    in[1] << 12, 13, 14, 15,
            16, 17, 18, 19,
            20, 21, 22, 23;

    OpenViBE::CMatrix ref0, ref1, ref2, ref3, ref4, ref5;
    ref0.resize({ 2, 3, 4 });
    ref0.setBuffer(std::vector<double>{
        0, 1, 2, 3,
        4, 5, 6, 7,
        8, 9, 10, 11,

        12, 13, 14, 15,
        16, 17, 18, 19,
        20, 21, 22, 23
    });
    ref1.resize({ 2, 4, 3 });
    ref1.setBuffer(std::vector<double>{
        0, 4, 8,
        1, 5, 9,
        2, 6, 10,
        3, 7, 11,

        12, 16, 20,
        13, 17, 21,
        14, 18, 22,
        15, 19, 23
    });
    ref2.resize({ 3, 2, 4 });
    ref2.setBuffer(std::vector<double>{
        0, 1, 2, 3,
        12, 13, 14, 15,

        4, 5, 6, 7,
        16, 17, 18, 19,

        8, 9, 10, 11,
        20, 21, 22, 23
    });
    ref3.resize({ 3, 4, 2 });
    ref3.setBuffer(std::vector<double>{
        0, 12,
        1, 13,
        2, 14,
        3, 15,

        4, 16,
        5, 17,
        6, 18,
        7, 19,

        8, 20,
        9, 21,
        10, 22,
        11, 23
    });
    ref4.resize({ 4, 2, 3 });
    ref4.setBuffer(std::vector<double>{
        0, 4, 8,
        12, 16, 20,

        1, 5, 9,
        13, 17, 21,

        2, 6, 10,
        14, 18, 22,

        3, 7, 11,
        15, 19, 23
    });
    ref5.resize({ 4, 3, 2 });
    ref5.setBuffer(std::vector<double>{
        0, 12,
        4, 16,
        8, 20,

        1, 13,
        5, 17,
        9, 21,

        2, 14,
        6, 18,
        10, 22,

        3, 15,
        7, 19,
        11, 23
    });


    OpenViBE::CMatrix out0, out1, out2, out3, out4, out5;
    EXPECT_TRUE(MatrixConvert(in, out0, 0)) << "Error During Conversion";
    EXPECT_TRUE(MatrixConvert(in, out1, 1)) << "Error During Conversion";
    EXPECT_TRUE(MatrixConvert(in, out2, 2)) << "Error During Conversion";
    EXPECT_TRUE(MatrixConvert(in, out3, 3)) << "Error During Conversion";
    EXPECT_TRUE(MatrixConvert(in, out4, 4)) << "Error During Conversion";
    EXPECT_TRUE(MatrixConvert(in, out5, 5)) << "Error During Conversion";

    EXPECT_TRUE(ref0 == out0) << "Conversion Buffer doesn't match";
    EXPECT_TRUE(ref1 == out1) << "Conversion Buffer doesn't match";
    EXPECT_TRUE(ref2 == out2) << "Conversion Buffer doesn't match";
    EXPECT_TRUE(ref3 == out3) << "Conversion Buffer doesn't match";
    EXPECT_TRUE(ref4 == out4) << "Conversion Buffer doesn't match";
    EXPECT_TRUE(ref5 == out5) << "Conversion Buffer doesn't match";
}
//---------------------------------------------------------------------------------------------------
