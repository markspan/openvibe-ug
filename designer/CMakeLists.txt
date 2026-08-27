PROJECT(Designer VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

SET(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH_BASE}
	${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules
	# ${CMAKE_MODULE_PATH}
	)

# OpenViBE-specific helper functions that are used by the CMakeLists of the subprojects.
INCLUDE("OvAddProjects")
INCLUDE("OvDelayLoad")

# ----------------------
# Build CMake flags
# ----------------------
SET(OV_SOURCE_DEPENDENCIES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dependencies-source")

if(NOT(${Flag_VerboseOutput}))
	set(CMAKE_INSTALL_MESSAGE LAZY)
endif()

# ----------------------
# Set build version
# ----------------------
# These versions are used by the subprojects by default.
set_version()
MESSAGE(STATUS "BUILDING VERSION Designer: ${PROJECT_VERSION}")

# OpenViBE SDK modules list
SET(INCLUDED_OV_SDK_COMPONENTS BASE ALLPLUGINS EBML SYSTEM FS XML TOOLKIT)

SET(DESIGNER_NAME "Designer")
SET(LICENCE_TEXT "\
This program is free software: you can redistribute it and/or modify\
it under the terms of the GNU Affero General Public License as\
published by the Free Software Foundation, either version 3 of the\
License, or (at your option) any later version.\
\
This program is distributed in the hope that it will be useful, but\
WITHOUT ANY WARRANTY; without even the implied warranty of\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\
Affero General Public License for more details.\
\
You should have received a copy of the GNU Affero General Public\
License along with this program.  If not, see http://www.gnu.org/licenses/ .")

IF(OV_DISPLAY_ERROR_LOCATION)
	ADD_DEFINITIONS(-DOV_DISPLAY_ERROR_LOCATION)
ENDIF()

# The user files will be stored in .../OV_CONFIG_SUBDIR folder
ADD_DEFINITIONS("-DOV_PROJECT_NAME=\"openvibe\"")
ADD_DEFINITIONS("-DOV_CONFIG_SUBDIR=\"${OV_CONFIG_SUBDIR}\"")

IF(WIN32)
	ADD_DEFINITIONS("-DNOMINMAX -DBOOST_ALL_NO_LIB")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4355")
	SET(OV_WIN32_BOOST_VERSION "1_58")
ELSEIF(APPLE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
ELSEIF(UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
ELSE(WIN32)
	MESSAGE(WARNING "Warning: unknown platform")
ENDIF(WIN32)

# Print the used compilation parameters (for transparency)
GET_DIRECTORY_PROPERTY(TMP_DEFINITIONS COMPILE_DEFINITIONS)
debug_message("Compilation flags used at source root: ")
debug_message("  COMPILE_DEFINITIONS = '${TMP_DEFINITIONS}'")
debug_message("  CMAKE_CXX_FLAGS = '${CMAKE_CXX_FLAGS}'")
debug_message("  CMAKE_CXX_FLAGS_RELEASE = '${CMAKE_CXX_FLAGS_RELEASE}'")
debug_message("  CMAKE_CXX_FLAGS_DEBUG = '${CMAKE_CXX_FLAGS_DEBUG}'")

# if no specific branch has been given for an openvibe component, default is to compile its trunk folder
## SET(OV_TRUNK "trunc")

# By setting SKIP[_FOLDER]* you can skip a subtree (example: SKIP_A_B_C skips folder a/b/c and all its subfolders if any)

# Custom cmakelist can be used to overwrite the default compilation & packaging parameters
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CustomCMakeLists.txt")
	debug_message("Found custom build settings")
	INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/CustomCMakeLists.txt")
ENDIF()



ADD_DEFINITIONS(-DBRAND_NAME="${BRAND_NAME}")
ADD_DEFINITIONS(-DDESIGNER_NAME="${DESIGNER_NAME}")

# Used by the various Find* scripts to locate OpenViBE modules
SET(OV_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# needed for making visual studio projects when this script is called without CMAKE_BUILD_TYPE
SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:TARGET_BUILDTYPE_Debug>)
SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>:TARGET_BUILDTYPE_Release>)
SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:RelWithDebInfo>:TARGET_BUILDTYPE_Release>)

# Add vendor specific installation instructions
# Traverse these directories and build their components

IF(NOT(SKIP_PLUGINS))
	ADD_SUBDIRECTORY("plugins/")
ENDIF()

IF(NOT(SKIP_APPLICATIONS))
	ADD_SUBDIRECTORY("applications/")
ENDIF()

IF(NOT(SKIP_LIBRARIES))
	ADD_SUBDIRECTORY("libraries/")
ENDIF()

IF(NOT(SKIP_VISUALIZATION_TOOLKIT))
	ADD_SUBDIRECTORY("visualization-toolkit/")
ENDIF()

# add the scripts to the project so IDEs using the CMake file are aware of them
FILE(GLOB_RECURSE script_files scripts/*.cmd scripts/*.sh )
ADD_CUSTOM_TARGET(openvibe-designer-scripts SOURCES ${script_files})
set_target_properties(openvibe-designer-scripts PROPERTIES
	FOLDER ${MISC_FOLDER}
)
