# Regex to catch errors in logs when return code is ok
set (failRegex "\\[ *ERROR * \\]")

##################################################
# Function to factorise basic validation test with an output (Run a scenario and compare output csv with a ref)
# How to use : add_validation_test(your-folder Your-Test-Name Comparison Extension-Of-Output Use-threshold-flag) folder can be ""
# 0 on COMPARE if you don't check output
# 0 on USE_THRESHOLD to have only a strict equality with git command, 1 to have the threshold comparison programm for csv or xml 
function(add_validation_test TEST_PATH TEST_NAME COMPARE OUTPUT_EXT USE_THRESHOLD THRESHOLD_VALUE)
	if(TEST_PATH)
		set(TEST_FILE ${TEST_PATH}/${TEST_NAME})
	else()
		set(TEST_FILE ${TEST_NAME})
	endif()

	if(COMPARE)
		add_test(clean_${TEST_NAME}		"${CMAKE_COMMAND}" "-E" "remove" "-f" "${TEST_FILE}-output.${OUTPUT_EXT}")
	endif(COMPARE)

	message(STATUS "add_test run_${TEST_NAME}")
	add_test(run_${TEST_NAME}		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/designer" "--no-session-management" "--no-gui" "--play-fast" "${TEST_FILE}-test.xml")

	#set_tests_properties(run_${TEST_NAME} DIRECTORY ${TEST_PATH} PROPERTIES FAIL_REGULAR_EXPRESSION "${failRegex}")

	if(COMPARE)
		if(USE_THRESHOLD)
			add_test(compare_${TEST_NAME}	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/threshold-comparison" "${TEST_FILE}-output.${OUTPUT_EXT}" "${TEST_FILE}-ref.${OUTPUT_EXT}" ${THRESHOLD_VALUE})
		else()
			add_test(compare_${TEST_NAME}	"git" "diff" "--no-index" "--ignore-space-change" "${TEST_FILE}-output.${OUTPUT_EXT}" "${TEST_FILE}-ref.${OUTPUT_EXT}")
		endif(USE_THRESHOLD)
	endif(COMPARE)

	if(COMPARE)
		set_tests_properties(compare_${TEST_NAME} PROPERTIES DEPENDS run_${TEST_NAME})
		set_tests_properties(run_${TEST_NAME} PROPERTIES DEPENDS clean_${TEST_NAME})
	endif(COMPARE)

endfunction()

##################################################
# Function to factorise basic validation test with an output (Run a scenario and compare output with a ref for any file)
# How to use : add_validation_test_with_git(your-folder Your-Test-Name Extension-Of-Output) folder can be ""
# Test with git command for compare. Useful for not implemented threshold comparison and any binary files
function(add_validation_test_with_git TEST_PATH TEST_NAME OUTPUT_EXT)
	if(TEST_PATH)
		add_validation_test(${TEST_PATH} ${TEST_NAME} TRUE ${OUTPUT_EXT} FALSE 0.0001)
	else()
		add_validation_test("" ${TEST_NAME} TRUE ${OUTPUT_EXT} FALSE 0.0001)
	endif()
endfunction()

##################################################
# Function to factorise basic validation test with a csv output (Run a scenario and compare output csv with a ref)
# How to use : add_validation_test_with_csv(your-folder Your-Test-Name) folder can be ""
function(add_validation_test_with_csv TEST_PATH TEST_NAME)
	message(STATUS "add_validation_test_with_csv(${TEST_PATH} ${TEST_NAME})")
	if(TEST_PATH)
		add_validation_test(${TEST_PATH} ${TEST_NAME} TRUE csv TRUE 0.0001)
	else()
		add_validation_test("" ${TEST_NAME} TRUE csv TRUE 0.0001)
	endif()
endfunction()

##################################################
# Function to factorise basic validation test with a xml output (Run a scenario and compare output xml with a ref.)
# How to use : add_validation_test_with_xml(your-folder Your-Test-Name) folder can be ""
function(add_validation_test_with_xml TEST_PATH TEST_NAME)
	if(TEST_PATH)
		add_validation_test(${TEST_PATH} ${TEST_NAME} TRUE xml TRUE 0.0001)
	else()
		add_validation_test("" ${TEST_NAME} TRUE xml TRUE 0.0001)
	endif()
endfunction()

##################################################
# Function to factorise basic validation test without output
# How to use : add_validation_test_without_comparison(your-folder Your-Test-Name) folder can be ""
function(add_validation_test_without_comparison TEST_PATH TEST_NAME)
	if(TEST_PATH)
		add_validation_test(${TEST_PATH} ${TEST_NAME} FALSE FALSE FALSE 0.0001)
	else()
		add_validation_test("" ${TEST_NAME} FALSE FALSE FALSE 0.0001)
	endif()	
endfunction()

##################################################
# Function to factorise basic validation test for visualization. There is no output generally,  it's just a run test in normal mode (not fast). 
# Crash is only detected and cause an error during tests. Invisible tag must be still here because openvibe can't be closed automaticly without that
# Only manual run with human validation can be optimal. 
# How to use : add_validation_test_visualization(your-folder Your-Test-Name) folder can be ""
function(add_validation_test_visualization TEST_PATH TEST_NAME)
	if(TEST_PATH)
		set(TEST_FILE ${TEST_PATH}/${TEST_NAME})
	else()
		set(TEST_FILE ${TEST_NAME})
	endif()

	add_test(run_${TEST_NAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/designer" "--no-session-management" "--no-gui" "--play" "${TEST_FILE}-test.xml")
	set_tests_properties(run_${TEST_NAME} PROPERTIES FAIL_REGULAR_EXPRESSION "${failRegex}")
endfunction()
