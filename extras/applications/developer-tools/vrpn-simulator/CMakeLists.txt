if(OV_DISABLE_GTK)
  message(STATUS  "Skipping VRPN Simulator, no GTK")
  return()
endif(OV_DISABLE_GTK)

project(openvibe-vrpn-simulator VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})
 
file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  vrpn
					  gtk2
)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})

# ---------------------------------
# Finds standard library pthread
# Adds library to target
# Adds include path
# ---------------------------------
if(UNIX)
        find_package(Threads)
        if(Threads_FOUND)
          target_link_libraries(${PROJECT_NAME} Threads::Threads)
	else(Threads_FOUND)
	  message(STATUS "  FAILED to find pthread...")
	endif(Threads_FOUND)
endif(UNIX)

file(COPY share/ DESTINATION ${BUILD_DATADIR}/applications/vrpn-simulator)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/vrpn-simulator)

install_root_alias($<TARGET_FILE_NAME:${PROJECT_NAME}>)
