# SPDX-FileCopyrightText: Inria / Gipsa-lab
# SPDX-License-Identifier: AGPL-3.0-only
#
# g.tec Unicorn Hybrid Black acquisition driver, built as a loadable plugin.
#
# The acquisition server discovers this library at startup by scanning <exeDir>/drivers for
# shared libraries exporting getInstance() (see CInitApp::loadPlugins). Note that
# BUILTIN_PLUGIN is deliberately NOT defined here: that define is what selects the
# self-registering flavour of REGISTER_DRIVER, which only works inside the executable.

## ############################################################################
## Define target name
## ############################################################################
set(TARGET_NAME GTecUnicornDriver)


###############################################################################
## Project
###############################################################################
project(${TARGET_NAME} VERSION 1.0.0 LANGUAGES CXX)


###############################################################################
## Manage dependencies
###############################################################################
# sdk-gtec-unicorn and UNICORN_FOUND come from CMake/drivers/FindThirdPartyGTecUnicorn.cmake,
# included once from the root CMakeLists. Without the vendor SDK there is nothing to build.
if(NOT WIN32 OR NOT UNICORN_FOUND)
	return()
endif()


## ############################################################################
## Define sources, headers, and directories
## ############################################################################
list(APPEND DLL_SOURCES
	src/CDriverGTecUnicorn.cpp
)

list(APPEND DLL_PRIVATE_HEADERS
	src/CDriverGTecUnicorn.h
)

extract_relative_directories( DLL_PRIVATE_HEADERS  DLL_PRIVATE_INCLUDE_DIRECTORIES)


## ############################################################################
## Create and configure the target
## ############################################################################
add_library(${TARGET_NAME} SHARED)
setup_output_paths_drivers(${TARGET_NAME})

target_sources(${TARGET_NAME}
	PRIVATE
		${DLL_SOURCES}
		${DLL_PRIVATE_HEADERS}
)

target_include_directories(${TARGET_NAME}
	PRIVATE
		${DLL_PRIVATE_INCLUDE_DIRECTORIES}
		${CMAKE_CURRENT_SOURCE_DIR}/../common/src
)

target_link_libraries(${TARGET_NAME} PRIVATE
	openvibe-acquisition-abstractions
	sdk-gtec-unicorn
)

target_compile_definitions(${TARGET_NAME} PRIVATE -DWIN32_LEAN_AND_MEAN -D_USE_MATH_DEFINES)

create_filters(${TARGET_NAME})
