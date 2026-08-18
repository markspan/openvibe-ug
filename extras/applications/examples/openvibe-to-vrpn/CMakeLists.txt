project(openvibe-example-openvibe-to-vrpn VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl include/*.h)
include_directories(include)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} vrpn)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})

include("FindThirdPartyPThread") # needed on Linux for VRPN

file(COPY box-tutorials DESTINATION ${BUILD_DATADIR}/scenarios)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY box-tutorials/ DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials)

install_root_alias($<TARGET_FILE_NAME:${PROJECT_NAME}>)
