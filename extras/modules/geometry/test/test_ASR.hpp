///-------------------------------------------------------------------------------------------------
/// 
/// \file test_ASR.hpp
/// \brief Tests for Artifact Subspace Reconstruction.
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

#include <geometry/artifacts/CASR.hpp>
#include <geometry/Basics.hpp>

//---------------------------------------------------------------------------------------------------
class Tests_ASR : public testing::Test
{
protected:
    std::vector<Eigen::MatrixXd> m_dataset;

    void SetUp() override { m_dataset = Geometry::Vector2DTo1D(InitDataset::Dataset()); }
};

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_ASR, Train_Euclidian)
{
    const Geometry::CASR ref = InitASR::Euclidian::Reference();
    const Geometry::CASR calc(Geometry::EMetric::Euclidian, m_dataset);
    EXPECT_TRUE(calc == ref) << ErrorMsg("Train ASR in Euclidian metric", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_ASR, Train_Riemann)
{
    std::cout << "Riemannian Eigen Value isn't implemented, so result is same as Euclidian metric." << std::endl;
    const Geometry::CASR ref = InitASR::Riemann::Reference();
    const Geometry::CASR calc(Geometry::EMetric::Riemann, m_dataset);
    EXPECT_TRUE(calc == ref) << ErrorMsg("Train ASR in Riemann metric", ref, calc);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_ASR, Process)
{
    m_dataset = InitDataset::FirstClassDataset();
    Geometry::CASR calc(Geometry::EMetric::Euclidian, m_dataset);

    std::vector<Eigen::MatrixXd> testset = InitDataset::SecondClassDataset();
    std::vector<Eigen::MatrixXd> result(testset.size());
    for (size_t i = 0; i < testset.size(); ++i) {
        testset[i] *= 2;
        EXPECT_TRUE(calc.Process(testset[i], result[i])) << "ASR Process fail for sample " + std::to_string(i) + ".\n";
    }
    for (size_t i = 1; i < testset.size(); ++i) {
        EXPECT_FALSE(isAlmostEqual(result[i], testset[i])) << "the sample " + std::to_string(i) + " wasn't reconstructed.\n";
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(Tests_ASR, Save)
{
    Geometry::CASR calc;
    const Geometry::CASR ref = InitASR::Euclidian::Reference();
    EXPECT_TRUE(ref.SaveXML("test_ASR_Save.xml")) << "Error during Saving : " << std::endl << ref << std::endl;
    EXPECT_TRUE(calc.LoadXML("test_ASR_Save.xml")) << "Error during Loading : " << std::endl << calc << std::endl;
    EXPECT_TRUE(ref == calc) << ErrorMsg("ASR Save", ref, calc);
}
//---------------------------------------------------------------------------------------------------
