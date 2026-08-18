# SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

## ############################################################################
## Project
## ############################################################################
project(openvibe-module-network VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})


## ############################################################################
## Define target name
## ############################################################################
set(TARGET_NAME ${PROJECT_NAME})


## ############################################################################
## Define target's options
## ############################################################################
option(BUILD_SHARED_LIB "Build ${TARGET_NAME} as a shared library." OFF)

# Check coherency and determine library type
set(LIBRARY_TYPE STATIC) # Default value

if(BUILD_SHARED_LIB)
    set(LIBRARY_TYPE SHARED)
    message(STATUS "Building ${TARGET_NAME} as a SHARED library.")
else()
    message(STATUS "Building ${TARGET_NAME} as a STATIC library.")
endif()


## ############################################################################
## Define sources, headers, and directories
## ############################################################################
list(APPEND LIBRARY_SOURCES
    ClientServerCommon.cpp
    CNetwork.cpp
    Client/CClient.cpp
    OptFirewall/firewall.cpp
    Protocol/ProtocolMsgHandler.cpp
    Server/ConnectedClient.cpp
    Server/CServer.cpp
)

list(APPEND LIBRARY_PUBLIC_HEADERS
    CNetwork.h
    Server/CServer.h
    Client/CClient.h
    ClientServerCommon.h
    OptFirewall/firewall.h
    Protocol/ProtocolMsgHandler.h
    AbstractionLayer/NAL.h
)

list(APPEND LIBRARY_PRIVATE_HEADERS
    Server/ConnectedClient.h
    Tools/crc32.h
)

list(APPEND LIBRARY_PUBLIC_INCLUDE_DIRS
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Server>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Client>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/OptFirewall>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Protocol>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/AbstractionLayer>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<INSTALL_INTERFACE:include>
)

list(APPEND LIBRARY_PRIVATE_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/Tools"
)

## ############################################################################
## Create and configure the target
## ############################################################################
add_library(${TARGET_NAME} ${LIBRARY_TYPE})

target_sources(${TARGET_NAME} PRIVATE ${LIBRARY_SOURCES} ${LIBRARY_PRIVATE_HEADERS} ${LIBRARY_PUBLIC_HEADERS})

generate_export_header(
    ${TARGET_NAME}
    BASE_NAME "${TARGET_NAME}"
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/NetworkExport.h"
)

target_sources(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/NetworkExport.h")

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${LIBRARY_PRIVATE_INCLUDE_DIRS}
		
    PUBLIC
        ${LIBRARY_PUBLIC_INCLUDE_DIRS}
)

## ############################################################################
## Installation phase
## ############################################################################

install(TARGETS ${TARGET_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
