# SPDX-License-Identifier: AGPL-3.0-only
# UG fork: new file. The part of the Acquisition client that needs neither the player nor the network.

add_executable(test-stimulation-delay StimulationDelayTest.cpp)
target_include_directories(test-stimulation-delay PRIVATE ../src/box-algorithms)
set_property(TARGET test-stimulation-delay PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-stimulation-delay PRIVATE openvibe GTest::GTest GTest::Main)

# UG fork: see UG.md 2.30
add_executable(test-sample-blocks SampleBlocksTest.cpp)
target_include_directories(test-sample-blocks PRIVATE ../src/box-algorithms)
set_property(TARGET test-sample-blocks PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-sample-blocks PRIVATE openvibe GTest::GTest GTest::Main)

include(GoogleTest)
gtest_discover_tests(test-stimulation-delay)
gtest_discover_tests(test-sample-blocks)
