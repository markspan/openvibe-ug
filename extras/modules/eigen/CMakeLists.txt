project(openvibe-module-eigen VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  Eigen3::Eigen
)

target_include_directories(${PROJECT_NAME}
						   PRIVATE include/eigen
						   PUBLIC include)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${MODULES_FOLDER}
					  COMPILE_FLAGS "-DOV_Shared -DOV_Exports")


# ---------------------------------
# Target macros
# Defines target operating system, architecture, compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")

# ---------------------------------
# Test applications
# ---------------------------------
if(OV_COMPILE_TESTS)
ADD_SUBDIRECTORY(test)
endif()