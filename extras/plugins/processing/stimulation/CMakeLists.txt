project(openvibe-plugins-stimulation VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  PUBLIC
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  Boost::boost
					  Boost::system
					  Boost::thread
					  alut
					  vorbis
					  lua
)

if(UNIX AND NOT APPLE)
	find_library(LIB_RT rt)
	if(LIB_RT)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_RT})
	else()
		message(WARNING "  FAILED to find rt...")
	endif()
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")


add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include("FindOpenViBEModuleTCPTagging")

# ---------------------------------
# Finds standard library winmm
# Adds library to target
# Adds include path
# ---------------------------------
if(WIN32)
	find_library(LIB_STANDARD_MODULE_WINMM winmm)
	if(LIB_STANDARD_MODULE_WINMM)
		message(STATUS "  Found winmm...")
		target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_STANDARD_MODULE_WINMM})
	else(LIB_STANDARD_MODULE_WINMM)
		message(STATUS "  FAILED to find winmm...")
	endif(LIB_STANDARD_MODULE_WINMM)
endif(WIN32)

file(COPY share/ DESTINATION ${BUILD_DATADIR}/plugins/stimulation)
file(COPY box-tutorials DESTINATION ${BUILD_DATADIR}/scenarios)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/stimulation)
install(DIRECTORY box-tutorials DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)

add_subdirectory(test)
