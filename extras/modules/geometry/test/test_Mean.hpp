///-------------------------------------------------------------------------------------------------
/// 
/// \file test_Mean.hpp
/// \brief Tests for Mean Functions.
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

#include <geometry/Mean.hpp>

//---------------------------------------------------------------------------------------------------
class Tests_Means : public testing::Test
{
protected:
    std::vector<Eigen::MatrixXd> m_dataSet;

    void SetUp() override { m_dataSet = Geometry::Vector2DTo1D(InitCovariance::LWF::Reference()); }
};

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, BadInput)
{
    std::vector<Eigen::MatrixXd> bad;
    Eigen::MatrixXd calc;
    EXPECT_FALSE(Mean(bad, calc, Geometry::EMetric::Riemann));
    bad.emplace_back(Eigen::MatrixXd::Zero(1, 2));
    bad.emplace_back(Eigen::MatrixXd::Zero(1, 2));
    EXPECT_FALSE(Mean(bad, calc, Geometry::EMetric::Riemann));
    bad.emplace_back(Eigen::MatrixXd::Zero(2, 2));
    EXPECT_FALSE(Mean(bad, calc, Geometry::EMetric::Riemann));
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, Euclidian)
{
    const Eigen::MatrixXd ref = InitMeans::Euclidian::Reference();
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::Euclidian);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix Euclidian", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, LogEuclidian)
{
    const Eigen::MatrixXd ref = InitMeans::LogEuclidian::Reference();
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::LogEuclidian);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix LogEuclidian", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, Riemann)
{
    const Eigen::MatrixXd ref = InitMeans::Riemann::Reference();
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::Riemann);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix Riemann", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, LogDet)
{
    const Eigen::MatrixXd ref = InitMeans::LogDeterminant::Reference();
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::LogDet);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix LogDet", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, Kullback)
{
    const Eigen::MatrixXd ref = InitMeans::Kullback::Reference();
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::Kullback);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix Kullback", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, Wasserstein)
{
    std::cout << "Precision Error" << std::endl;
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::Wasserstein);
    //const Eigen::MatrixXd ref = InitMeans::Wasserstein::Reference();
    //EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix Wasserstein", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, ALE)
{
    std::cout << "Not implemented" << std::endl;
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::ALE);
    //const Eigen::MatrixXd ref = InitMeans::ALE::Reference();
    //EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix ALE", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, Harmonic)
{
    const Eigen::MatrixXd ref = InitMeans::Harmonic::Reference();
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::Harmonic);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix Harmonic", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Means, Identity)
{
    const Eigen::MatrixXd ref = InitMeans::Identity::Reference();
    Eigen::MatrixXd calc;
    Mean(m_dataSet, calc, Geometry::EMetric::Identity);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Mean Matrix Identity", ref, calc);
}
//---------------------------------------------------------------------------------------------------
