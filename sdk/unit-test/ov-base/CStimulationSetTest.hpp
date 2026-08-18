///-------------------------------------------------------------------------------------------------
/// 
/// \file CStimulationSetTest.hpp
/// \brief Test Definitions for OpenViBE Stimulation Set Class.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 08/11/2021.
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
#include <openvibe/CStimulationSet.hpp>

//---------------------------------------------------------------------------------------------------
class CStimulationSet_Tests : public testing::Test
{
protected:
    OpenViBE::CStimulationSet m_set;

    void SetUp() override
    {
        m_set.push_back(0, 1, 2);
        m_set.push_back(3, 4, 5);
        m_set.push_back(6, 7, 8);
        // [[0, 1, 2], [3, 4, 5], [6, 7, 8]]
    }
};

//---------------------------------------------------------------------------------------------------
TEST_F(CStimulationSet_Tests, Constructor)
{
    const OpenViBE::CStimulationSet res;

    // [[0, 1, 2], [9, 10, 11], [3, 4, 5], [6, 7, 8]]
    ASSERT_EQ(0, res.size()) << "Default constructor doesn't have a size of 0.";
    ASSERT_EQ(3, m_set.size()) << "Setup Stimulation Set doesn't have 3 values.";
    ASSERT_EQ(0, m_set.getId(0)) << "Setup Stimulation Set 1rst Stimulation Id isn't 0.";
    ASSERT_EQ(1, m_set.getDate(0)) << "Setup Stimulation Set 1rst Stimulation Date isn't 1.";
    ASSERT_EQ(2, m_set.getDuration(0)) << "Setup Stimulation Set 1rst Stimulation Duration isn't 2.";
    ASSERT_EQ(3, m_set.getId(1)) << "Setup Stimulation Set 2nd Stimulation Id isn't 3.";
    ASSERT_EQ(4, m_set.getDate(1)) << "Setup Stimulation Set 2nd Stimulation Date isn't 4.";
    ASSERT_EQ(5, m_set.getDuration(1)) << "Setup Stimulation Set 2nd Stimulation Duration isn't 5.";
    ASSERT_EQ(6, m_set.getId(2)) << "Setup Stimulation Set 3rd Stimulation Id isn't 6.";
    ASSERT_EQ(7, m_set.getDate(2)) << "Setup Stimulation Set 3rd Stimulation Date isn't 7.";
    ASSERT_EQ(8, m_set.getDuration(2)) << "Setup Stimulation Set 3rd Stimulation Duration isn't 8.";
    std::cout << m_set;
}

//---------------------------------------------------------------------------------------------------
TEST_F(CStimulationSet_Tests, Accessor)
{
    const OpenViBE::CStimulationSet res;
    res.resize(3);
    res.setId(0, m_set.getId(0));
    res.setDate(0, m_set.getDate(0));
    res.setDuration(0, m_set.getDuration(0));
    res.setId(1, m_set.getId(1));
    res.setDate(1, m_set.getDate(1));
    res.setDuration(2, m_set.getDuration(2));
    res.setId(2, m_set.getId(2));
    res.setDate(2, m_set.getDate(2));
    res.setDuration(1, m_set.getDuration(1));

    // [[0, 1, 2], [9, 10, 11], [3, 4, 5], [6, 7, 8]]
    ASSERT_EQ(res.size(), m_set.size()) << "Stimulation Set size are not equals.";
    ASSERT_EQ(res.getId(0), m_set.getId(0)) << "Stimulation Set 1rst Stimulation Id are not equals.";
    ASSERT_EQ(res.getDate(0), m_set.getDate(0)) << "Stimulation Set 1rst Stimulation Date are not equals.";
    ASSERT_EQ(res.getDuration(0), m_set.getDuration(0)) << "Stimulation Set 1rst Stimulation Duration are not equals.";
    ASSERT_EQ(res.getId(1), m_set.getId(1)) << "Stimulation Set 2nd Stimulation Id are not equals.";
    ASSERT_EQ(res.getDate(1), m_set.getDate(1)) << "Stimulation Set 2nd Stimulation Date are not equals.";
    ASSERT_EQ(res.getDuration(1), m_set.getDuration(1)) << "Stimulation Set 2nd Stimulation Duration are not equals.";
    ASSERT_EQ(res.getId(2), m_set.getId(2)) << "Stimulation Set 3rd Stimulation Id are not equals.";
    ASSERT_EQ(res.getDate(2), m_set.getDate(2)) << "Stimulation Set 3rd Stimulation Date are not equals.";
    ASSERT_EQ(res.getDuration(2), m_set.getDuration(2)) << "Stimulation Set 3rd Stimulation Duration are not equals.";
}

//---------------------------------------------------------------------------------------------------
TEST_F(CStimulationSet_Tests, Manipulation)
{
    const OpenViBE::CStimulationSet res;
    res.copy(m_set);                // [[0, 1, 2], [3, 4, 5], [6, 7, 8]]
    ASSERT_EQ(3, res.size()) << "Stimulation Set doesn't have 3 values.";

    res.insert(1, 9, 10, 11);       // [[0, 1, 2], [9, 10, 11], [3, 4, 5], [6, 7, 8]]
    ASSERT_EQ(4, res.size()) << "Stimulation Set doesn't have 4 values.";
    ASSERT_EQ(9, res.getId(1)) << "Inserted Stimulation Id isn't 9.";
    ASSERT_EQ(10, res.getDate(1)) << "Inserted Stimulation Date isn't 10.";
    ASSERT_EQ(11, res.getDuration(1)) << "Inserted Stimulation Duration isn't 11.";

    res.erase(2);                   // [[0, 1, 2], [9, 10, 11], [6, 7, 8]]
    ASSERT_EQ(3, res.size()) << "Stimulation Set doesn't have 3 values.";
    ASSERT_EQ(6, res.getId(2)) << "After remove Stimulation Id isn't 6.";
    ASSERT_EQ(7, res.getDate(2)) << "After remove Stimulation Date isn't 7.";
    ASSERT_EQ(8, res.getDuration(2)) << "After remove Stimulation Duration isn't 8.";

    res.append(res, 10);            // [[0, 1, 2], [9, 10, 11], [6, 7, 8], [0, 11, 2], [9, 20, 11], [6, 17, 8]]
    ASSERT_EQ(6, res.size()) << "Stimulation Set doesn't have 6 values.";
    ASSERT_EQ(11, res.getDate(3)) << "After append Stimulation Date isn't 11.";
    ASSERT_EQ(20, res.getDate(4)) << "After append Stimulation Date isn't 20.";
    ASSERT_EQ(17, res.getDate(5)) << "After append Stimulation Date isn't 17.";

    res.appendRange(res, 18, 22);   // [[0, 1, 2], [9, 10, 11], [6, 7, 8], [0, 11, 2], [9, 20, 11], [6, 17, 8], [9, 20, 11]]
    ASSERT_EQ(7, res.size()) << "Stimulation Set doesn't have 7 values.";
    ASSERT_EQ(20, res.getDate(6)) << "After append Stimulation Date isn't 20.";

    res.removeRange(0, 11);         // [[0, 11, 2], [9, 20, 11], [6, 17, 8], [9, 20, 11]]
    ASSERT_EQ(4, res.size()) << "Stimulation Set doesn't have 4 values.";

    res.shift(1);                   // [[0, 12, 2], [9, 21, 11], [6, 18, 8], [9, 21, 11]]
    ASSERT_EQ(12, res.getDate(0)) << "After append Stimulation Date isn't 12.";
    ASSERT_EQ(21, res.getDate(1)) << "After append Stimulation Date isn't 21.";
    ASSERT_EQ(18, res.getDate(2)) << "After append Stimulation Date isn't 18.";
    ASSERT_EQ(21, res.getDate(3)) << "After append Stimulation Date isn't 21.";
}
