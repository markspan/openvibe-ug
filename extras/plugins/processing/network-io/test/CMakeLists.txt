include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Lag of LSL can't allow comparison between output and ref
add_validation_test_without_comparison(scenarios-tests LSL-Communication)
