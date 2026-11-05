# SPDX-License-Identifier: AGPL-3.0-only
# UG fork: new file. The LSL Output writer against a real LSL client, in a binary of its own as it needs liblsl and the network.

add_executable(test-lsl-output LSLOutputWriterTest.cpp ../../builtin/writers/lsl-output/LSLOutputWriter.cpp)
target_include_directories(test-lsl-output PRIVATE ../../builtin/writers/lsl-output ../../abstractions/writer ../../abstractions)
set_property(TARGET test-lsl-output PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-lsl-output PRIVATE
    openvibe
    openvibe-acquisition-abstractions
    LSL::lsl
    GTest::GTest
    GTest::Main
)

include(GoogleTest)
gtest_discover_tests(test-lsl-output)
