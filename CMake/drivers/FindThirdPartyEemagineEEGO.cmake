###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Author: Thomas Prampart (Inria)
#
# Copyright (C) 2022 Inria
#
# Module to Eemagine EEGO SDK
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

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyEemagineEEGO)


if (WIN32)
    add_library(eemagine-eego-sdk)

    find_library(EEMAGINE_EEGO_SDK_LIB eego-SDK)
    find_path(EEMAGINE_EEGO_SDK_DIR eemagine) # Find the directory containing the eemagine sdk

    if(EEMAGINE_EEGO_SDK_LIB AND EEMAGINE_EEGO_SDK_DIR)
        ov_print(OV_PRINTED "  Found EEmagine EEGO SDK...") 
        file(GLOB_RECURSE SRC_FILES ${EEMAGINE_EEGO_SDK_DIR}/eemagine/src/*.cc ${EEMAGINE_EEGO_SDK_DIR}/eemagine/include/eemagine/sdk/*.h)
        target_sources(eemagine-eego-sdk
                       PRIVATE ${SRC_FILES}
        )
        target_include_directories(eemagine-eego-sdk PUBLIC ${EEMAGINE_EEGO_SDK_DIR}/eemagine/include)
        target_link_libraries(eemagine-eego-sdk PUBLIC ${EEMAGINE_EEGO_SDK_LIB})
        target_compile_definitions(eemagine-eego-sdk PRIVATE -DEEGO_SDK_BIND_DYNAMIC)
        target_compile_options(eemagine-eego-sdk
                               INTERFACE -DTARGET_HAS_ThirdPartyEEGOAPI
        )
    else(EEMAGINE_EEGO_SDK_LIB AND EEMAGINE_EEGO_SDK_DIR)
        ov_print(OV_PRINTED "  FAILED to find EEmagine EEGO SDK (optional)")
    endif(EEMAGINE_EEGO_SDK_LIB AND EEMAGINE_EEGO_SDK_DIR)
else()
    add_library(eemagine-eego-sdk INTERFACE)
endif()

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyEemagineEEGO "Yes")