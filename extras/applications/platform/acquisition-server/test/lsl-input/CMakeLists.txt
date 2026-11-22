# SPDX-License-Identifier: AGPL-3.0-only
# UG fork: new file. The LSL driver against real LSL streams on this PC, in a binary of its own as it needs liblsl and the network.
# See UG.md 2.64.

add_executable(test-lsl-input LSLDriverTest.cpp ../../builtin/drivers/labstreaminglayer/CDriverLabStreamingLayer.cpp)
target_include_directories(test-lsl-input PRIVATE ../../builtin/drivers/labstreaminglayer ../../builtin ../../abstractions/driver ../../abstractions)
set_property(TARGET test-lsl-input PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-lsl-input PRIVATE
    openvibe
    openvibe-toolkit
    openvibe-acquisition-abstractions
    LSL::lsl
    GTest::GTest
    GTest::Main
)

include(GoogleTest)
gtest_discover_tests(test-lsl-input)
