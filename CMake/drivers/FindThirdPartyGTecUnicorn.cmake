###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Author: Thomas Prampart (Inria)
#
# Copyright (C) 2022 Inria
#
# Module to Find GTec Unicorn SDK
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
###############################################################################

set(UNICORN_FOUND FALSE)

if(WIN32)

	find_library(UNICORN_LIB Unicorn)
	find_path(UNICORN_INCLUDE_DIR unicorn.h PATH_SUFFIXES sdk-gtec-unicorn)

	if(UNICORN_LIB AND UNICORN_INCLUDE_DIR)
		message(STATUS "Found g.tec Unicorn device API")

		add_library(sdk-gtec-unicorn INTERFACE)
		target_include_directories(sdk-gtec-unicorn INTERFACE ${UNICORN_INCLUDE_DIR})
		target_link_libraries(sdk-gtec-unicorn INTERFACE ${UNICORN_LIB})
		set(UNICORN_FOUND TRUE)

	else()
		message(STATUS "g.tec Unicorn device API not found (optional driver)")
	endif()
endif()

if(UNIX)
	message(STATUS "g.tec Unicorn device API (optional driver): no Linux or Apple support")
endif()
