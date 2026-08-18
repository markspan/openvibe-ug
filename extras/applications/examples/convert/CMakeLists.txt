
if(OV_DISABLE_GTK)
  message(STATUS  "Skipping Convert, no GTK (no Designer)")
  return()
endif()

project(openvibe-convert VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

ADD_CUSTOM_TARGET(openvibe-convert SOURCES
	CMakeLists.txt src/openvibe-convert-real.cmd-base src/openvibe-convert-real.sh-base)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})

ADD_DEPENDENCIES(openvibe-convert openvibe-plugins-file-io)

# ----------------------
# Generate launch script
# ----------------------
if(WIN32) 
	set(OV_EXTENSION "cmd")
elseif(UNIX)
	set(OV_EXTENSION "sh")
endif()

# Create the actual script thats run
CONFIGURE_FILE(src/openvibe-convert-real.${OV_EXTENSION}-base ${CMAKE_CURRENT_BINARY_DIR}/openvibe-convert-real.${OV_EXTENSION} @ONLY)

# -----------------------------
# Install files
# -----------------------------

file(COPY share/ DESTINATION ${BUILD_DATADIR}/scenarios/convert)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/openvibe-convert-real.${OV_EXTENSION} DESTINATION ${DIST_BINDIR})

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/scenarios/convert)

install_root_alias(openvibe-convert.${OV_EXTENSION} openvibe-convert-real.${OV_EXTENSION})

