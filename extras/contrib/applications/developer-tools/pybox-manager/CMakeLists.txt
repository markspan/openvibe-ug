project(openvibe-plugins-contrib-pybox VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE source_files src/*.cpp src/*.hpp  src/*.h src/*.inl)
add_library(${PROJECT_NAME} SHARED ${source_files})

target_link_libraries(${PROJECT_NAME}
                      openvibe
                      openvibe-common
                      openvibe-toolkit
                      openvibe-module-system
                      Boost::boost
                      PythonBuildStandalone)

if(win32)
  # These are needed not to cause a popup on machines missing the dll
  target_link_libraries(${PROJECT_NAME} optimized Delayimp )
  #set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DELAYLOAD:python37.dll")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    FOLDER ${PLUGINS_FOLDER}
    COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include_directories("src")

# ---------------------------------
# Target macros
# Defines target operating system
# Defines target architecture
# Defines target compiler
# ---------------------------------
SET_BUILD_PLATFORM()

file(COPY box-tutorials DESTINATION ${BUILD_DATADIR}/scenarios)
file(COPY share/ DESTINATION ${BUILD_DATADIR}/plugins/python3)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY box-tutorials DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/python3)

# ---------------------------------
# Test applications (uncomment to enable your test directory)
# ---------------------------------
#if(OV_COMPILE_TESTS)
#	ADD_SUBDIRECTORY(test)
#endif(OV_COMPILE_TESTS)

