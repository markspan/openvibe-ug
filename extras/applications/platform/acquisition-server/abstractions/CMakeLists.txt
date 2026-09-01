# SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only
#
# The driver/writer abstraction layer of the acquisition server.
#
# This is a shared library rather than a set of sources compiled into the
# acquisition-server executable, because loadable driver and writer plugins
# derive from AbstractDriver / AbstractWriter and therefore need their
# out-of-line members at link time. Both the executable and every plugin link
# this target; its headers are installed so that a plugin can also be built
# out-of-tree, against an installed OpenViBE.

## ############################################################################
## Define target name
## ############################################################################
set(TARGET_NAME openvibe-acquisition-abstractions)


###############################################################################
## Project
###############################################################################
project(${TARGET_NAME} VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION} LANGUAGES CXX)


## ############################################################################
## Define sources, headers, and directories
## ############################################################################
list(APPEND DLL_SOURCES
	driver/AbstractDriver.cpp
	writer/AbstractWriter.cpp
)

list(APPEND DLL_PUBLIC_HEADERS
	samplesBuffer.h
	driver/AbstractDriver.h
	driver/AbstractDriverBuilder.h
	writer/AbstractWriter.h
	writer/AbstractWriterBuilder.h
)

extract_relative_directories( DLL_PUBLIC_HEADERS   DLL_PUBLIC_INCLUDE_DIRECTORIES)
prefix_postfix_list_items(DLL_PUBLIC_INCLUDE_DIRECTORIES "\$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/" ">" BUILD_DLL_PUBLIC_INCLUDE_DIRS)
prefix_postfix_list_items(DLL_PUBLIC_INCLUDE_DIRECTORIES "\$<INSTALL_INTERFACE:include/openvibe/acquisition-server/" ">" INSTALL_DLL_PUBLIC_INCLUDE_DIRS)

list(APPEND DLL_PUBLIC_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
	${BUILD_DLL_PUBLIC_INCLUDE_DIRS}
	${INSTALL_DLL_PUBLIC_INCLUDE_DIRS}
)


## ############################################################################
## Create and configure the target
## ############################################################################
add_library(${TARGET_NAME} SHARED)
setup_output_paths_general(${TARGET_NAME})

target_sources(${TARGET_NAME}
	PRIVATE
		${DLL_SOURCES}
	PUBLIC
		${DLL_PUBLIC_HEADERS}
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		${DLL_PUBLIC_INCLUDE_DIRS}
)

# "openvibe" carries the acquisition-messaging types (sampleType, timestampType, ovStimulation)
# and the parameter classes exposed in the public headers, so it is a PUBLIC dependency.
target_link_libraries(${TARGET_NAME} PUBLIC
	openvibe
)

#############################
# Dll export header handling
set(EXPORT_BASE_NAME OVAcquisitionAbstractions)
generate_export_header(
	${TARGET_NAME}
	BASE_NAME "${EXPORT_BASE_NAME}"
	EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_BASE_NAME}_Export.h"
)
create_filters(${TARGET_NAME} EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_BASE_NAME}_Export.h")
target_sources(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_BASE_NAME}_Export.h")
# end
#############################

create_filters(${TARGET_NAME})


## ############################################################################
## Install the headers, so that a driver or writer plugin can be built
## against an installed OpenViBE rather than from inside this tree.
## ############################################################################
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/samplesBuffer.h
              ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_BASE_NAME}_Export.h
        DESTINATION ${DIST_INCLUDEDIR}/openvibe/acquisition-server/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/driver/AbstractDriver.h
              ${CMAKE_CURRENT_SOURCE_DIR}/driver/AbstractDriverBuilder.h
        DESTINATION ${DIST_INCLUDEDIR}/openvibe/acquisition-server/driver/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/writer/AbstractWriter.h
              ${CMAKE_CURRENT_SOURCE_DIR}/writer/AbstractWriterBuilder.h
        DESTINATION ${DIST_INCLUDEDIR}/openvibe/acquisition-server/writer/)
