project(openvibe-plugins-sdk-tools VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})


file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
include("FindSourceRCProperties")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-system
					  openvibe-module-ebml
					  openvibe-module-fs
					  openvibe-module-socket
					  openvibe-module-communication)

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")


ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)
#ADD_DEFINITIONS(-DCommunication_Shared)

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

file(COPY share/ DESTINATION ${BUILD_DATADIR}/plugins/tools)
file(COPY box-tutorials DESTINATION ${BUILD_DATADIR}/scenarios/)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/tools)
install(DIRECTORY box-tutorials DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)

