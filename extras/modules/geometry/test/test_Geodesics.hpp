///-------------------------------------------------------------------------------------------------
/// 
/// \file test_Geodesics.hpp
/// \brief Tests for Geodesic Functions.
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

#include <geometry/Geodesic.hpp>

//---------------------------------------------------------------------------------------------------
class Tests_Geodesic : public testing::Test
{
protected:
    std::vector<Eigen::MatrixXd> m_dataSet;

    void SetUp() override { m_dataSet = Geometry::Vector2DTo1D(InitCovariance::LWF::Reference()); }
};

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Geodesic, Euclidian)
{
    const std::vector<Eigen::MatrixXd> ref = InitGeodesics::Euclidian::Reference();
    const Eigen::MatrixXd mean             = InitMeans::Euclidian::Reference();
    for (size_t i = 0; i < m_dataSet.size(); ++i) {
        Eigen::MatrixXd calc;
        Geodesic(mean, m_dataSet[i], calc, Geometry::EMetric::Euclidian, 0.5);
        EXPECT_TRUE(isAlmostEqual(ref[i], calc)) << ErrorMsg("Geodesic Euclidian Sample [" + std::to_string(i) + "]", ref[i], calc);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Geodesic, LogEuclidian)
{
    const std::vector<Eigen::MatrixXd> ref = InitGeodesics::LogEuclidian::Reference();
    const Eigen::MatrixXd mean             = InitMeans::LogEuclidian::Reference();
    for (size_t i = 0; i < m_dataSet.size(); ++i) {
        Eigen::MatrixXd calc;
        Geodesic(mean, m_dataSet[i], calc, Geometry::EMetric::LogEuclidian, 0.5);
        EXPECT_TRUE(isAlmostEqual(ref[i], calc)) << ErrorMsg("Geodesic LogEuclidian Sample [" + std::to_string(i) + "]", ref[i], calc);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Geodesic, Riemann)
{
    const std::vector<Eigen::MatrixXd> ref = InitGeodesics::Riemann::Reference();
    const Eigen::MatrixXd mean             = InitMeans::Riemann::Reference();
    for (size_t i = 0; i < m_dataSet.size(); ++i) {
        Eigen::MatrixXd calc;
        Geodesic(mean, m_dataSet[i], calc, Geometry::EMetric::Riemann, 0.5);
        EXPECT_TRUE(isAlmostEqual(ref[i], calc)) << ErrorMsg("Geodesic Riemann Sample [" + std::to_string(i) + "]", ref[i], calc);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Geodesic, Identity)
{
    const Eigen::MatrixXd mean = InitMeans::Riemann::Reference(), ref = Eigen::MatrixXd::Identity(NB_CHAN, NB_CHAN);
    for (size_t i = 0; i < m_dataSet.size(); ++i) {
        Eigen::MatrixXd calc;
        Geodesic(mean, m_dataSet[i], calc, Geometry::EMetric::Identity, 0.5);
        EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Geodesic Identity Sample [" + std::to_string(i) + "]", ref, calc);
    }
}
//---------------------------------------------------------------------------------------------------
