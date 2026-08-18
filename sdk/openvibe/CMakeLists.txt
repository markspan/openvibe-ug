project(openvibe VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})


file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl include/*.h include/*.hpp)
include("FindSourceRCProperties")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe-common
					  nlohmann_json::nlohmann_json)

target_include_directories(${PROJECT_NAME} 
						   PRIVATE include/openvibe
						   PUBLIC include)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${KERNEL_FOLDER}
					  COMPILE_FLAGS "-DOV_Exports -DOV_Shared")

target_compile_options(${PROJECT_NAME}
					   PRIVATE -DOV_Exports
					   PUBLIC -DOV_Shared 
)

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

INSTALL(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
INSTALL(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")
