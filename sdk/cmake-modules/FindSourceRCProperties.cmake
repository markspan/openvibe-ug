# ---------------------------------
# Configure RC file
# Sets SRC_FILES to add the path to the newly created rc file
# Adds appropriate include dir
# ---------------------------------
IF(WIN32)
	OV_CONFIGURE_RC(NAME ${PROJECT_NAME})
	SET(SRC_FILES "${SRC_FILES};${CMAKE_BINARY_DIR}/resource-files/${PROJECT_NAME}.rc")
ENDIF()

