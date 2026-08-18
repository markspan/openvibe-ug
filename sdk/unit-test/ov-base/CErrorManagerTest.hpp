///-------------------------------------------------------------------------------------------------
/// 
/// \file CErrorManagerTest.hpp
/// \brief Test Definitions for OpenViBE CErrorManager Class.
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
#include <openvibe/kernel/error/CErrorManager.hpp>


// DO NOT USE a global Test::ScopedTest<Test::SKernelFixture> variable here
// because it causes a bug due to plugins global descriptors beeing destroyed before the kernel context.


//---------------------------------------------------------------------------------------------------
class CErrorManager_Tests : public testing::Test
{
protected:
    OpenViBE::Kernel::CErrorManager m_errorManager;
};


//---------------------------------------------------------------------------------------------------
TEST_F(CErrorManager_Tests, Init)
{
    // check manager is correctly initialized
    EXPECT_FALSE(m_errorManager.hasError());
    EXPECT_TRUE(m_errorManager.getLastError() == nullptr);
    EXPECT_TRUE(std::string(m_errorManager.getLastErrorString()).empty());
    EXPECT_EQ(m_errorManager.getLastErrorType(), OpenViBE::Kernel::ErrorType::NoErrorFound);
    EXPECT_NO_THROW(m_errorManager.releaseErrors());
}

//---------------------------------------------------------------------------------------------------
TEST_F(CErrorManager_Tests, Push)
{
    // push an error
    m_errorManager.pushError(OpenViBE::Kernel::ErrorType::Overflow, std::string("An integer overflow error occurred"));

    EXPECT_TRUE(m_errorManager.hasError());
    EXPECT_STREQ(m_errorManager.getLastErrorString(), "An integer overflow error occurred");
    EXPECT_EQ(m_errorManager.getLastErrorType(), OpenViBE::Kernel::ErrorType::Overflow);

    // test match error features returned direclty by manager match error features
    const auto* error = m_errorManager.getLastError();

    ASSERT_TRUE(error != nullptr);
    EXPECT_STREQ(error->getErrorString(), "An integer overflow error occurred");
    EXPECT_EQ(error->getErrorType(), OpenViBE::Kernel::ErrorType::Overflow);
    EXPECT_STREQ(error->getErrorLocation(), "NoLocationInfo:0");
    EXPECT_TRUE(error->getNestedError() == nullptr);

    // push another error
    m_errorManager.pushError(OpenViBE::Kernel::ErrorType::BadAlloc, "Memory allocation failed", "urErrorManagerTest.cpp", 64);

    // test top error has changed
    EXPECT_STREQ(m_errorManager.getLastErrorString(), "Memory allocation failed");
    EXPECT_EQ(m_errorManager.getLastErrorType(), OpenViBE::Kernel::ErrorType::BadAlloc);

    error = m_errorManager.getLastError();
    ASSERT_TRUE(error != nullptr);
    EXPECT_STREQ(error->getErrorString(), "Memory allocation failed");
    EXPECT_EQ(error->getErrorType(), OpenViBE::Kernel::ErrorType::BadAlloc);
    EXPECT_STREQ(error->getErrorLocation(), "urErrorManagerTest.cpp:64");

    const auto* const nestedError = error->getNestedError();
    ASSERT_TRUE(nestedError != nullptr);
    EXPECT_STREQ(nestedError->getErrorString(), "An integer overflow error occurred");
    EXPECT_EQ(nestedError->getErrorType(), OpenViBE::Kernel::ErrorType::Overflow);
    EXPECT_STREQ(nestedError->getErrorLocation(), "NoLocationInfo:0");
    EXPECT_TRUE(nestedError->getNestedError() == nullptr);
}

//---------------------------------------------------------------------------------------------------
TEST_F(CErrorManager_Tests, Release)
{
    m_errorManager.releaseErrors();

    // check manager is correctly released
    EXPECT_FALSE(m_errorManager.hasError());
    EXPECT_TRUE(m_errorManager.getLastError() == nullptr);
    EXPECT_TRUE(std::string(m_errorManager.getLastErrorString()).empty());
    EXPECT_EQ(m_errorManager.getLastErrorType(), OpenViBE::Kernel::ErrorType::NoErrorFound);

    // add an error after release
    m_errorManager.pushError(OpenViBE::Kernel::ErrorType::ResourceNotFound, "File not found on system", "urErrorManagerTest.cpp", 93);

    EXPECT_TRUE(m_errorManager.hasError());
    EXPECT_STREQ(m_errorManager.getLastErrorString(), "File not found on system");
    EXPECT_EQ(m_errorManager.getLastErrorType(), OpenViBE::Kernel::ErrorType::ResourceNotFound);

    const auto* error = m_errorManager.getLastError();

    ASSERT_TRUE(error != nullptr);
    EXPECT_STREQ(error->getErrorString(), "File not found on system");
    EXPECT_EQ(error->getErrorType(), OpenViBE::Kernel::ErrorType::ResourceNotFound);
    EXPECT_STREQ(error->getErrorLocation(), "urErrorManagerTest.cpp:93");
    EXPECT_TRUE(error->getNestedError() == nullptr);
}

//---------------------------------------------------------------------------------------------------
TEST_F(CErrorManager_Tests, StressPush)
{
    m_errorManager.releaseErrors();
    const size_t expectedErrorCount = 10;
    for (size_t i = 0; i < expectedErrorCount; ++i) { m_errorManager.pushError(OpenViBE::Kernel::ErrorType::Unknown, std::string("Error")); }

    size_t errorCount = 0;
    const auto* error = m_errorManager.getLastError();
    while (error) {
        errorCount++;
        error = error->getNestedError();
    }

    EXPECT_EQ(errorCount, expectedErrorCount);
}
