PROJECT(openvibe-plugins-designer-advanced-vis)
if (NOT GTK2_FOUND)
  message(STATUS "GTK2 not found - skipping building ${PROJECT_NAME}")
  return()
endif()
SET(PROJECT_PRODUCT_NAME "Mensia Advanced Visualization Toolset")

FILE(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl include/*.h include/*.hpp)

INCLUDE("FindSourceRCProperties")

if (APPLE)
  message(WARNING "${PROJECT_NAME} will not be build on APPLE")
  return()
endif()

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_FILES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
					  ${RLM_LIB_FILES}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-system
					  mensia-advanced-visualization-static
                                          OpenGL::GL
)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

IF(MENSIA_ADVANCED_VISUALIZATION_EXPERIMENTAL)
	MESSAGE(STATUS "  MENSIA_ADVANCED_VISUALIZATION_EXPERIMENTAL is set, experimental visualization will be built.")
	ADD_DEFINITIONS(-DTARGET_Has_Experimental)
ENDIF()

INCLUDE("FindOpenViBEVisualizationToolkit")

INCLUDE("FindLibMensiaAdvancedVisualisation")

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()


file(COPY share/ DESTINATION ${BUILD_DATADIR}/plugins/)
file(COPY box-tutorials/ DESTINATION ${BUILD_DATADIR}/scenarios/box-tutorials/advanced-visualization)

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins)
INSTALL(DIRECTORY box-tutorials/  DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials/advanced-visualization)
