# ---------------------------------
# Finds libmensia-advanced-visualization
# Adds library to target
# Adds include path
# ---------------------------------
# OPTION(DYNAMIC_LINK_LIBADVVIZ "Dynamically link libmensia-advanced-visualization" ON)

# Default is dynamic library
IF(NOT DEFINED DYNAMIC_LINK_LIBADVVIZ)
	OPTION(DYNAMIC_LINK_LIBADVVIZ "Dynamically link lib mensia-advanced-visualization" OFF)
ENDIF()

IF(DYNAMIC_LINK_LIBADVVIZ)
	ADD_DEFINITIONS(-DLMAV_Shared)
	SET(LIBADVVIZ_LINKING "")
ELSE()
	ADD_DEFINITIONS(-DLMAV_Static)
	SET(LIBADVVIZ_LINKING -static)
ENDIF()

SET(PATH_LIBADVVIZ "PATH_LIBADVVIZ-NOTFOUND")
SET(MENSIA_SRC_DIR ${CMAKE_SOURCE_DIR}/designer/libraries/lib-advanced-visualization/include/)
FIND_PATH(PATH_LIBADVVIZ mensia/advanced-visualization.hpp PATHS ${MENSIA_SRC_DIR} NO_DEFAULT_PATH)
IF(PATH_LIBADVVIZ)
	debug_message( "  Found mensia-advanced-visualization... ${PATH_LIBADVVIZ}")
	IF(TARGET ${PROJECT_NAME})
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} mensia-advanced-visualization${LIBADVVIZ_LINKING})
		debug_message( "Linking with mensia-advanced-visualization${LIBADVVIZ_LINKING}")

	ENDIF()
	IF(TARGET ${PROJECT_NAME}-static)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME}-static mensia-advanced-visualization${LIBADVVIZ_LINKING})
		debug_message( "Linking with mensia-advanced-visualization${LIBADVVIZ_LINKING}")
	ENDIF()

	ADD_DEFINITIONS(-DTARGET_HAS_LibAdvancedVisualization)
ELSE()
	MESSAGE(WARNING "  FAILED to find mensia-advanced-visualization...")
ENDIF()
