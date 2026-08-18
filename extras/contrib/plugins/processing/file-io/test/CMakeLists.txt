include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Test in 2 parts: CSV -> EDF -> CSV with CSV comparison
# Reads from CSV to write to EDF
add_validation_test_without_comparison(scenarios-tests EDF-Write-Read-Part1)
# Reads from EDF to write to CSV and compare with input CSV from part 1
add_validation_test_with_csv(scenarios-tests EDF-Write-Read-Part2)
# Part2 reads the EDF file produced by Part1 ("run_" prefix is appended to the test name in TestUtilities.cmake)
set_tests_properties(run_EDF-Write-Read-Part2 PROPERTIES DEPENDS run_EDF-Write-Read-Part1)
