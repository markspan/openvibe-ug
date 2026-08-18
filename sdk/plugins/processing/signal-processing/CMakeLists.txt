project(openvibe-plugins-sdk-signal-processing VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
include("FindSourceDependencyWavelib")
include("FindSourceRCProperties")
include("FindSourceDependencyDSPFilters")
include("FindSourceDependencyR8Brain")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-system
					  openvibe-module-fs
					  Boost::boost
					  Eigen3::Eigen)

target_include_directories(${PROJECT_NAME} PRIVATE src)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${PLUGINS_FOLDER}
					  COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)
add_definitions(-D_USE_MATH_DEFINES)	# Definition for constant math as M_PI

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

file(COPY box-tutorials DESTINATION ${BUILD_DATADIR}/scenarios/)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY box-tutorials/ DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials)

