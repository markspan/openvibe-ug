///-------------------------------------------------------------------------------------------------
/// 
/// \file CNameValuePairListTest.hpp
/// \brief Test Definitions for OpenViBE Name/Value Pair List Class.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 24/11/2021.
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
#include <openvibe/CNameValuePairList.hpp>

//---------------------------------------------------------------------------------------------------
class CNameValuePairList_Tests : public testing::Test
{
protected:
    OpenViBE::CNameValuePairList m_list;

    const std::string m_sValue = "String Value", m_dValue = "Double Value", m_bValue = "Boolean Value";

    void SetUp() override
    {
        m_list.setValue(m_sValue, std::string("string"));
        m_list.setValue(m_dValue, 72.14);
        m_list.setValue(m_bValue, true);
    }
};

//---------------------------------------------------------------------------------------------------
TEST_F(CNameValuePairList_Tests, Constructor)
{
    const OpenViBE::CNameValuePairList res;
    ASSERT_TRUE(0 == res.size()) << "Default constructor doesn't have a size of 0.";
    ASSERT_TRUE(3 == m_list.size()) << "Setup Name/Value Pair List doesn't have 3 values.";

    std::string s;
    double d = 0;
    bool b   = false;
    ASSERT_FALSE(m_list.getValue(std::string("false value"), s)) << "Setup Name/Value Pair List have false value.";
    ASSERT_TRUE(m_list.getValue(m_sValue, s)) << "Setup Name/Value Pair List Haven't String value.";
    ASSERT_TRUE(m_list.getValue(m_dValue, d)) << "Setup Name/Value Pair List Haven't Double value.";
    ASSERT_TRUE(m_list.getValue(m_bValue, b)) << "Setup Name/Value Pair List Haven't Boolean value.";

    ASSERT_TRUE(s == "string") << "Setup Name/Value Pair List String value doesn't match : " << s << " instead string";
    ASSERT_TRUE(std::abs(d - 72.14) < OV_EPSILON) << "Setup Name/Value Pair List Double value doesn't match : " << d << " instead 72.14";
    ASSERT_TRUE(b == true) << "Setup Name/Value Pair List Boolean value doesn't match : " << b << " instead true";

    // Take count of the order of map.
    std::string name0, name1, name2, value0, value1, value2;
    ASSERT_TRUE(m_list.getValue(0, name0, value0)) << "Setup Name/Value Pair List Haven't Value 0.";
    ASSERT_TRUE(m_list.getValue(1, name1, value1)) << "Setup Name/Value Pair List Haven't Value 1.";
    ASSERT_TRUE(m_list.getValue(2, name2, value2)) << "Setup Name/Value Pair List Haven't Value 2.";

    ASSERT_TRUE(name0 == m_bValue) << "Setup Name/Value Pair List name 0 doesn't match : " << name0 << " instead " << m_bValue;
    ASSERT_TRUE(name1 == m_dValue) << "Setup Name/Value Pair List name 1 doesn't match : " << name1 << " instead " << m_dValue;
    ASSERT_TRUE(name2 == m_sValue) << "Setup Name/Value Pair List name 2 doesn't match : " << name2 << " instead " << m_sValue;

    ASSERT_TRUE(value0 == "1") << "Setup Name/Value Pair List value 0 doesn't match : " << value0 << " instead 1";
    ASSERT_TRUE(std::abs(std::stod(value1) - 72.14) < OV_EPSILON) << "Setup Name/Value Pair List value 1 doesn't match : " << value1 << " instead 72.14";
    ASSERT_TRUE(value2 == "string") << "Setup Name/Value Pair List value 2 doesn't match : " << value2 << " instead string";

    std::cout << m_list;
}
