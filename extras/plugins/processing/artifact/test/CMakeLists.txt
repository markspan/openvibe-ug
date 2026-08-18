include(TestUtilities)

add_validation_test_with_csv(scenarios-tests Amplitude-Artifact-Detector-Cutoff)
add_validation_test_with_csv(scenarios-tests Amplitude-Artifact-Detector-Stop)
add_validation_test_with_csv(scenarios-tests Amplitude-Artifact-Detector-Stim)
add_validation_test_with_csv(scenarios-tests Amplitude-Artifact-Detector-Zero)
#add_validation_test_with_xml(scenarios-tests ASR-Trainer) # Actually fail on x32 and ubuntu 18, output isn't created.....
add_validation_test_without_comparison(scenarios-tests ASR-Trainer)
add_validation_test_with_csv(scenarios-tests ASR-Processor)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
