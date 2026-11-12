include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Test in 2 parts: CSV -> EDF -> CSV with CSV comparison
# Reads from CSV to write to EDF
add_validation_test_without_comparison(scenarios-tests EDF-Write-Read-Part1)
# Reads from EDF to write to CSV and compare with input CSV from part 1
add_validation_test_with_csv(scenarios-tests EDF-Write-Read-Part2)
# Part2 reads the EDF file produced by Part1 ("run_" prefix is appended to the test name in TestUtilities.cmake)
set_tests_properties(run_EDF-Write-Read-Part2 PROPERTIES DEPENDS run_EDF-Write-Read-Part1)

# UG fork: what the BrainVision Format file writer (UG) puts in its files (BrainVisionUG.h)
add_executable(test-brainvision-ug BrainVisionUGTest.cpp)
target_include_directories(test-brainvision-ug PRIVATE ../src/box-algorithms)
set_property(TARGET test-brainvision-ug PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-brainvision-ug PRIVATE GTest::GTest GTest::Main)
include(GoogleTest)
gtest_discover_tests(test-brainvision-ug)

# UG fork: a signal with a Trigger channel and a DC offset, and its hardware triggers, through the BrainVision Format file
# writer (UG); then check_brainvision.py checks the files: every marker on its Trigger channel switch, the sample counter
# whole, the samples all there
add_validation_test_without_comparison(scenarios-tests BrainVision-UG-Write)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
	add_test(NAME check_BrainVision-UG-Write
			 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/check_brainvision.py
					 ${CMAKE_CURRENT_BINARY_DIR}/scenarios-tests/BrainVision-UG-Write-output.vhdr --expect-markers 10 --expect-samples 3072)
	set_tests_properties(check_BrainVision-UG-Write PROPERTIES DEPENDS run_BrainVision-UG-Write)
endif()
