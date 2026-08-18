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

project(openvibe-module-ebml-test VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

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
	uoEBMLReaderTest.cpp
	uoEBMLWriterTest.cpp
)

# Test that needs to called without parameters
set(TEST_NO_PARAM
)

# Create test sources list
# This macro auto-generate ${PROJECT_NAME}.cpp
# in the build tree. ${PROJECT_NAME}.cpp is 
# the test driver called by ctest to run 
# the different tests added to this target.
create_test_sourcelist(Tests
	${PROJECT_NAME}.cpp
	${TEST_WITH_PARAM}
	${TEST_NO_PARAM}
)

add_executable(${PROJECT_NAME} ${Tests})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-module-ebml
					  openvibe-test-unit-toolkit)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})

# Add test without parameter to driver 
foreach(test ${TEST_NO_PARAM})
	get_filename_component(TName ${test} NAME_WE)
	add_test(NAME ${TName} COMMAND ${PROJECT_NAME} ${TName})
endforeach()

# Add test with parameter to driver
add_test(NAME uoEBMLReaderTest COMMAND ${PROJECT_NAME} uoEBMLReaderTest "${CMAKE_CURRENT_SOURCE_DIR}/data/" "${OVT_TEST_TEMPORARY_DIR}")
add_test(NAME uoEBMLWriterTest COMMAND ${PROJECT_NAME} uoEBMLWriterTest "${CMAKE_CURRENT_SOURCE_DIR}/data/" "${OVT_TEST_TEMPORARY_DIR}")