###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Module to find OpenAL and connected libraries library
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

find_package(PkgConfig REQUIRED)

if (APPLE AND (DEFINED ENV{CONDA_PREFIX}))
  # save previous value
  set(FRAMEWORK_POSITION ${CMAKE_FIND_FRAMEWORK})
  set(CMAKE_FIND_FRAMEWORK NEVER)
endif()

##
# Find OpenAL
#
find_package(OpenAL)

if (APPLE AND (DEFINED ENV{CONDA_PREFIX}))
  set(CMAKE_FIND_FRAMEWORK ${FRAMEWORK_POSITION})
endif()

if (OpenAL_FOUND)
    ov_print(OV_PRINTED "Found OpenAL")
    target_compile_definitions(OpenAL::OpenAL INTERFACE -DTARGET_HAS_ThirdPartyOpenAL)
else()
    ov_print(OV_PRINTED "Failed to find OpenAL, Vorbis or OGG")
endif()


##
# Find ALUT
# 
pkg_check_modules(ALUT freealut)
    ov_print(OV_PRINTED "Checking for ALUT")

if (ALUT_FOUND)
    ov_print(OV_PRINTED "Found ALUT")
    add_library(alut INTERFACE)

    set(ALUT_LINK_LIB "ALUT_LINK_LIB-NOTFOUND")
    find_library(ALUT_LINK_LIB NAMES alut PATHS ${ALUT_LIBRARY_DIRS} NO_DEFAULT_PATH)
    if (ALUT_LINK_LIB)
        target_include_directories(alut INTERFACE ${ALUT_INCLUDE_DIRS})
        target_link_libraries(alut INTERFACE ${ALUT_LINK_LIB} OpenAL::OpenAL)
        target_compile_definitions(alut INTERFACE -DTARGET_HAS_ThirdPartyOpenAL)
    else()
        ov_print(OV_PRINTED "Found ALUT, but failed to find the lib")
    endif()
endif()


##
# Find OGG
#
pkg_check_modules(OGG ogg)
if (OGG_FOUND)
    ov_print(OV_PRINTED "Found OGG")
    add_library(ogg INTERFACE)

    set(OGG_LINK_LIB "OGG_LINK_LIB-NOTFOUND")
    find_library(OGG_LINK_LIB NAMES ogg PATHS ${OGG_LIBRARY_DIRS} NO_DEFAULT_PATH)
    if(OGG_LINK_LIB)
        target_include_directories(ogg INTERFACE ${OGG_INCLUDE_DIRS})
        target_link_libraries(ogg INTERFACE ${OGG_LINK_LIB})
    else()
        ov_print(OV_PRINTED "Found OGG, but failed to find the lib")
    endif()
else()
    ov_print(OV_PRINTED "Failed to find OGG")
endif()

##
# Find Vorbis
#
# Linux conda package for OGG not providing OggConfig.cmake and Vorbis needs it so find_package(Vorbis) cannot be used either.

find_library(VORBIS_LIB NAMES vorbisfile)
find_path(VORBIS_INCLUDE_DIR NAMES vorbisfile.h PATH_SUFFIXES vorbis)

if(NOT VORBIS_LIB STREQUAL VORBIS_LIB-NOTFOUND
        AND NOT VORBIS_INCLUDE_DIR STREQUAL VORBIS_INCLUDE_DIR-NOTFOUND)

        ov_print(OV_PRINTED "Found Vorbis library")

    # Create target to link against.
    add_library(vorbis INTERFACE)
    target_include_directories(vorbis INTERFACE ${VORBIS_INCLUDE_DIR})
    target_link_libraries(vorbis INTERFACE ${VORBIS_LIB} ogg)
else()
    ov_print(OV_PRINTED "  FAILED to find Vorbis")
endif()
