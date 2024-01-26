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

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyGtecUnicornCAPI)

add_library(sdk-gtec-unicorn INTERFACE)
if(WIN32)

    find_library(UNICORN_LIB Unicorn)
	find_path(UNICORN_INCLUDE_DIR unicorn.h PATH_SUFFIXES sdk-gtec-unicorn)

	if(UNICORN_LIB AND UNICORN_INCLUDE_DIR)
		OV_PRINT(OV_PRINTED "  Found Gtec Unicorn device API...")

        target_include_directories(sdk-gtec-unicorn INTERFACE ${UNICORN_INCLUDE_DIR})
        target_link_libraries(sdk-gtec-unicorn INTERFACE ${UNICORN_LIB})
        target_compile_definitions(sdk-gtec-unicorn INTERFACE TARGET_HAS_ThirdPartyGtecUnicron)

	else(UNICORN_LIB AND UNICORN_INCLUDE_DIR)
		OV_PRINT(OV_PRINTED "  FAILED to find Gtec Unicorn device API (optional driver)")
	endif(UNICORN_LIB AND UNICORN_INCLUDE_DIR)

endif(WIN32)

if(UNIX)
		OV_PRINT(OV_PRINTED "  Gtec Unicorn device API (optional driver): No Linux or Apple support")
endif(UNIX)

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyGtecUnicornCAPI "Yes")
