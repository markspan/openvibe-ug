# SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

## ############################################################################
## Define target name
## ############################################################################
set(TARGET_NAME TcpTaggingService)


###############################################################################
## Project
###############################################################################
project(${TARGET_NAME} VERSION 1.0.0 LANGUAGES CXX)


## ############################################################################
## Define sources, headers, and directories
## ############################################################################
list(APPEND DLL_SOURCES
    TcpTaggingService.cpp
    TagProtocolHandler.cpp
)

list(APPEND DLL_PUBLIC_HEADERS
    TcpTaggingService.h
    TcpTaggingProtocol.h
)

list(APPEND DLL_PRIVATE_HEADERS
    TagProtocolHandler.h
)

extract_relative_directories( DLL_PUBLIC_HEADERS   DLL_PUBLIC_INCLUDE_DIRECTORIES)
extract_relative_directories( DLL_PRIVATE_HEADERS  DLL_PRIVATE_INCLUDE_DIRECTORIES)
prefix_postfix_list_items(DLL_PUBLIC_INCLUDE_DIRECTORIES "\$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/" ">" BUILD_DLL_PUBLIC_INCLUDE_DIRS)
prefix_postfix_list_items(DLL_PUBLIC_INCLUDE_DIRECTORIES "\$<INSTALL_INTERFACE:include/" ">" INSTALL_DLL_PUBLIC_INCLUDE_DIRS)

list(APPEND DLL_PUBLIC_INCLUDE_DIRS
	${BUILD_DLL_PUBLIC_INCLUDE_DIRS}
	${INSTALL_DLL_PUBLIC_INCLUDE_DIRS}
)


## ############################################################################
## Create and configure the target
## ############################################################################
add_library(${TARGET_NAME} SHARED)
setup_output_paths_services(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PUBLIC
    openvibe
    openvibe-module-network
    openvibe-module-system
)

target_sources(${TARGET_NAME}
    PRIVATE
	    ${DLL_SOURCES}
		${DLL_PRIVATE_HEADERS}
	PUBLIC
		${DLL_PUBLIC_HEADERS}
	)

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${DLL_PRIVATE_INCLUDE_DIRECTORIES}
	PUBLIC
	    ${DLL_PUBLIC_INCLUDE_DIRS}
)

#############################
# Dll export header handling
generate_export_header(
    ${TARGET_NAME}
    BASE_NAME "${TARGET_NAME}"
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}_Export.h"
)
create_filters(${TARGET_NAME} EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}_Export.h")
target_sources(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}_Export.h")
# end
#############################

create_filters(${TARGET_NAME})

if(OV_COMPILE_TESTS)
    add_subdirectory(test)
endif()
