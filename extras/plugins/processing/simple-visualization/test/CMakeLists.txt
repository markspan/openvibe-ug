include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_visualization(scenarios-tests Display-cue-image)
add_validation_test_visualization(scenarios-tests ERP-plot)
add_validation_test_visualization(scenarios-tests Multimodal-Graz-visualization)
# if (NOT APPLE)
#    add_validation_test_visualization(scenarios-tests Power-Spectrum)
#    add_validation_test_visualization(scenarios-tests Topography)
# endif()
add_validation_test_visualization(scenarios-tests Simple-visualization)
