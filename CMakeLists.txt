cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0026 OLD)
cmake_policy(SET CMP0039 NEW)
cmake_policy(SET CMP0042 OLD)
cmake_policy(SET CMP0043 OLD)
cmake_policy(SET CMP0045 OLD)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0057 NEW)


set(OPENVIBE_MAJOR_VERSION 3)
set(OPENVIBE_MINOR_VERSION 5)
set(OPENVIBE_PATCH_VERSION 0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(OpenVIBE VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(CMAKE_MODULE_PATH_BASE "${CMAKE_SOURCE_DIR}/CMake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_BASE}
		              ${CMAKE_MODULE_PATH_BASE}/drivers
)

include("Utilities")
include("OvPrint")
include("OvInstallLaunchScript")

set(BUILD_ARCH "x64" CACHE STRING "Architecture tu build for (x64 or x86)")
if(${BUILD_ARCH} STREQUAL "x64")
  set(DEPENDENCIES_ARCH "_x64")
endif()
if (UNIX)  #TODO: install deps in _x64 foler on unix and remove this condition
  set(DEPENDENCIES_ARCH "")
endif()

set(CMAKE_INSTALL_LIBDIR lib)  # Enforce as all distros may not have the same default (e.g. lib, lib32/lib64)
# Set CMake variables
get_property (GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(${GENERATOR_IS_MULTI_CONFIG})
	if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/../dist/${BUILD_ARCH}" CACHE PATH "Install directory" FORCE)
	endif()
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin" )
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib" )
	foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
		string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIGU )
		string(CONCAT DIST_ROOT ${DIST_ROOT} $<$<CONFIG:${OUTPUTCONFIGU}>:${CMAKE_INSTALL_PREFIX}/${OUTPUTCONFIG}>)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIGU} "${CMAKE_INSTALL_PREFIX}/${OUTPUTCONFIG}/bin" )
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIGU} "${CMAKE_INSTALL_PREFIX}/${OUTPUTCONFIG}/lib" )
	endforeach()
elseif(NOT OV_PACKAGE)
	# Mono-build but not packaging
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build Type (Debug, Release, RelWithDebInfo, MinSizeRel")
	if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/../dist/${BUILD_ARCH}/${CMAKE_BUILD_TYPE}" CACHE PATH "Install directory" FORCE)
	endif()
	
	set(DIST_ROOT ${CMAKE_INSTALL_PREFIX})
else()
	# Mono-build and packaging
	set(CPACK_GENERATOR "ZIP")
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenViBE Designer")
	set(CPACK_PACKAGE_VENDOR "Inria & Mensia Technologies")
	set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.md")
	set(CPACK_PACKAGE_VERSION ${OV_GLOBAL_VERSION_MAJOR}.${OV_GLOBAL_VERSION_MINOR}.${OV_GLOBAL_VERSION_PATCH}.${OV_GLOBAL_VERSION_BUILD})
	set(CPACK_PACKAGE_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
	if(UNIX)
		execute_process(COMMAND bash -c "echo -n $(lsb_release -i -s)$(lsb_release -r -s)"
			OUTPUT_VARIABLE CPACK_SYSTEM_NAME)
	endif()

	set(CPACK_PACKAGE_NAME "OpenViBE${CMAKE_PROJECT_NAME}-${CMAKE_BUILD_TYPE}-${BUILD_ARCH}")
	include(CPack)
endif()

# Setup knowledge of GNU-style install path variables
# Creates variables on inclusion. Therefore, must be called after setting CMAKE_INSTALL_PREFIX up which it is based.
INCLUDE("GNUInstallDirs")

set(DIST_BINDIR ${DIST_ROOT}/bin)
set(DIST_LIBDIR ${DIST_ROOT}/lib)
set(DIST_INCLUDEDIR ${DIST_ROOT}/include)
set(DIST_DATADIR ${DIST_ROOT}/share)
set(DIST_SYSCONFDIR ${DIST_ROOT}/etc)
set(DIST_BINDIR_ABSOLUTE ${DIST_BINDIR})

# Add directories that OpenViBE will use to look for its components runtime, unless overridden by environment variables in the launch scripts.
# These defines should only be used in "openvibe/ov_directories.h".
add_definitions(-DOV_CMAKE_PATH_ROOT="${DIST_ROOT}")
add_definitions(-DOV_CMAKE_PATH_BIN="${DIST_ROOT}/bin")
add_definitions(-DOV_CMAKE_PATH_LIB="${DIST_ROOT}/lib")
add_definitions(-DOV_CMAKE_PATH_DATA="${DIST_ROOT}/share/openvibe")

# Set OV variables
set(LIST_DEPENDENCIES_PATH "${CMAKE_SOURCE_DIR}/dependencies${DEPENDENCIES_ARCH}" CACHE PATH "Dependencies folder" )
# Create a path usable by Unix shell scripts (used by .sh launchers)
set(OV_PATH_DEPENCENCY_LIBS "")
foreach(TMP ${LIST_DEPENDENCIES_PATH})
	list(APPEND OV_PATH_DEPENDENCY_LIBS "${TMP}/lib")
endforeach()
string(REPLACE ";" ":" OV_PATH_PATH_DEPENDENCY_LIBS "${OV_PATH_DEPENDENCY_LIBS}")

set(OV_CONFIG_SUBDIR "${PROJECT_NAME}-${PROJECT_VERSION}" CACHE STRING "Subdirectory under user directory when configuration and logs will be saved")

set(OVT_TEST_DATA_DIR "${LIST_DEPENDENCIES_PATH}/test-input")


# Options
option(Flag_VerboseOutput "Verbose CMake output" OFF)
option(OV_DISPLAY_ERROR_LOCATION "display complete error locations" ON)
option(OV_PACKAGE "Create a package in build directory" OFF)
option(BUILD_VALIDATION_TEST "Build the validation tests" ON)
option(BUILD_UNIT_TEST "Build the unit tests" ON)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" OFF)

message(STATUS "META OPTIONS : ")
message(STATUS "  Flag_VerboseOutput = '${Flag_VerboseOutput}'")
message(STATUS "  OV_DISPLAY_ERROR_LOCATION = '${OV_DISPLAY_ERROR_LOCATION}'")
message(STATUS "  OV_PACKAGE = '${OV_PACKAGE}'")
message(STATUS "  BUILD_VALIDATION_TEST = '${BUILD_VALIDATION_TEST}'")
message(STATUS "  BUILD_UNIT_TEST = '${BUILD_UNIT_TEST}'")
message(STATUS "  BUILD_DOCUMENTATION = '${BUILD_DOCUMENTATION}'")

# Add some dependencies to PATH
#Could (SHOULD) be replaced with find_path / find_package functionnalities
set(ENV{PATH} "$ENV{PATH};${LIST_DEPENDENCIES_PATH}/expat/bin")
set(ENV{PATH} "$ENV{PATH};${LIST_DEPENDENCIES_PATH}/vcredist")
set(ENV{PATH} "$ENV{PATH};${LIST_DEPENDENCIES_PATH}/xerces-c/lib")
set(ENV{PATH} "$ENV{PATH};${LIST_DEPENDENCIES_PATH}/gtk/bin")

# By setting SKIP[_FOLDER]* you can skip a subtree (example: SKIP_A_B_C skips folder a/b/c and all its subfolders if any)
# Skip building documentation
# a list of all project which will be filled by the directory traversal. This is needed to generate the documentation.
set_property(GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS "")
set_property(GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS_BUILD_DIR "")

# Sort target into directories for better visualization in IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(APP_FOLDER Applications)
set(KERNEL_FOLDER Kernel)
set(MODULES_FOLDER Modules)
set(PLUGINS_FOLDER Plugins)
set(MISC_FOLDER Misc)
set(TESTS_FOLDER Unit-Tests)
set(VALIDATION_FOLDER Validation-Tests)

# #############################################################################
# Find external projects
include("FindThirdPartyBoost")
include("FindThirdPartyEigen")
include("FindThirdPartyGTest")
include("FindThirdPartyLua")
include("FindThirdPartyTinyXML2")
include("FindThirdPartyVRPN")

# #############################################################################
# Find Acquisition Drivers
include("FindThirdPartyBrainProductsAmplifierAPI")
include("FindThirdPartyEemagineEEGO")

# #############################################################################
# Add OpenViBE subdirectories

# Add SDK to build
add_subdirectory(sdk)

# Add Designer to build
add_subdirectory(designer)

# Add extras to build
add_subdirectory(extras)

# Add Documentation to Build
if(BUILD_DOCUMENTATION)
	add_subdirectory(documentation)
endif()
