project(test_evaluation VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

if(WIN32)
	add_definitions(-DTARGET_OS_Windows)
endif(WIN32)
if(UNIX)
	add_definitions(-DTARGET_OS_Linux)
endif(UNIX)
add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
add_definitions(-DTARGET_ARCHITECTURE_i386)
	
include_directories(../src)
add_executable(${PROJECT_NAME} test_kappa.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})	# Place project in folder unit-test (for some IDE)

# Tests
include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_with_csv(scenarios-tests Kappa-coefficient)
#add_validation_test_with_csv(scenarios-tests Confusion-Matrix) # We must Fix the Issue complex title for csv writer/reader
add_validation_test_without_comparison(scenarios-tests Confusion-Matrix)
add_validation_test_without_comparison(scenarios-tests ROC-Curve)
add_validation_test_with_xml(scenarios-tests Statistic-Generator)
add_validation_test_without_comparison(scenarios-tests Classifier-Accuracy)
