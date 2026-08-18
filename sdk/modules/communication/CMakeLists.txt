project(openvibe-module-communication VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp include/*.h include/*.hpp)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe-common
					  openvibe-module-socket)

target_include_directories(${PROJECT_NAME}
						   PRIVATE include/communication
						   PUBLIC include)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${MODULES_FOLDER}
)

target_compile_options(${PROJECT_NAME}
					   PRIVATE -DCommunication_Exports
					   PUBLIC -DCommunication_Shared
)

SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
