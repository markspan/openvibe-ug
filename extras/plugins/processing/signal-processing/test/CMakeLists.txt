
project(test_eigen VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

if(WIN32)
	add_definitions(-DTARGET_OS_Windows)
endif(WIN32)
if(UNIX)
	add_definitions(-DTARGET_OS_Linux)
endif(UNIX)
add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
add_definitions(-DTARGET_ARCHITECTURE_i386)
	
include_directories(../src)

add_executable(${PROJECT_NAME} test_eigen.cpp)

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-module-system
					  Eigen3::Eigen
)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})	# Place project in folder unit-test (for some IDE)

if(WIN32)	
	# Since cert, getting timeBeginPeriod() linker issues without this
	target_link_libraries(${PROJECT_NAME} winmm)
endif(WIN32)

include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_with_csv(scenarios-tests Epoch-Variance)
add_validation_test_with_csv(scenarios-tests ARFeatures)
add_validation_test_with_csv(scenarios-tests IFFT)
add_validation_test_with_xml(scenarios-tests xDawn)
add_validation_test_with_csv(scenarios-tests Hilbert-Transform)
add_validation_test_with_csv(scenarios-tests Crop)
add_validation_test_with_csv(scenarios-tests Signal-Average)
add_validation_test_with_csv(scenarios-tests Epoch-Average)
add_validation_test_with_csv(scenarios-tests ERSP-Average)
add_validation_test_with_csv(scenarios-tests Matrix-Transpose)
add_validation_test_with_csv(scenarios-tests Windowing)
add_validation_test_with_csv(scenarios-tests Decimation)
add_validation_test_with_csv(scenarios-tests Stimulation-Epoching)
add_validation_test_with_csv(scenarios-tests Time-Epoching)
add_validation_test_with_csv(scenarios-tests Spectral-Analysis)
add_validation_test_with_csv(scenarios-tests Discrete-Wavelet-Transform)
add_validation_test_with_csv(scenarios-tests Discrete-Wavelet-Transform-Inverse)
add_validation_test_with_csv(scenarios-tests ConnectivityWelch)
add_validation_test_with_csv(scenarios-tests ConnectivityMVAR)
add_validation_test_with_csv(scenarios-tests Pulse-Rate-Calculator)
add_validation_test_with_csv(scenarios-tests Entropy-Measure)
