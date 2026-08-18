project(sdk-examples-communication-client-generator VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(SRC_FILES src/main-generator.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-module-communication
                      openvibe-module-ebml
					  openvibe-toolkit)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})


SET_BUILD_PLATFORM()

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

# -----------------------------
project(sdk-examples-communication-client-filter VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(SRC_FILES src/main-filter.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} openvibe-module-communication)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})

SET_BUILD_PLATFORM()

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
