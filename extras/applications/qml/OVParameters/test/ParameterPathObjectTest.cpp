/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file. What a path setting holds after the file dialog or the text field has set it. See UG.md, 2.22.
 */
#include "parameterPathObject.h"

#include <gtest/gtest.h>

using OpenViBE::Parameters::ParameterPath;
using OpenViBE::Parameters::ParameterPathObject;

namespace {

class ParameterPathObjectTest : public ::testing::Test
{
protected:
    ParameterPath m_path;
    ParameterPathObject m_object{&m_path};
};

// What the file dialog gives
TEST_F(ParameterPathObjectTest, aPickedFileHasForwardSlashes)
{
    m_object.choose("file:///C:/Users/me/signals/p300-train-[2026.11.05-10.02.13].ov");
    EXPECT_EQ(m_object.value().toStdString(), "C:/Users/me/signals/p300-train-[2026.11.05-10.02.13].ov");
    EXPECT_EQ(m_object.value().indexOf('\\'), -1);
}

TEST_F(ParameterPathObjectTest, aTokenStaysAsItIs)
{
    m_object.setValue("${Player_ScenarioDirectory}/signals/CHANGE_ME.ov");
    EXPECT_EQ(m_object.value().toStdString(), "${Player_ScenarioDirectory}/signals/CHANGE_ME.ov");
}

TEST_F(ParameterPathObjectTest, anEmptyPathStaysEmpty)
{
    m_object.setValue("C:/a.ov");
    m_object.setValue("");
    EXPECT_TRUE(m_object.value().isEmpty());
}

#if defined(_WIN32)
// What Explorer's "Copy as path" gives. On other systems a backslash can be part of a file's name.
TEST_F(ParameterPathObjectTest, aPastedWindowsPathHasForwardSlashes)
{
    m_object.setValue("C:\\Users\\me\\signals\\p300-train.ov");
    EXPECT_EQ(m_object.value().toStdString(), "C:/Users/me/signals/p300-train.ov");
}
#endif

}  // namespace
