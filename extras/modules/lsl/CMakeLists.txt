project(openvibe-module-lsl VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(SRC_FILES
	src/Utils.cpp
	include/lsl/Utils.hpp)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-module-system
					  LSL::lsl
)

target_include_directories(${PROJECT_NAME}
						   PRIVATE include/lsl
						   PUBLIC include)

set_target_properties(${PROJECT_NAME} PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}
		FOLDER ${MODULES_FOLDER})

if(UNIX)
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif(UNIX)

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
set_build_platform()

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
