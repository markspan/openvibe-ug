///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrixTest.hpp
/// \brief Test Definitions for OpenViBE Matrix Class.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 11/05/2020.
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

#include <gtest/gtest.h>
#include <openvibe/CMatrix.hpp>
#include "utils.hpp"

//---------------------------------------------------------------------------------------------------
class CMatrix_Tests : public testing::Test
{
protected:
    OpenViBE::CMatrix m_mat;

    void SetUp() override
    {
        m_mat.resize(1, 2);                         // one row two column buffer not init
        m_mat.getBuffer()[0] = 10;                  // Buffer init with getBuffer and First element set
        m_mat.getBuffer()[1] = 20;                  // Second Element set (buffer already init so refresh function not run)
        m_mat.setDimensionLabel(0, 0, "dim0e0");    // Row label set (warning with setDimensionLabel unused return. Question to keep return true must be asked)
        m_mat.setDimensionLabel(1, 0, "dim1e0");    // Column 1 Label set
        m_mat.setDimensionLabel(1, 1, "dim1e1");    // Column 2 Label set
    }
};

//---------------------------------------------------------------------------------------------------
TEST_F(CMatrix_Tests, Constructor)
{
    const OpenViBE::CMatrix res;
    ASSERT_EQ(0, res.getSize()) << "Default constructor doesn't have a size of 0.";
    EXPECT_STREQ("", res.getDimensionLabel(0, 0)) << "Default constructor has no dimension so no label.";

    ASSERT_EQ(2, m_mat.getSize()) << "Setup Matrix doesn't have 2 values.";
    ASSERT_EQ(2, m_mat.getDimensionCount()) << "Setup Matrix doesn't have 2 Dimensions.";
    ASSERT_EQ(1, m_mat.getDimensionSize(0)) << "Setup Matrix doesn't have 1 Row.";
    ASSERT_EQ(2, m_mat.getDimensionSize(1)) << "Setup Matrix doesn't have 2 Columns.";
    EXPECT_TRUE(AlmostEqual(10, m_mat.getBuffer()[0])) << "Setup Matrix 1st value isn't 10.";
    EXPECT_TRUE(AlmostEqual(20, m_mat.getBuffer()[1])) << "Setup Matrix 2nd value isn't 20.";
    EXPECT_STREQ("dim0e0", m_mat.getDimensionLabel(0, 0)) << "Setup Matrix Row Label isn't dim0e0.";
    EXPECT_STREQ("dim1e0", m_mat.getDimensionLabel(1, 0)) << "Setup Matrix 1st Column Label isn't dim1e0.";
    EXPECT_STREQ("dim1e1", m_mat.getDimensionLabel(1, 1)) << "Setup Matrix 2nd Column Label isn't dim1e1.";

    EXPECT_TRUE(m_mat.isBufferValid()) << "Setup Matrix isn't valid";
    m_mat.getBuffer()[0] = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(m_mat.isBufferValid()) << "infinity isn't considered as invalid";
    m_mat.getBuffer()[0] = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(m_mat.isBufferValid()) << "Setup Matrix isn't valid";
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(CMatrix_Tests, Constructor_Copy)
{
    OpenViBE::CMatrix res(m_mat);

    EXPECT_TRUE(m_mat == res) << ErrorMsg("Copy Constructor", m_mat, res);

    res.getBuffer()[0] = 15;
    res.getBuffer()[1] = 25;
    res.setDimensionLabel(1, 0, "changed");

    EXPECT_TRUE(AlmostEqual(10, m_mat.getBuffer()[0])) << "Setup Matrix 1st value isn't 10.";
    EXPECT_TRUE(AlmostEqual(20, m_mat.getBuffer()[1])) << "Setup Matrix 2nd value isn't 20.";
    EXPECT_STREQ("dim1e0", m_mat.getDimensionLabel(1, 0)) << "Setup Matrix 1st Column Label isn't dim1e0.";
    EXPECT_TRUE(AlmostEqual(15, res.getBuffer()[0])) << "New Matrix 1st value isn't 15.";
    EXPECT_TRUE(AlmostEqual(25, res.getBuffer()[1])) << "New Matrix 2nd value isn't 25.";
    EXPECT_STREQ("changed", res.getDimensionLabel(1, 0)) << "New Matrix 1st Column Label isn't changed.";
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(CMatrix_Tests, Constructor_copy_in_array_push)
{
    std::vector<OpenViBE::CMatrix> res;
    res.push_back(m_mat);
    res.push_back(m_mat);

    res[0].getBuffer()[0] = 15;
    res[0].getBuffer()[1] = 25;
    res[0].setDimensionLabel(1, 0, "changed");

    EXPECT_TRUE(AlmostEqual(15, res[0].getBuffer()[0])) << "1st Matrix 1st value isn't 15.";
    EXPECT_TRUE(AlmostEqual(25, res[0].getBuffer()[1])) << "1st Matrix 2nd value isn't 25.";
    EXPECT_STREQ("changed", res[0].getDimensionLabel(1, 0)) << "1st Matrix 1st Column Label isn't changed.";
    EXPECT_TRUE(AlmostEqual(10, res[1].getBuffer()[0])) << "2nd Matrix 1st value isn't 10.";
    EXPECT_TRUE(AlmostEqual(20, res[1].getBuffer()[1])) << "2nd Matrix 2nd value isn't 20.";
    EXPECT_STREQ("dim1e0", res[1].getDimensionLabel(1, 0)) << "2nd Matrix 1st Column Label isn't dim1e0.";
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(CMatrix_Tests, Operators)
{
    OpenViBE::CMatrix res = m_mat;

    EXPECT_TRUE(m_mat.isDescriptionEqual(m_mat)) << ErrorMsg("equality description", m_mat, m_mat);
    EXPECT_TRUE(m_mat.isBufferEqual(m_mat)) << ErrorMsg("equality buffer", m_mat, m_mat);
    EXPECT_TRUE(m_mat.isBufferAlmostEqual(m_mat)) << ErrorMsg("Almost equality buffer", m_mat, m_mat);
    EXPECT_TRUE(m_mat == m_mat) << ErrorMsg("equality operator", m_mat, m_mat);
    EXPECT_TRUE(m_mat == res) << ErrorMsg("Copy assignement", m_mat, res);

    m_mat.getBuffer()[0] = 15;
    m_mat.getBuffer()[1] = 25;
    m_mat.setDimensionLabel(1, 0, "changed");

    EXPECT_TRUE(AlmostEqual(15, m_mat.getBuffer()[0])) << "Setup Matrix 1st value isn't 15.";
    EXPECT_TRUE(AlmostEqual(25, m_mat.getBuffer()[1])) << "Setup Matrix 2nd value isn't 25.";
    EXPECT_STREQ("changed", m_mat.getDimensionLabel(1, 0)) << "Setup Matrix 1st Column Label isn't changed.";
    EXPECT_TRUE(AlmostEqual(10, res.getBuffer()[0])) << "New Matrix 1st value isn't 10.";
    EXPECT_TRUE(AlmostEqual(20, res.getBuffer()[1])) << "New Matrix 2nd value isn't 20.";
    EXPECT_STREQ("dim1e0", res.getDimensionLabel(1, 0)) << "New Matrix 1st Column Label isn't dim1e0.";
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(CMatrix_Tests, SetBuffer)
{
    OpenViBE::CMatrix res(1, 2);
    std::vector<double> buffer = { 10, 20 };
    res.setBuffer(buffer);
    EXPECT_TRUE(AlmostEqual(10, res.getBuffer()[0])) << "Matrix 1st value isn't 10.";
    EXPECT_TRUE(AlmostEqual(20, res.getBuffer()[1])) << "Matrix 2nd value isn't 20.";

    buffer = { 1, 2, 3 };
    res.setBuffer(buffer.data(), 1);
    EXPECT_TRUE(AlmostEqual(1, res.getBuffer()[0])) << "Matrix 1st value isn't 1.";
    EXPECT_TRUE(AlmostEqual(20, res.getBuffer()[1])) << "Matrix 2nd value isn't 20.";
    res.setBuffer(buffer);  // Too big buffer data is copied until and of matrix buffer
    EXPECT_TRUE(AlmostEqual(2, res.getBuffer()[1])) << "Matrix 2nd value isn't 2.";
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(CMatrix_Tests, Resize)
{
    OpenViBE::CMatrix res(1, 2);
    res.resetBuffer();
    res.setDimensionLabel(0, 0, "label");
    ASSERT_EQ(2, res.getDimensionCount()) << "Matrix doesn't have 2 Dimensions.";
    ASSERT_EQ(1, res.getDimensionSize(0)) << "Matrix doesn't have 1 Row.";
    ASSERT_EQ(2, res.getDimensionSize(1)) << "Matrix doesn't have 2 Columns.";
    EXPECT_TRUE(AlmostEqual(0, res.getBuffer()[0])) << "Matrix 1st default value isn't 0 after resize.";
    EXPECT_TRUE(AlmostEqual(0, res.getBuffer()[1])) << "Matrix 2nd default value isn't 0 after resize.";
    EXPECT_STREQ("label", res.getDimensionLabel(0, 0)) << "Matrix Row Label isn't \"label\".";
    EXPECT_STREQ("", res.getDimensionLabel(1, 0)) << "Matrix 1st Col default Label isn't empty.";
    EXPECT_STREQ("", res.getDimensionLabel(1, 1)) << "Matrix 2nd Col default Label isn't empty.";

    res.resize(2, 2);
    res.setDimensionLabel(1, 1, "label");
    ASSERT_EQ(2, res.getDimensionCount()) << "Matrix doesn't have 2 Dimensions.";
    ASSERT_EQ(2, res.getDimensionSize(0)) << "Matrix doesn't have 2 Row.";
    ASSERT_EQ(2, res.getDimensionSize(1)) << "Matrix doesn't have 2 Columns.";
    EXPECT_STREQ("", res.getDimensionLabel(0, 0)) << "Matrix 1st Row default Label isn't empty.";       // The resize remove all previous label and size
    EXPECT_STREQ("label", res.getDimensionLabel(1, 1)) << "Matrix 2nd Col Label isn't \"label\".";

    res.resetLabels();
    EXPECT_STREQ("", res.getDimensionLabel(1, 1)) << "Matrix 2nd Col reseted Label isn't empty.";

    res.resize(2);
    ASSERT_EQ(1, res.getDimensionCount()) << "Matrix doesn't have 1 Dimension.";
    ASSERT_EQ(2, res.getDimensionSize(0)) << "Matrix doesn't have 2 Row.";
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST_F(CMatrix_Tests, Save_Load)
{
    OpenViBE::CMatrix calc;
    EXPECT_TRUE(m_mat.toTextFile("Save_2DMatrix-output.txt")) << "Error during Saving 2D Matrix : " << std::endl << m_mat << std::endl;
    EXPECT_TRUE(calc.fromTextFile("Save_2DMatrix-output.txt")) << "Error during Loading 2D Matrix : " << std::endl << calc << std::endl;
    EXPECT_TRUE(m_mat == calc) << ErrorMsg("Save", m_mat, calc);

    OpenViBE::CMatrix ref(2);
    ref.getBuffer()[0] = -1;
    ref.getBuffer()[1] = -4.549746549678;
    EXPECT_TRUE(ref.toTextFile("Save_2DMatrix-output.txt")) << "Error during Saving 1D Matrix : " << std::endl << ref << std::endl;
    EXPECT_TRUE(calc.fromTextFile("Save_2DMatrix-output.txt")) << "Error during Loading 1D Matrix : " << std::endl << calc << std::endl;
    EXPECT_TRUE(ref == calc) << ErrorMsg("Save", ref, calc);
}
//---------------------------------------------------------------------------------------------------
