///-------------------------------------------------------------------------------------------------
/// 
/// \file test_Median.hpp
/// \brief Tests for Median Functions.
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

#include "init.hpp"
#include "misc.hpp"

#include <geometry/Basics.hpp>
#include <geometry/Median.hpp>

//---------------------------------------------------------------------------------------------------
class Tests_Median : public testing::Test
{
protected:
    std::vector<Eigen::MatrixXd> m_dataSet;

    void SetUp() override { m_dataSet = Geometry::Vector2DTo1D(InitCovariance::LWF::Reference()); }
};

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Median, Simple_Median)
{
    std::vector<double> v{ 5, 6, 4, 3, 2, 6, 7, 9, 3 };
    double calc = Geometry::Median(v);
    EXPECT_EQ(calc, 5);

    v.pop_back();
    calc = Geometry::Median(v);
    EXPECT_EQ(calc, 5.5);

    Eigen::MatrixXd m(3, 3);
    m << 5, 6, 4, 3, 2, 6, 7, 9, 3;
    calc = Geometry::Median(m);
    EXPECT_EQ(calc, 5);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Median, Euclidian)
{
    Eigen::MatrixXd calc;
    Eigen::MatrixXd ref(3, 3);
    ref << 1.749537973777478, 0.002960131606861, 0.020507254841909,
            0.002960131606861, 1.754563395557952, 0.043042786354499,
            0.020507254841909, 0.043042786354499, 1.057672472691352;
    EXPECT_TRUE(Geometry::Median(m_dataSet, calc, 0.0001, 50, Geometry::EMetric::Euclidian)) << "Error During Median Computing";
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Euclidian Median of Dataset", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Median, Riemann)
{
    Eigen::MatrixXd calc;
    Eigen::MatrixXd ref(3, 3);
    ref << 1.851330747504982, 0.002002346316770, 0.022122030618131,
            0.002002346316770, 1.644242996651016, 0.033655563302757,
            0.022122030618131, 0.033655563302757, 0.851184143800763;
    EXPECT_TRUE(Geometry::Median(m_dataSet, calc, 0.0001, 50, Geometry::EMetric::Riemann)) << "Error During Median Computes";
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Riemann Median of Dataset", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Median, Identity)
{
    const Eigen::MatrixXd ref = InitMeans::Identity::Reference();
    Eigen::MatrixXd calc;
    EXPECT_TRUE(Geometry::Median(m_dataSet, calc, 0.0001, 50, Geometry::EMetric::Identity)) << "Error During Median Computes";
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("Identity Median of Dataset", ref, calc);
}
//---------------------------------------------------------------------------------------------------
