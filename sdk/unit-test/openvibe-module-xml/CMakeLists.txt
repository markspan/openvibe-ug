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
function(SETUP_TEST)
	SET_BUILD_PLATFORM()

	target_link_libraries(${PROJECT_NAME}
						  openvibe
						  openvibe-common
						  openvibe-module-xml
						  openvibe-module-fs
						  Boost::boost
						  Boost::filesystem
						  GTest::GTest
						  GTest::Main
	)
	
	set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})	# Place project in folder unit-test (for some IDE)

	# Add test with parameter to driver
	add_definitions("-DDATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
	add_definitions("-DTEMP_DIR=\"${OVT_TEST_TEMPORARY_DIR}\"")
endfunction()

project(openvibe-module-xml-reader-test VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})
add_executable(${PROJECT_NAME} uoXMLReaderTest.cpp)
SETUP_TEST()
add_test(NAME uoXMLReaderTest COMMAND ${PROJECT_NAME})

project(openvibe-module-xml-writer-test VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})
add_executable(${PROJECT_NAME} uoXMLWriterTest.cpp)
SETUP_TEST()
add_test(NAME uoXMLWriterTest COMMAND ${PROJECT_NAME})
