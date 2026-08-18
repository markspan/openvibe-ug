project(openvibe-common VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE include)

# A headers-only project, the executable is a NOP having some static asserts, not to be installed.
add_executable(${PROJECT_NAME}-nop src/main.cpp)

target_link_libraries(${PROJECT_NAME}-nop
					  ${PROJECT_NAME}
					  Boost::boost
)
	
set_target_properties(${PROJECT_NAME}-nop PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${KERNEL_FOLDER})

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------

INSTALL(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
