if(OV_DISABLE_GTK)
  message(STATUS  "Skipping Skeleton Generator, no GTK")
  return()
endif(OV_DISABLE_GTK)

project(openvibe-skeleton-generator VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-fs
					  Boost::boost
					  Boost::thread
					  Boost::regex
					  gtk2
)

if(UNIX AND NOT APPLE)
	find_library(LIB_RT rt)
	if(LIB_RT)
		target_link_libraries(${PROJECT_NAME} ${LIB_RT})
	else()
		message(WARNING "  FAILED to find rt...")
	endif()
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

file(COPY share/ DESTINATION ${BUILD_DATADIR}/applications/skeleton-generator)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/skeleton-generator)

install_root_alias($<TARGET_FILE_NAME:${PROJECT_NAME}>)
