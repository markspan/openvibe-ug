project(openvibe-kernel VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)

include("FindSourceDependencyLepton")
include("FindSourceRCProperties")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-module-fs
					  openvibe-module-system
					  openvibe-module-xml
					  Boost::boost
)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${KERNEL_FOLDER}
					  COMPILE_FLAGS "-DOVK_Exports -DOVK_Shared")

# ---------------------------------
# Finds standard library dl
# Adds library to target
# Adds include path
# ---------------------------------
if(UNIX)
	find_library(LIB_STANDARD_MODULE_DL dl)
	if(LIB_STANDARD_MODULE_DL)
		message(STATUS "  Found dl...")
		target_link_libraries(${PROJECT_NAME} ${LIB_STANDARD_MODULE_DL})
	else(LIB_STANDARD_MODULE_PTHREAD)
		message(STATUS "  FAILED to find dl...")
	endif(LIB_STANDARD_MODULE_DL)
endif(UNIX)

# ---------------------------------
# Finds standard library pthread
# Adds library to target
# Adds include path
# ---------------------------------
if(UNIX)
	find_library(LIB_STANDARD_MODULE_PTHREAD pthread)
	if(LIB_STANDARD_MODULE_PTHREAD)
		message(STATUS "  Found pthread...")
		target_link_libraries(${PROJECT_NAME} ${LIB_STANDARD_MODULE_PTHREAD})
	else(LIB_STANDARD_MODULE_PTHREAD)
		message(STATUS "  FAILED to find pthread...")
	endif(LIB_STANDARD_MODULE_PTHREAD)
endif(UNIX)

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

file(COPY share/ DESTINATION ${BUILD_DATADIR}/kernel PATTERN "*-base" EXCLUDE)
install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/kernel PATTERN "*-base" EXCLUDE)

# -----------------------------
# Install and configure .conf
# -----------------------------
# This construct is to fetch the documentation specific to this version. 
# In addition, it tweaks the patch version to allow the "1.0.0+git" style version to 
# fetch the "1.0.0" (base) version documentation as we don't have git version docs on the web.
string(REGEX REPLACE "[^0-9]" "" PROJECT_VERSION_PATCH_PRUNED ${PROJECT_VERSION_PATCH})
set(OV_PROJECT_BASE_VERSION ${PROJECT_VERSION})
