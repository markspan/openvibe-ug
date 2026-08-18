///-------------------------------------------------------------------------------------------------
/// 
/// \file test_Classifier.hpp
/// \brief Tests for Classifier Functions.
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

#include <geometry/Classification.hpp>

//---------------------------------------------------------------------------------------------------
class Tests_Classifier : public testing::Test
{
protected:
    std::vector<std::vector<Eigen::RowVectorXd>> m_dataSet;

    void SetUp() override
    {
        const std::vector<Eigen::RowVectorXd> tmp = InitFeaturization::TangentSpace::Reference();
        m_dataSet                                 = Geometry::Vector1DTo2D(tmp, { NB_TRIALS1, NB_TRIALS2 });
    }
};

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Classifier, LSQR)
{
    const Eigen::MatrixXd ref = InitClassif::LSQR::Reference();
    Eigen::MatrixXd calc;
    Geometry::LSQR(m_dataSet, calc);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("LSQR", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Classifier, FgDACompute)
{
    const Eigen::MatrixXd ref = InitClassif::FgDACompute::Reference();
    Eigen::MatrixXd calc;
    Geometry::FgDACompute(m_dataSet, calc);
    EXPECT_TRUE(isAlmostEqual(ref, calc)) << ErrorMsg("FgDA", ref, calc);
}
//---------------------------------------------------------------------------------------------------
