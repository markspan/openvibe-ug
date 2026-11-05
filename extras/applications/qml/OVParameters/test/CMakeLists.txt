# SPDX-License-Identifier: AGPL-3.0-only
# UG fork: new file.

add_executable(test-parameter-path ParameterPathObjectTest.cpp)
set_property(TARGET test-parameter-path PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-parameter-path PRIVATE
    openvibe-parameters-object
    openvibe
    GTest::GTest
    GTest::Main
)

include(GoogleTest)
gtest_discover_tests(test-parameter-path)
