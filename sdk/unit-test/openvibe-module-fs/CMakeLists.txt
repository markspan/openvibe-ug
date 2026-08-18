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
project(openvibe-module-fs-test-utf VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

add_executable(${PROJECT_NAME} uoFilesTestUTF.cpp)

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-module-fs
					  openvibe-test-unit-toolkit
					  Boost::boost
					  Boost::filesystem
					  GTest::GTest
					  GTest::Main
)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})	# Place project in folder unit-test (for some IDE)

add_definitions("-DDATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
add_definitions("-DTMP_DIR=\"${OVT_TEST_TEMPORARY_DIR}\"")

add_test(NAME uoFSTestUTF COMMAND ${PROJECT_NAME})

#######################################################################
project(openvibe-module-fs-test VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# ----------------------
# Configure test target
# ----------------------

# Test that needs to called with parameters
set(TEST_WITH_PARAM
	uoEntryEnumeratorTest.cpp
	uoFilesTest.cpp
)

# Test that needs to called without parameters
set(TEST_NO_PARAM
)

# Create test sources list
# This macro auto-generate ${PROJECT_NAME}.cpp
# in the build tree. ${PROJECT_NAME}.cpp is 
# the test driver called by ctest to run 
# the different tests added to this target.
create_test_sourcelist (Tests
	${PROJECT_NAME}.cpp
	${TEST_WITH_PARAM}
	${TEST_NO_PARAM}
	)

add_executable(${PROJECT_NAME} ${Tests})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-module-fs
					  openvibe-test-unit-toolkit)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})

# Add test without parameter to driver 
foreach(test ${TEST_NO_PARAM})
	get_filename_component(TName ${test} NAME_WE)
	add_test(NAME ${TName} COMMAND ${PROJECT_NAME} ${TName})
endforeach()

# Add test with parameter to driver
add_test(NAME uoEntryEnumeratorTest COMMAND ${PROJECT_NAME} uoEntryEnumeratorTest "${CMAKE_CURRENT_SOURCE_DIR}/data/")
add_test(NAME uoFilesTest COMMAND ${PROJECT_NAME} uoFilesTest "${OVT_TEST_TEMPORARY_DIR}")
