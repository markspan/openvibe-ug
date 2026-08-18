project(openvibe-plugins-contrib-python2 VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  Boost::boost)

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

# -----------------------------
include("FindThirdPartyPython2")
# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

if(WIN32)
	# The pygame scenario doesn't work on Windows, so do not install it
	install(DIRECTORY box-tutorials DESTINATION ${DIST_DATADIR}/openvibe/scenarios/ PATTERN "*-pygame-*" EXCLUDE)
else()	
	install(DIRECTORY box-tutorials DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
endif()

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/python2)
