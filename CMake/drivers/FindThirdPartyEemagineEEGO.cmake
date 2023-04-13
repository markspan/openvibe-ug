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
    if (EXISTS ${LIST_DEPENDENCIES_PATH}/sdk_eemagine_eego)
        set(EEMAGINE_EEGO_SDK_DIR ${LIST_DEPENDENCIES_PATH}/sdk_eemagine_eego/eemagine/)
    endif()

    find_library(EEMAGINE_EEGO_SDK_LIB eego-SDK PATHS ${EEMAGINE_EEGO_SDK_DIR}/lib)
    if(EEMAGINE_EEGO_SDK_LIB)
        ov_print(OV_PRINTED "  Found EEmagine EEGO SDK...")

        file(GLOB_RECURSE SRC_FILES ${EEMAGINE_EEGO_SDK_DIR}/sdk/*.cc ${EEMAGINE_EEGO_SDK_DIR}/sdk/*.h)
        target_sources(eemagine-eego-sdk
                       PRIVATE ${SRC_FILES}
        )
        target_include_directories(eemagine-eego-sdk PUBLIC ${EEMAGINE_EEGO_SDK_DIR}/sdk/include)
        target_link_libraries(eemagine-eego-sdk PUBLIC ${EEMAGINE_EEGO_SDK_LIB})
        target_compile_definitions(eemagine-eego-sdk PRIVATE -DEEGO_SDK_BIND_DYNAMIC)
        target_compile_options(eemagine-eego-sdk
                               INTERFACE -DTARGET_HAS_ThirdPartyEEGOAPI
        )

        # Copy the DLL file at install - Could it be attached to the target and installed with the depending target instead ?
        install(DIRECTORY ${EEMAGINE_EEGO_SDK_DIR}/bin/ DESTINATION ${DIST_BINDIR} FILES_MATCHING PATTERN "*.dll")

    else(EEMAGINE_EEGO_SDK_LIB)
        ov_print(OV_PRINTED "  FAILED to find EEmagine EEGO SDK (optional)")
    endif(EEMAGINE_EEGO_SDK_LIB)
else()
    add_library(eemagine-eego-sdk INTERFACE)
endif()

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyEemagineEEGO "Yes")