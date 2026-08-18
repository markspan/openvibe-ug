include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_with_csv(scenarios-tests Sinus-Oscillator)
add_validation_test_without_comparison(scenarios-tests Noise-Generator)
add_validation_test_with_csv(scenarios-tests Time-Signal)
