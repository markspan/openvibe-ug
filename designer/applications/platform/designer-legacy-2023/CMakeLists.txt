project(openvibe-designer VERSION  ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

if (NOT GTK2_FOUND)
  message(STATUS "GTK2 not found - skipping building ${PROJECT_NAME}")
  return()
endif()

set(PROJECT_PRODUCT_NAME "${BRAND_NAME} Designer")

if(WIN32)
	# Should be uncommented once we stop using cmd as launcher
	#SET(ENTRYPOINT WIN32)
	set(ENTRYPOINT )
else()
	set(ENTRYPOINT )
endif()

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/visualization/*.cpp src/dynamic_settings/*.cpp)

include("FindSourceDependencyJSON")

ov_configure_rc(NAME ${PROJECT_NAME} ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/share/designer.ico)

set(SRC_FILES "${SRC_FILES};${CMAKE_BINARY_DIR}/resource-files/${LOWER_BRAND_NAME}-${LOWER_DESIGNER_NAME}.rc")
add_executable(${PROJECT_NAME} ${ENTRYPOINT} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-kernel
					  openvibe-toolkit
					  openvibe-module-system
					  openvibe-module-fs
					  openvibe-module-communication
					  Boost::boost
					  Boost::filesystem
					  Boost::serialization
					  tinyxml2::tinyxml2
					  ZLIB::ZLIB
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

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  FOLDER ${APP_FOLDER})

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include("FindOpenViBEVisualizationToolkit")

add_definitions(-DProjectVersion=\"${PROJECT_VERSION}\")
add_definitions(-DM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_definitions(-DM_VERSION_MINOR=${PROJECT_VERSION_MINOR})

SET_BUILD_PLATFORM()

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

install_root_alias($<TARGET_FILE_NAME:${PROJECT_NAME}>)
  
configure_file(share/designer.conf-base "${BUILD_DATADIR}/applications/designer/designer.conf" @ONLY)
configure_file(share/about-dialog.ui-base "${BUILD_DATADIR}/applications/designer/about-dialog.ui")
configure_file(share/interface.ui-base "${BUILD_DATADIR}/applications/designer/interface.ui")
configure_file(share/interface-settings.ui-base "${BUILD_DATADIR}/applications/designer/interface-settings.ui")
file(COPY share/ DESTINATION ${BUILD_DATADIR}/applications/designer PATTERN "*-base*" EXCLUDE)


install(FILES "${BUILD_DATADIR}/applications/designer/designer.conf" DESTINATION ${DIST_DATADIR}/openvibe/applications/designer)
install(FILES "${BUILD_DATADIR}/applications/designer/about-dialog.ui" DESTINATION ${DIST_DATADIR}/openvibe/applications/designer)
install(FILES "${BUILD_DATADIR}/applications/designer/interface.ui" DESTINATION ${DIST_DATADIR}/openvibe/applications/designer)
install(FILES "${BUILD_DATADIR}/applications/designer/interface-settings.ui" DESTINATION ${DIST_DATADIR}/openvibe/applications/designer)

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/designer PATTERN PATTERN "*-base*" EXCLUDE)

