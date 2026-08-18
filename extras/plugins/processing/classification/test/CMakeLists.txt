project(test_accuracy VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

if(WIN32)
	add_definitions(-DTARGET_OS_Windows)
endif(WIN32)
if(UNIX)
	add_definitions(-DTARGET_OS_Linux)
endif(UNIX)
add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
add_definitions(-DTARGET_ARCHITECTURE_i386)
	
include_directories(../src)
add_executable(${PROJECT_NAME} test_accuracy.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})	# Place project in folder unit-test (for some IDE)

# Tests
include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shrinkageLDA DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(CLASSIFIERS LDA sLDA)
set(METHODS Native OneVsOne-HT OneVsOne-PKPD OneVsOne-Voting OneVsAll)

foreach(CLASSIFIER ${CLASSIFIERS})
	foreach(METHOD ${METHODS})
		# Compare SVM and MLP have not exactly the same value each time
		add_validation_test_with_xml(scenarios-tests ${CLASSIFIER}-${METHOD})
	endforeach(METHOD)
endforeach(CLASSIFIER)

# Compare SVM and MLP have not exactly the same value each time
set(CLASSIFIERS SVM MLP)
foreach(CLASSIFIER ${CLASSIFIERS})
	foreach(METHOD ${METHODS})
		add_validation_test_without_comparison(scenarios-tests ${CLASSIFIER}-${METHOD})
	endforeach(METHOD)
endforeach(CLASSIFIER)

# Processor
add_validation_test_with_csv(scenarios-tests Classifier-Processor)

# Shrinkage LDA tests. These are in a different block as they use different data (and miss ProcessorBox part)

set(TEST_SCENARIOS shrinkage_lda shrinkage_lda_rot)
set(TEST_THRESHOLD 80)

if (UNIX)
	set(OV_LOGFILE "$ENV{HOME}/.config/${OV_CONFIG_SUBDIR}/log/openvibe-designer.log")
elseif(WIN32)
	set(OV_LOGFILE "$ENV{APPDATA}/${OV_CONFIG_SUBDIR}/log/openvibe-designer.log")
endif()

file(TO_CMAKE_PATH ${OV_LOGFILE} OV_LOGFILE)

foreach(TEST_NAME ${TEST_SCENARIOS})

	set(SCENARIO_TO_TEST "shrinkageLDA/${TEST_NAME}.xml")

	add_test(clean_Classification_${TEST_NAME} "${CMAKE_COMMAND}" "-E" "remove" "-f" ${OV_LOGFILE})
	add_test(run_Classification_${TEST_NAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/designer" "--no-session-management" "--no-gui" "--play-fast" "${SCENARIO_TO_TEST}")
	add_test(compare_Classification_${TEST_NAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_accuracy" ${OS_FLAGS} ${OV_LOGFILE} ${TEST_THRESHOLD})
	
	# It would be better to clean last, but we can't do this as it will delete the 
	# output we wish to include, and we can't prevent clean from running if a prev. test fails
	# We need the clean to be sure that the comparator stage is not getting data from a previous run.
	set_tests_properties(run_Classification_${TEST_NAME} PROPERTIES DEPENDS clean_Classification_${TEST_NAME})

	set_tests_properties(compare_Classification_${TEST_NAME} PROPERTIES DEPENDS run_Classification_${TEST_NAME})
	
endforeach(TEST_NAME)
