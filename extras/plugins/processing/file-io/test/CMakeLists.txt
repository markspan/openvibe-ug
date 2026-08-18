include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_with_csv(scenarios-tests CSV-Signal)
add_validation_test_with_csv(scenarios-tests CSV-Matrix)
add_validation_test_with_csv(scenarios-tests CSV-Features)
add_validation_test_with_csv(scenarios-tests CSV-Spectrum)
add_validation_test_with_csv(scenarios-tests CSV-Stimulations)

add_validation_test_with_csv(scenarios-tests GDF-Reader)

# The test outputs a GDF file, but it runs without comparison as we don't have test tools to compare it to the source.
add_validation_test_without_comparison(scenarios-tests GDF-Read-Write)

# Test in 2 parts: CSV -> GDF -> CSV with CSV comparison
# Reads from CSV to write to GDF
add_validation_test_without_comparison(scenarios-tests GDF-Write-Read-Part1)
# Reads from GDF to write to CSV and compare with input CSV from part 1
add_validation_test_with_csv(scenarios-tests GDF-Write-Read-Part2)
# Part2 reads the GDF file produced by Part1 ("run_" prefix is appended to the test name in TestUtilities.cmake)
set_tests_properties(run_GDF-Write-Read-Part2 PROPERTIES DEPENDS run_GDF-Write-Read-Part1)


add_validation_test_with_git(scenarios-tests OV-Read-Write ov)

add_validation_test_with_csv(scenarios-tests Signal-Concatenation)

# No tests For BCI 2000, Brainvision and EDF because no input Files 
