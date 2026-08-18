include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_with_csv(scenarios-tests Signal-Merger)
add_validation_test_with_csv(scenarios-tests Stream-Switch)
add_validation_test_with_csv(scenarios-tests Input-Switch)
add_validation_test_with_csv(scenarios-tests Stimulation-Multiplexer)
add_validation_test_with_csv(scenarios-tests Matrix-Multiplexer)
