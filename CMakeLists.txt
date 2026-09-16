cmake_minimum_required(VERSION 3.20)

# The git tag is the source of truth for the version. This runs before project(), so it is
# included by full path: CMAKE_MODULE_PATH is only set up below, once a project exists.
# Sets OPENVIBE_{MAJOR,MINOR,PATCH}_VERSION plus OPENVIBE_VERSION_{FULL,SERIES,PRERELEASE}.
include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/OvVersion.cmake)
ov_compute_version()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(OpenVIBE VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})
message(STATUS "OpenViBE version: ${OPENVIBE_VERSION_FULL} (series ${OPENVIBE_VERSION_SERIES}, from ${OPENVIBE_VERSION_SOURCE})")

set(CMAKE_MODULE_PATH_BASE "${CMAKE_SOURCE_DIR}/CMake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_BASE}
                      ${CMAKE_MODULE_PATH_BASE}/drivers
)

set(CMAKE_AUTOMOC ON)

## #################################################################
## Generate compilation database
## #################################################################

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")


## ###################################################################
## Output directory setup
## ###################################################################
include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if (WIN32 AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' on Windows because of GTK and boost dependencies")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Win32 Release build only because of GTK and boost dependencies" FORCE)
endif(WIN32 AND NOT CMAKE_CONFIGURATION_TYPES)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include("Utilities")
include("OvPrint")
include("OvConfigureRC")

set(OV_LAUNCHER_SOURCE_PATH "${CMAKE_SOURCE_DIR}/CMake/launchers")
set(COMPANY_NAME "Inria")
SET(BRAND_NAME "openvibe")
string(TIMESTAMP COPYRIGHT_DATE "2012-%Y")

# Icon shared by applications & installer
set(OV_ICON_FILE "${CMAKE_SOURCE_DIR}/share/ov-icon.ico")

set(CMAKE_INSTALL_LIBDIR lib)  # Enforce as all distros may not have the same default (e.g. lib, lib32/lib64)
# Set CMake variables
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/../dist/" CACHE PATH "Install directory" FORCE)
endif()

# Install RPATH. No effect on Windows, which has no RPATH.
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
	# The origin-relative entries come first so that the installed tree keeps resolving its own
	# libraries after being moved -- a packaged archive is extracted somewhere other than
	# CMAKE_INSTALL_PREFIX by definition. Executables live in <root>/bin and libraries in <root>/lib,
	# except for the plugins, which outputPath.cmake installs next to the executables.
	if(APPLE)
		set(OV_RPATH_ORIGIN "@loader_path")
	else()
		set(OV_RPATH_ORIGIN "$ORIGIN")
	endif()
	# The absolute prefix is kept as a trailing fallback, for running straight out of dist/.
	set(CMAKE_INSTALL_RPATH "${OV_RPATH_ORIGIN}/../${CMAKE_INSTALL_LIBDIR}" "${OV_RPATH_ORIGIN}"
	                        "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif("${isSystemDir}" STREQUAL "-1")


# Todo: handle multi config generators
#foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
#  string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIGU )
#  string(CONCAT DIST_ROOT ${DIST_ROOT} $<$<CONFIG:${OUTPUTCONFIGU}>:${CMAKE_INSTALL_PREFIX}/${OUTPUTCONFIG}>)
#  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIGU} "${CMAKE_INSTALL_PREFIX}/${OUTPUTCONFIG}/bin" )
#  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIGU} "${CMAKE_INSTALL_PREFIX}/${OUTPUTCONFIG}/lib" )
#endforeach()


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(BUILD_DATADIR ${CMAKE_BINARY_DIR}/share/openvibe)

# The DIST_* variables below are *relative* to CMAKE_INSTALL_PREFIX: an absolute install(DESTINATION)
# ignores DESTDIR and CPack's staging directory, which would make "cpack" package an empty tree.
# DIST_ROOT_ABSOLUTE is for the few consumers that are not install() destinations.
set(DIST_ROOT_ABSOLUTE ${CMAKE_INSTALL_PREFIX})

set(DIST_ROOT .)
set(DIST_BINDIR ${CMAKE_INSTALL_BINDIR})
set(DIST_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(DIST_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})
set(DIST_DATADIR ${CMAKE_INSTALL_DATADIR})

# Fallback paths for lookup done in "openvibe/StringDirectories.hpp". They are only used if OV_PATH_* (e.g. OV_PATH_ROOT) is set.
# They are not needed unless someone knows exactly what they are doing. 
# @TODO: remove them and their usages ?
add_definitions(-DOV_CMAKE_PATH_ROOT="${DIST_ROOT_ABSOLUTE}")
add_definitions(-DOV_CMAKE_PATH_BIN="${DIST_ROOT_ABSOLUTE}/${CMAKE_INSTALL_BINDIR}")
add_definitions(-DOV_CMAKE_PATH_LIB="${DIST_ROOT_ABSOLUTE}/${CMAKE_INSTALL_LIBDIR}")
add_definitions(-DOV_CMAKE_PATH_DATA="${DIST_ROOT_ABSOLUTE}/${CMAKE_INSTALL_DATADIR}/openvibe")

# Where the user's settings, recent-scenario list and logs live: %APPDATA%\OpenVIBE-4.0 or
# ~/.config/OpenVIBE-4.0. Keyed on the MAJOR.MINOR series, not the full version, so a beta, its
# release and later patches all share one directory and a user does not lose their settings
# moving between them. Only a minor bump starts a fresh one.
#
# Deliberately not a CACHE variable: as a cache entry it kept whatever value the tree was first
# configured with, so a version bump silently went on writing to the previous release's directory
# until someone wiped the cache.
set(OV_CONFIG_SUBDIR "${PROJECT_NAME}-${OPENVIBE_VERSION_SERIES}")
set(OVT_TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/test-data")


# Options
option(Flag_VerboseOutput "Verbose CMake output" OFF)
option(OV_DISPLAY_ERROR_LOCATION "display complete error locations" ON)
option(BUILD_VALIDATION_TEST "Build the validation tests" ON)
option(BUILD_UNIT_TEST "Build the unit tests" ON)
option(BUILD_GCOV "Add coverage compilation options" OFF)

message(STATUS "META OPTIONS : ")
message(STATUS "  Flag_VerboseOutput = '${Flag_VerboseOutput}'")
message(STATUS "  OV_DISPLAY_ERROR_LOCATION = '${OV_DISPLAY_ERROR_LOCATION}'")
message(STATUS "  BUILD_VALIDATION_TEST = '${BUILD_VALIDATION_TEST}'")
message(STATUS "  BUILD_UNIT_TEST = '${BUILD_UNIT_TEST}'")

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
find_package(PythonBuildStandalone)
include(DeployPythonBuildStandalone)

include("FindThirdPartyBoost")
include("FindThirdPartyEigen")
include("FindThirdPartyExpat")
include("FindThirdPartyFFTW3")
include("FindThirdPartyGTest")
include("FindThirdPartyITPP")
include("FindThirdPartyLSL")
include("FindThirdPartyLua")
include("FindThirdPartyNlohmannJSON")
include("FindThirdPartyOpenAL")
include("FindThirdPartyTinyXML2")
include("FindThirdPartyVRPN")
include("FindThirdPartyZLib")
include("FindThirdPartyOpenGL")
include("FindThirdPartyXercesC")
include("FindThirdPartyQt")

qt_policy(SET QTP0004 NEW)

# #############################################################################
# Find Acquisition Drivers
include("FindThirdPartyBioSemiLabViewAPI")
include("FindThirdPartyBrainProductsAmplifierAPI")
include("FindThirdPartyEemagineEEGO")
include("FindThirdPartyGTecUnicorn")
include("FindThirdPartyGUSBampCAPI")
include("FindThirdPartyMicromed")
include("FindThirdPartyTMSI")

# #############################################################################
# install conda env libraries for execution
# excluding a few that are not needed at runtime and collide with GTK dlls
if (WIN32)
  # $env{CONDA_PREFIX} contains '\' on WIN32, causing parsing issues.
  # Converting it to CMake PATH solves it.
  file(TO_CMAKE_PATH $ENV{CONDA_PREFIX} CONDA_PREFIX)

	install(DIRECTORY ${CONDA_PREFIX}/Library/bin/
			DESTINATION ${DIST_BINDIR} 
			FILES_MATCHING PATTERN "*.dll"
      PATTERN "*.bit"
      PATTERN "intl.dll" EXCLUDE
      PATTERN "intl-8.dll" EXCLUDE
      PATTERN "iconv.dll" EXCLUDE
      PATTERN "gtk-2.0" EXCLUDE # GTK2 copy managed in GTK Find module
  )

  install(DIRECTORY ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}/
      DESTINATION ${DIST_BINDIR}
  )
  install(DIRECTORY ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}/
      DESTINATION ${DIST_ROOT}/plugins/
  )
  install(DIRECTORY ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}/
      DESTINATION ${DIST_ROOT}/qml/
  )

  file (COPY ${CONDA_PREFIX}/Library/bin/
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "*.bit"
        PATTERN "intl.dll" EXCLUDE
        PATTERN "intl-8.dll" EXCLUDE
        PATTERN "iconv.dll" EXCLUDE
        PATTERN "gtk-2.0" EXCLUDE # GTK2 copy managed in GTK Find module
  )

endif()

# #############################################################################
# Add test coverage options

if (BUILD_GCOV)
# From https://gcc.gnu.org/onlinedocs/gcc-6.1.0/gcc/Instrumentation-Options.html#Instrumentation-Options
# --coverage
# This option is used to compile and link code instrumented for coverage analysis. The option is a synonym for -fprofile-arcs -ftest-coverage (when compiling) and -lgcov (when linking). See the documentation for those options for more details.
# The code have to be execute to be able to see any results
# https://en.wikipedia.org/wiki/Gcov#Example
  set(GCC_COVERAGE_COMPILE_FLAGS " --coverage -O0 -g -fPIC -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

endif()

# #############################################################################
# CMake Utils
include(GenerateExportHeader)

# #############################################################################
# CTest configuration
# enable_testing() must be called here (repo root) rather than in a nested CMakeLists.txt,
# so its "testing enabled" directory property propagates to every subdirectory (sdk/, extras/),
# letting a single "ctest" invocation from the build root discover all native (add_test /
# gtest_discover_tests) suites project-wide.

# Temporary dirs used by sdk/unit-test and sdk/validation-test around each Test stage run
# (must match what sdk/unit-test/CMakeLists.txt and sdk/validation-test/CMakeLists.txt compute locally)
set(OVT_UNIT_TEST_TEMPORARY_DIR ${CMAKE_BINARY_DIR}/sdk/Testing/Temporary/OVT/)
set(OVT_VALIDATION_TEST_TEMPORARY_DIR ${CMAKE_BINARY_DIR}/sdk/validation-test/Testing/Temporary/OVT/)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake @ONLY)

# Memcheck configuration
IF(UNIX)
	find_program(MEMORYCHECK_COMMAND valgrind /usr/bin/valgrind)
	file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/data/resources/valgrind.supp" MEMORYCHECK_SUPPRESSIONS_FILE)
	set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --run-libc-freeres=no --show-possibly-lost=no" )
ELSEIF(WIN32)
	find_program(MEMORYCHECK_COMMAND purify "C:/Program Files (x86)/")
ENDIF()

# Timeout must be set first
set(DART_TESTING_TIMEOUT 72000 CACHE STRING "Maximum time allowed before CTest will kill the test." FORCE)
include(CTest)
enable_testing()

# #############################################################################
# Add OpenViBE subdirectories

# Add SDK to build
add_subdirectory(sdk)

# Add extras to build
add_subdirectory(extras)

# Add Designer to build
add_subdirectory(designer)

# The plugin-inspector renders its box snapshots with the designer's Box.qml, but it is
# configured back in extras/ before OVVisualization exists. Import the QML plugins here,
# once the module is known, otherwise the tool fails at runtime with
# "module OVVisualization is not installed".
qt_import_qml_plugins(openvibe-plugin-inspector)

# Documentation and (notably plugin-inspector) depend on every target shared objects.
function(ov_collect_targets_recursive out_var dir)
	get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
	get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
	foreach(subdir IN LISTS subdirs)
		ov_collect_targets_recursive(collected "${subdir}")
		list(APPEND targets ${collected})
	endforeach()
	set(${out_var} "${targets}" PARENT_SCOPE)
endfunction()

# Keep only the plugin libraries themselves: the name prefix also matches the Qt/AUTOMOC helper targets
# (<plugin>_autogen, and for QML-module plugins <plugin>_qmllint*, _qmltyperegistration, _resources_N...).
ov_collect_targets_recursive(OV_ALL_TARGETS "${CMAKE_SOURCE_DIR}")
set(OV_PLUGIN_TARGETS "")
foreach(target IN LISTS OV_ALL_TARGETS)
	if(target MATCHES "^openvibe-plugins-")
		get_target_property(target_type ${target} TYPE)
		if(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
			list(APPEND OV_PLUGIN_TARGETS ${target})
		endif()
	endif()
endforeach()

# Add Documentation to Build. This only defines the (ALL-excluded) "doc" target, so it is cheap
# enough to configure unconditionally: build the documentation with "ninja doc".
add_subdirectory(documentation)

# #############################################################################
# Packaging. Must stay last: it ends with include(CPack), which snapshots the CPACK_* variables.
include("OvPackaging")
