include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Lag of LSL can't allow comparison between output and ref
add_validation_test_without_comparison(scenarios-tests LSL-Communication)

# UG fork: the LSL Export box's times on LSL's clock (LSLTime.h, UG.md 2.64)
add_executable(test-lsl-time LSLTimeTest.cpp)
target_include_directories(test-lsl-time PRIVATE ../src/box-algorithms)
set_property(TARGET test-lsl-time PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-lsl-time PRIVATE GTest::GTest GTest::Main)
include(GoogleTest)
gtest_discover_tests(test-lsl-time)
