project(openvibe-module-system VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(SRC_FILES
	include/system/defines.h
	include/system/ovCChrono.h
	include/system/ovCMath.h
	include/system/ovCMemory.h
	include/system/ovCTime.h
	include/system/OSAL.h
	src/ovCChrono.cpp
	src/ovCMath.cpp
	src/ovCMemory.cpp
	src/ovCTime.cpp
	src/OSAL.cpp
	)

if(WIN32)
	list(APPEND SRC_FILES "include/system/WindowsUtilities.h" "src/WindowsUtilities.cpp")
endif()

include("FindSourceRCProperties")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe-common)

target_include_directories(${PROJECT_NAME} 
						   PUBLIC include)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${MODULES_FOLDER}
)

target_compile_options(${PROJECT_NAME}
					   PRIVATE -DSystem_Exports
					   PUBLIC -DSystem_Shared 
)

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.0")
	target_link_libraries(${PROJECT_NAME}
						  Boost::boost
						  Boost::system
						  Boost::thread
						  Boost::chrono
	)
endif()

# ---------------------------------
# Finds standard library winmm
# Adds library to target
# Adds include path
# ---------------------------------
if(WIN32)
	include("OvSetWindowsSDKPath")
	
	find_library(LIB_STANDARD_MODULE_WINMM winmm ${OV_MS_SDK_PATH}/lib)
	
	if(LIB_STANDARD_MODULE_WINMM)
		message(STATUS "  Found winmm...")
		target_link_libraries(${PROJECT_NAME} ${LIB_STANDARD_MODULE_WINMM})
	else(LIB_STANDARD_MODULE_WINMM)
		message(STATUS "  FAILED to find winmm...")
	endif(LIB_STANDARD_MODULE_WINMM)

	find_library(LIB_STANDARD_MODULE_DBGHELP dbghelp ${OV_MS_SDK_PATH}/lib)

	if(LIB_STANDARD_MODULE_DBGHELP)
		message(STATUS "  Found dbghelp...")
		target_link_libraries(${PROJECT_NAME} ${LIB_STANDARD_MODULE_DBGHELP})
	else(LIB_STANDARD_MODULE_DBGHELP)
		message(STATUS "  FAILED to find dbghelp...")
	endif(LIB_STANDARD_MODULE_DBGHELP)
else()
	target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
endif()

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

INSTALL(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
