///-------------------------------------------------------------------------------------------------
/// 
/// \file test_Covariance.hpp
/// \brief Tests for Covariance Matrix Functions.
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

#include <geometry/Covariance.hpp>

//---------------------------------------------------------------------------------------------------
class Tests_Covariances : public testing::Test
{
protected:
    std::vector<std::vector<Eigen::MatrixXd>> m_dataSet;

    void SetUp() override { m_dataSet = InitDataset::Dataset(); }
};

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Covariances, Covariance_Matrix_COR)
{
    const std::vector<std::vector<Eigen::MatrixXd>> ref = InitCovariance::COR::Dataset();
    std::vector<std::vector<Eigen::MatrixXd>> calc;
    calc.resize(m_dataSet.size());
    for (size_t k = 0; k < m_dataSet.size(); ++k) {
        calc[k].resize(m_dataSet[k].size());
        for (size_t i = 0; i < m_dataSet[k].size(); ++i) {
            CovarianceMatrix(m_dataSet[k][i], calc[k][i], Geometry::EEstimator::COR, Geometry::EStandardization::None);
            const std::string title = "Covariance Matrix COR Sample [" + std::to_string(k) + "][" + std::to_string(i) + "]";
            EXPECT_TRUE(isAlmostEqual(ref[k][i], calc[k][i])) << ErrorMsg(title, ref[k][i], calc[k][i]);
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Covariances, Covariance_Matrix_COV)
{
    const std::vector<std::vector<Eigen::MatrixXd>> ref = InitCovariance::COV::Dataset();
    std::vector<std::vector<Eigen::MatrixXd>> calc;
    calc.resize(m_dataSet.size());
    for (size_t k = 0; k < m_dataSet.size(); ++k) {
        calc[k].resize(m_dataSet[k].size());
        for (size_t i = 0; i < m_dataSet[k].size(); ++i) {
            CovarianceMatrix(m_dataSet[k][i], calc[k][i], Geometry::EEstimator::COV, Geometry::EStandardization::None);
            const std::string title = "Covariance Matrix COV Sample [" + std::to_string(k) + "][" + std::to_string(i) + "]";
            EXPECT_TRUE(isAlmostEqual(ref[k][i], calc[k][i])) << ErrorMsg(title, ref[k][i], calc[k][i]);
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Covariances, Covariance_Matrix_LWF)
{
    const std::vector<std::vector<Eigen::MatrixXd>> ref = InitCovariance::LWF::Reference();
    std::vector<std::vector<Eigen::MatrixXd>> calc;
    calc.resize(m_dataSet.size());
    for (size_t k = 0; k < m_dataSet.size(); ++k) {
        calc[k].resize(m_dataSet[k].size());
        for (size_t i = 0; i < m_dataSet[k].size(); ++i) {
            CovarianceMatrix(m_dataSet[k][i], calc[k][i], Geometry::EEstimator::LWF, Geometry::EStandardization::Center);
            const std::string title = "Covariance Matrix LWF Sample [" + std::to_string(k) + "][" + std::to_string(i) + "]";
            EXPECT_TRUE(isAlmostEqual(ref[k][i], calc[k][i])) << ErrorMsg(title, ref[k][i], calc[k][i]);
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Covariances, Covariance_Matrix_MCD)
{
    std::cout << "Not implemented" << std::endl;
    std::vector<std::vector<Eigen::MatrixXd>> calc;
    //std::vector<std::vector<Eigen::MatrixXd>> ref = InitCovariance::MCD::Reference();
    calc.resize(m_dataSet.size());
    for (size_t k = 0; k < m_dataSet.size(); ++k) {
        calc[k].resize(m_dataSet[k].size());
        for (size_t i = 0; i < m_dataSet[k].size(); ++i) {
            CovarianceMatrix(m_dataSet[k][i], calc[k][i], Geometry::EEstimator::MCD, Geometry::EStandardization::Center);
            //const std::string title = "Covariance Matrix MCD Sample [" + std::to_string(k) + "][" + std::to_string(i) + "]";
            //EXPECT_TRUE(isAlmostEqual(ref[k][i], calc[k][i])) << ErrorMsg(title, ref[k][i], calc[k][i]);
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Covariances, Covariance_Matrix_OAS)
{
    const std::vector<std::vector<Eigen::MatrixXd>> ref = InitCovariance::OAS::Reference();
    std::vector<std::vector<Eigen::MatrixXd>> calc;
    calc.resize(m_dataSet.size());
    for (size_t k = 0; k < m_dataSet.size(); ++k) {
        calc[k].resize(m_dataSet[k].size());
        for (size_t i = 0; i < m_dataSet[k].size(); ++i) {
            CovarianceMatrix(m_dataSet[k][i], calc[k][i], Geometry::EEstimator::OAS, Geometry::EStandardization::Center);
            const std::string title = "Covariance Matrix OAS Sample [" + std::to_string(k) + "][" + std::to_string(i) + "]";
            EXPECT_TRUE(isAlmostEqual(ref[k][i], calc[k][i])) << ErrorMsg(title, ref[k][i], calc[k][i]);
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Covariances, Covariance_Matrix_SCM)
{
    const std::vector<std::vector<Eigen::MatrixXd>> ref = InitCovariance::SCM::Reference();
    std::vector<std::vector<Eigen::MatrixXd>> calc;
    calc.resize(m_dataSet.size());
    for (size_t k = 0; k < m_dataSet.size(); ++k) {
        calc[k].resize(m_dataSet[k].size());
        for (size_t i = 0; i < m_dataSet[k].size(); ++i) {
            CovarianceMatrix(m_dataSet[k][i], calc[k][i], Geometry::EEstimator::SCM, Geometry::EStandardization::None);
            const std::string title = "Covariance Matrix SCM Sample [" + std::to_string(k) + "][" + std::to_string(i) + "]";
            EXPECT_TRUE(isAlmostEqual(ref[k][i], calc[k][i])) << ErrorMsg(title, ref[k][i], calc[k][i]);
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_Covariances, Covariance_Matrix_IDE)
{
    std::vector<std::vector<Eigen::MatrixXd>> calc;
    const Eigen::MatrixXd ref = Eigen::MatrixXd::Identity(NB_CHAN, NB_CHAN);
    calc.resize(m_dataSet.size());
    for (size_t k = 0; k < m_dataSet.size(); ++k) {
        calc[k].resize(m_dataSet[k].size());
        for (size_t i = 0; i < m_dataSet[k].size(); ++i) {
            CovarianceMatrix(m_dataSet[k][i], calc[k][i], Geometry::EEstimator::IDE, Geometry::EStandardization::None);
            const std::string title = "Covariance Matrix IDE Sample [" + std::to_string(k) + "][" + std::to_string(i) + "]";
            EXPECT_TRUE(isAlmostEqual(ref, calc[k][i])) << ErrorMsg(title, ref, calc[k][i]);
        }
    }
}
//---------------------------------------------------------------------------------------------------
