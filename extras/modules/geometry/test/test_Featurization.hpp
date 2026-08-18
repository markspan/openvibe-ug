///-------------------------------------------------------------------------------------------------
/// 
/// \file test_Featurization.hpp
/// \brief Tests for Matrix Featurization Functions.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 09/01/2019.
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

#include "gtest/gtest.h"
#include "misc.hpp"
#include "init.hpp"

#include <geometry/Featurization.hpp>

//---------------------------------------------------------------------------------------------------
class Tests_Featurization : public testing::Test
{
protected:
    std::vector<Eigen::MatrixXd> m_dataSet;

    void SetUp() override { m_dataSet = Geometry::Vector2DTo1D(InitCovariance::LWF::Reference()); }
};

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Featurization, TangentSpace)
{
    const std::vector<Eigen::RowVectorXd> ref = InitFeaturization::TangentSpace::Reference();
    const Eigen::MatrixXd mean                = InitMeans::Riemann::Reference();
    for (size_t i = 0; i < m_dataSet.size(); ++i) {
        Eigen::RowVectorXd calc;
        EXPECT_TRUE(Geometry::Featurization(m_dataSet[i], calc, true, mean)) << "Error During Processing";
        EXPECT_TRUE(isAlmostEqual(ref[i], calc)) << ErrorMsg("TangentSpace Sample [" + std::to_string(i) + "]", ref[i], calc);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Featurization, UnTangentSpace)
{
    const std::vector<Eigen::RowVectorXd> ref = InitFeaturization::TangentSpace::Reference();
    const Eigen::MatrixXd mean                = InitMeans::Riemann::Reference();
    for (size_t i = 0; i < m_dataSet.size(); ++i) {
        Eigen::MatrixXd calc;
        EXPECT_TRUE(Geometry::UnFeaturization(ref[i], calc, true, mean)) << "Error During Processing";
        EXPECT_TRUE(isAlmostEqual(m_dataSet[i], calc)) << ErrorMsg("UnTangentSpace Sample [" + std::to_string(i) + "]", m_dataSet[i], calc);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Featurization, Squeeze)
{
    const std::vector<Eigen::RowVectorXd> ref     = InitFeaturization::Squeeze::Reference();
    const std::vector<Eigen::RowVectorXd> refDiag = InitFeaturization::SqueezeDiag::Reference();
    const Eigen::MatrixXd mean                    = InitMeans::Riemann::Reference();
    for (size_t i = 0; i < m_dataSet.size(); ++i) {
        Eigen::RowVectorXd calc;
        EXPECT_TRUE(Geometry::Featurization(m_dataSet[i], calc, false, mean)) << "Error During Processing";
        EXPECT_TRUE(isAlmostEqual(ref[i], calc)) << ErrorMsg("Squeeze Sample [" + std::to_string(i) + "]", ref[i], calc);
        EXPECT_TRUE(Geometry::SqueezeUpperTriangle(m_dataSet[i], calc, false)) << "Error During Processing";
        EXPECT_TRUE(isAlmostEqual(refDiag[i], calc)) << ErrorMsg("Squeeze Sample [" + std::to_string(i) + "]", refDiag[i], calc);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Featurization, UnSqueeze)
{
    const std::vector<Eigen::RowVectorXd> ref     = InitFeaturization::Squeeze::Reference();
    const std::vector<Eigen::RowVectorXd> refDiag = InitFeaturization::SqueezeDiag::Reference();
    const Eigen::MatrixXd mean                    = InitMeans::Riemann::Reference();
    for (size_t i = 0; i < m_dataSet.size(); ++i) {
        Eigen::MatrixXd calc;
        EXPECT_TRUE(Geometry::UnFeaturization(ref[i], calc, false, mean)) << "Error During Processing";
        EXPECT_TRUE(isAlmostEqual(m_dataSet[i], calc)) << ErrorMsg("UnSqueeze Sample [" + std::to_string(i) + "]", m_dataSet[i], calc);
        EXPECT_TRUE(Geometry::UnSqueezeUpperTriangle(refDiag[i], calc, false)) << "Error During Processing";
        EXPECT_TRUE(isAlmostEqual(m_dataSet[i], calc)) << ErrorMsg("UnSqueeze Sample [" + std::to_string(i) + "]", m_dataSet[i], calc);
    }
}
//---------------------------------------------------------------------------------------------------
