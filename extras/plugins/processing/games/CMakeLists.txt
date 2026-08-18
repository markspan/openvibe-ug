project(openvibe-plugins-games VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp)
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-lsl
					  Boost::boost
					  Boost::system
					  LSL::lsl
					  )

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

# ---------------------------------
include_directories("src")

# ---------------------------------
# Target macros
# Defines target operating system, architecture, compiler
# ---------------------------------
SET_BUILD_PLATFORM()

#so that boost won't need to link against DateTime when using the interprocess communication library shared memory writer box
#add_definitions(-DBOOST_DATE_TIME_NO_LIB)

set(SUB_DIR_NAME Games)

file(COPY box-tutorials/ DESTINATION ${BUILD_DATADIR}/scenarios/box-tutorials/${SUB_DIR_NAME})

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY box-tutorials/       DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials/${SUB_DIR_NAME})	

# ---------------------------------
# Test applications
# ---------------------------------
#if(OV_COMPILE_TESTS)
#ADD_SUBDIRECTORY(test)
#endif()
