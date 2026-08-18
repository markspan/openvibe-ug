#######################################################################
# Software License Agreement (AGPL-3 License)
# 
# OpenViBE SDK Test Software
# Based on OpenViBE V1.1.0, Copyright (C) Inria, 2006-2015
# Copyright (C) Inria, 2015-2017,V1.0
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License version 3,
# as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.
# If not, see <http://www.gnu.org/licenses/>.
#######################################################################

PROJECT(openvibe-file-io-csv-test)

# ------------
# Test naming
# ------------

# As IO test was part of original OpenViBE, prefix is set to vo (validation/open)
SET(TEST_PREFIX vo)
SET(TEST_MODULE FileIO-CSV)

# -------------------------
# List of scenario to test
# -------------------------


# Test scenario for CSV reading and writing test purpose
SET(TEST_SCENARIOS 
	streamed-matrix
	empty-labels-matrix
	vector-matrix
	empty-labels-vector
	features
	signal
	stimulation
	signal-badtimes
	spectrum
)

# ----------------------
# Configure test target
# ----------------------

# This is just a way to have the test scripts available in the IDE
FILE(GLOB_RECURSE script_files ${CMAKE_CURRENT_SOURCE_DIR}/*.mxs ${CMAKE_CURRENT_SOURCE_DIR}/*.xml ${CMAKE_CURRENT_SOURCE_DIR}/*.xml.in)
ADD_CUSTOM_TARGET(${PROJECT_NAME} SOURCES ${script_files})
SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${VALIDATION_FOLDER})

# ---------------------------
# Add test to test to be run
# ---------------------------
FOREACH(SCENARIO ${TEST_SCENARIOS})

	# Modify test name to comply to naming rules
	SET(TEST_NAME ${TEST_PREFIX}${TEST_MODULE}-${SCENARIO})
	
	#Create scenario file with repository according to the test environement 
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${SCENARIO}.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/${SCENARIO}.xml" @ONLY)
	SET(SCENARIO_TO_TEST "${CMAKE_CURRENT_BINARY_DIR}/${SCENARIO}.xml")
	SET (INPUT_FILE "input-${SCENARIO}.csv")
	SET (OUTPUT_FILE "output-${SCENARIO}.csv")
	
	IF ((SCENARIO STREQUAL "signal-badtimes") OR (SCENARIO STREQUAL "signal-nofiletime"))
		#specific variable for signal bad time or signal no file time, the outpout file will be compared to another file than the input file

		SET(FILE_TO_COMPARE "input-signal.csv")
	
	ELSE ()
		# General case, the input and output will be compare
		SET(FILE_TO_COMPARE ${INPUT_FILE})
		
	ENDIF()

	# The test will compare files. It should be the same
	ADD_TEST(NAME ${TEST_NAME}
	COMMAND ${CMAKE_COMMAND}
	-DUNQUOTE=1
	-DCMD1="${CMAKE_COMMAND} -E remove -f ${OVT_OPENVIBE_PLAYER_LOG_FILE}"
	# Launch the scenario with all boxes to test
	-DCMD2="${OVT_OPENVIBE_PLAYER} --mode=x --play-mode=ff --max-time=2000 --config-file=${OVT_OPENVIBE_DATA}/openvibe.conf --scenario-file=${SCENARIO_TO_TEST}"
	# Compare file result with reference file
	-DCMD3="${PBS_EXECUTABLE} ${OVT_VALIDATION_TOOLKIT_PATH}/csv-comparator.py ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_TO_COMPARE} ${OVT_TEST_TEMPORARY_DIR}/${OUTPUT_FILE}"
	-P ${OVT_CMAKE_DIR}/OvtRunMultipleCommand.cmake
	)
	SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES ATTACHED_FILES_ON_FAIL ${OVT_OPENVIBE_PLAYER_LOG_FILE})
	
ENDFOREACH()
