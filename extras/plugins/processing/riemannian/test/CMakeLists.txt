include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_with_csv(scenarios-tests Covariance-Matrix-Calculator)
add_validation_test_with_csv(scenarios-tests Covariance-Mean-Calculator)
add_validation_test_with_csv(scenarios-tests Covariance-To-Feature)
add_validation_test_with_csv(scenarios-tests Feature-To-Covariance)

add_validation_test_with_xml(scenarios-tests Matrix-Classifier-Training-MDM)
add_validation_test_with_xml(scenarios-tests Matrix-Classifier-Training-MDM-Rebias)
add_validation_test_with_xml(scenarios-tests Matrix-Classifier-Training-FgMDM)
add_validation_test_with_xml(scenarios-tests Matrix-Classifier-Training-FgMDM-Rebias)

add_validation_test_with_csv(scenarios-tests Matrix-Classifier-Testing)
add_validation_test_with_csv(scenarios-tests Matrix-Classifier-Testing-Supervised)
add_validation_test_with_csv(scenarios-tests Matrix-Classifier-Testing-Unsupervised)

add_validation_test_with_xml(scenarios-tests Riemannian-Potato-Field-Trainer)
add_validation_test_with_csv(scenarios-tests Riemannian-Potato-Field)
