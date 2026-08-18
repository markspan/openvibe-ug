project(openvibe-plugins-network-io VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp)
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-system
					  openvibe-module-lsl
					  Boost::boost
					  Boost::system
					  LSL::lsl
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

set_target_properties(${PROJECT_NAME} PROPERTIES
					 VERSION ${PROJECT_VERSION}
					 SOVERSION ${PROJECT_VERSION_MAJOR}
					 FOLDER ${PLUGINS_FOLDER}
					 COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared"
)

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)
	
# ---------------------------------
if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0501) # for boost::asio
endif(WIN32)

#so that boost won't need to link against DateTime when using the interprocess communication library
#shared memory writer box
add_definitions(-DBOOST_DATE_TIME_NO_LIB)

# ---------------------------------
# Finds standard library winmm
# Adds library to target
# Adds include path
# ---------------------------------
if(WIN32)
	find_library(LIB_STANDARD_MODULE_WINMM winmm)
	if(LIB_STANDARD_MODULE_WINMM)
		message(STATUS "  Found winmm...")
		target_link_libraries(${PROJECT_NAME} ${LIB_STANDARD_MODULE_WINMM})
	else(LIB_STANDARD_MODULE_WINMM)
		message(STATUS "  FAILED to find winmm...")
	endif(LIB_STANDARD_MODULE_WINMM)
endif(WIN32)

file(COPY box-tutorials/ DESTINATION ${BUILD_DATADIR}/scenarios/box-tutorials)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

	
install(DIRECTORY box-tutorials/       DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials)	

add_subdirectory(test)
