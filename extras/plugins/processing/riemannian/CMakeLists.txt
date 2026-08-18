project(openvibe-plugins-riemannian VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp src/*.h src/*.inl src/*.c)
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-eigen
					  Boost::boost
					  Eigen3::Eigen
					  tinyxml2::tinyxml2)

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include_directories("src")

# OpenViBE Module
include("FindModuleGeometry")

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

set(SUB_DIR_NAME riemannian)

file(COPY box-tutorials/ DESTINATION ${BUILD_DATADIR}/scenarios/box-tutorials/${SUB_DIR_NAME})
file(COPY bci-examples/ DESTINATION ${BUILD_DATADIR}/scenarios/bci-examples/${SUB_DIR_NAME})

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY box-tutorials/       DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials/${SUB_DIR_NAME})	
install(DIRECTORY bci-examples/       DESTINATION ${DIST_DATADIR}/openvibe/scenarios/bci-examples/${SUB_DIR_NAME})	

add_subdirectory(test)
