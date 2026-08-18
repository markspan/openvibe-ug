include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_with_csv(scenarios-tests Threshold-Crossing-Detector)
add_validation_test_with_csv(scenarios-tests Clock-Stimulator)
add_validation_test_with_csv(scenarios-tests Stimulation-Converter)
add_validation_test_with_csv(scenarios-tests Stimulation-Validator)
add_validation_test_with_csv(scenarios-tests Stimulation-Filter)

#add_validation_test_with_git(scenarios-tests Run-Command)	# We can't really compare Directly echo "bla" write "bla" on windows but only bla on unix
add_validation_test_without_comparison(scenarios-tests Run-Command)

#basics definitions needed here
if(WIN32)
	set(EXT cmd)
	set(OS_SUFFIX "")
else()
	set(EXT sh)
	set(OS_SUFFIX "-linux")
endif()

# Test run command with spaces
#
# This construction is intended to test cases where run command box command has spaces
# in both executable path and some of the arguments. We use cmake script 'wrap' to get access to the stdout
# where the results are printed in this case.




set(TEST_NAME run-command-with-spaces)
set(SCENARIO_TO_TEST "${TEST_NAME}.xml")

file(COPY
	${CMAKE_CURRENT_SOURCE_DIR}/${SCENARIO_TO_TEST}
	"${CMAKE_CURRENT_SOURCE_DIR}/space test"
	${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.ref.txt
	${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}-linux.ref.txt
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(clean_${TEST_NAME} "${CMAKE_COMMAND}" "-E" "remove" "-f" "${TEST_NAME}.txt")
add_test(run_${TEST_NAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/designer" --no-gui --no-session-management -d EXT:${EXT} -d OUTFILE:${TEST_NAME}.txt --play ${SCENARIO_TO_TEST})
add_test(compare_${TEST_NAME} "git" "diff" "--no-index" "--ignore-space-at-eol" "${TEST_NAME}${OS_SUFFIX}.ref.txt" "${TEST_NAME}.txt")

set_tests_properties(run_${TEST_NAME} PROPERTIES DEPENDS clean_${TEST_NAME}})
