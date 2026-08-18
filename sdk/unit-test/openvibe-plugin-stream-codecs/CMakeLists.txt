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
						  GTest::GTest
						  GTest::Main
	)
endfunction()

project(openvibe-plugin-stream-codecs-test VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})
add_executable(${PROJECT_NAME} uoStreamedMatrixTest.cpp ../common/ovtKernelContext.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	FOLDER ${MISC_FOLDER}
)

SETUP_TEST()

add_test(NAME uoStreamedMatrixTest COMMAND ${PROJECT_NAME})
