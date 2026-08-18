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

PROJECT(openvibe-classification-lda)

# ------------
# Test naming
# ------------

# As IO test was part of original OpenViBE, prefix is set to vo (validation/open)
SET(TEST_PREFIX vo)
SET(TEST_MODULE Classification-LDA)

# -------------------------
# List of scenario to test
# -------------------------


# Test scenario for Classification-LDA
SET(TEST_SCENARIOS 
Native
Onevsall
#Onevsone-HT
Onevsone-PKPD
#Onevsone-Voting
)

# Templates xml.in files
SET (LDA_TEST  LDA)

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
		
	# create scenario file
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${LDA_TEST}-${SCENARIO}.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/${LDA_TEST}-${SCENARIO}.xml" @ONLY)
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${LDA_TEST}-Classification.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/${LDA_TEST}-Classification-${SCENARIO}.xml" @ONLY)

			
	# The test will compare files. It should be the same
	ADD_TEST(NAME ${TEST_NAME}
	COMMAND ${CMAKE_COMMAND}
	-DUNQUOTE=1
	-DCMD1="${CMAKE_COMMAND} -E remove -f ${OVT_OPENVIBE_PLAYER_LOG_FILE}"
	
	# Launch the training scenario
	-DCMD2="${OVT_OPENVIBE_PLAYER} --mode=x --max-time=2000 --play-mode=ff --config-file=${OVT_OPENVIBE_DATA}/openvibe.conf --scenario-file=${LDA_TEST}-${SCENARIO}.xml"
		
	# Launch python checking module
	-DCMD3="${PBS_EXECUTABLE} ${OVT_VALIDATION_TOOLKIT_PATH}/accuracy.py ${OVT_OPENVIBE_PLAYER_LOG_FILE} Cross-validation 55"
	
	# Launch Classification scenario
	-DCMD4="${OVT_OPENVIBE_PLAYER} --mode=x --max-time=2000 --play-mode=ff --config-file=${OVT_OPENVIBE_DATA}/openvibe.conf --scenario-file=${LDA_TEST}-Classification-${SCENARIO}.xml"
	
	# Launch python checking classification result
	-DCMD5="${PBS_EXECUTABLE} ${OVT_VALIDATION_TOOLKIT_PATH}/classification.py ${OVT_TEST_TEMPORARY_DIR}/before-classification-${SCENARIO}.csv ${OVT_TEST_TEMPORARY_DIR}/after-classification-${SCENARIO}.csv ${OVT_TEST_DATA_DIR}/classificationreference-${SCENARIO}.txt"
	

	
	-P ${OVT_CMAKE_DIR}/OvtRunMultipleCommand.cmake
	)
	SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES ATTACHED_FILES_ON_FAIL ${OVT_OPENVIBE_PLAYER_LOG_FILE})
	
ENDFOREACH()
