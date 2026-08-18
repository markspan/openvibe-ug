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

PROJECT(openvibe-regularized-csp)

# ------------
# Test naming
# ------------

# As IO test was part of original OpenViBE, prefix is set to vo (validation/open)
SET(TEST_PREFIX vo)
SET(TEST_MODULE Regularized-CSP)

# -------------------------
# List of scenario to test
# -------------------------


# Test scenario for regularized-csp
SET(TEST_SCENARIOS 
#None  ## Disabled as signal is too short and matrix inversion is undefined. Hence the test results may vary
Tikhonov
Shrink
Both
)

# Templates xml.in files
SET (TRAIN_REGULARIZED_CSP regularized-csp-train)
SET (REGULARIZED_CSP regularized-csp)

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

	# Create test name complying to naming rules
	SET(TEST_NAME ${TEST_PREFIX}${TEST_MODULE}-${SCENARIO})
	
	# The regularized CSP have 4 tests
	# Shrinks and Thikhonovs parameters were involved in the train scenario
	# A threshold value is set for each test regarding the Shrinks and Thikhonovs parameters
	# for each test a training scenario and a test scenario will be created
		
	IF (SCENARIO STREQUAL "Tikhonov")
		SET(SHRINKS 0.0)
		SET(TIKHONOVS 0.9)
		SET(THRESHOLDS 70)
		
	ELSEIF (SCENARIO STREQUAL "Shrink")
		SET(SHRINKS 0.9)
		SET(TIKHONOVS 0.0)
		SET(THRESHOLDS 70)
	
	ELSEIF (SCENARIO STREQUAL "Both")
		SET(SHRINKS 0.5)
		SET(TIKHONOVS 0.5)
		SET(THRESHOLDS 70)
		
	ELSEIF (SCENARIO STREQUAL "None")
		SET(SHRINKS 0.0)
		SET(TIKHONOVS 0.0)
		SET(THRESHOLDS 50)
		
	ENDIF()
	
	# create scenario training scenario file
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${TRAIN_REGULARIZED_CSP}.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/${TRAIN_REGULARIZED_CSP}_${SCENARIO}.xml" @ONLY)
	
	# create scenario test file
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${REGULARIZED_CSP}.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/${REGULARIZED_CSP}_${SCENARIO}.xml" @ONLY)
	
	# assign scenario previously cretaed to variable
	SET (SCENARIO_TO_TRAIN "${CMAKE_CURRENT_BINARY_DIR}/${TRAIN_REGULARIZED_CSP}_${SCENARIO}.xml")
	SET (SCENARIO_TO_TEST "${CMAKE_CURRENT_BINARY_DIR}/${REGULARIZED_CSP}_${SCENARIO}.xml")
	
	# The test will compare files. It should be the same
	ADD_TEST(NAME ${TEST_NAME}
	COMMAND ${CMAKE_COMMAND}
	-DUNQUOTE=1
	-DCMD1="${CMAKE_COMMAND} -E remove -f ${OVT_OPENVIBE_PLAYER_LOG_FILE}"
	
	# Launch the training scenario
	-DCMD2="${OVT_OPENVIBE_PLAYER} --mode=x --max-time=2000 --play-mode=ff --config-file=${OVT_OPENVIBE_DATA}/openvibe.conf --scenario-file=${SCENARIO_TO_TRAIN}"
	
	# Launch the scenario to test
	-DCMD3="${OVT_OPENVIBE_PLAYER} --mode=x --max-time=15000 --play-mode=ff --config-file=${OVT_OPENVIBE_DATA}/openvibe.conf --scenario-file=${SCENARIO_TO_TEST}"
	
	#Launch python checking module
	-DCMD4="${PBS_EXECUTABLE} ${OVT_VALIDATION_TOOLKIT_PATH}/accuracy.py ${OVT_OPENVIBE_PLAYER_LOG_FILE} Cross-validation ${THRESHOLDS}"
	
	-P ${OVT_CMAKE_DIR}/OvtRunMultipleCommand.cmake
	)
	SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES ATTACHED_FILES_ON_FAIL ${OVT_OPENVIBE_PLAYER_LOG_FILE})
	
ENDFOREACH()
