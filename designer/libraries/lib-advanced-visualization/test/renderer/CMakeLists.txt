project(openvibe-lib-adv-viz-test-renderer VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})
message(STATUS "Now building ${PROJECT_NAME} ${PROJECT_VERSION} (${PROJECT_BRANCH}/${PROJECT_COMMITHASH})" )

add_executable(${PROJECT_NAME} main_test_renderer.cpp)

target_link_libraries(${PROJECT_NAME}
  mensia-advanced-visualization
  OpenGL::GL
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    FOLDER ${VALIDATION_FOLDER}
)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${DIST_BINDIR}
        LIBRARY DESTINATION ${DIST_LIBDIR}
        ARCHIVE DESTINATION ${DIST_LIBDIR})
