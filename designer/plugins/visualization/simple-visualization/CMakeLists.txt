PROJECT(openvibe-plugins-designer-simple-vis)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

FILE(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
INCLUDE("FindSourceRCProperties")
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_FILES})

qt_add_resources(${PROJECT_NAME} "resourcesDesignerSimpleVis"
	FILES
	  qml/CTopographicMap2DView.qml
	  qml/MatrixDisplayWindow.qml
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

target_link_libraries(${PROJECT_NAME}
	Qt6::Core
	Qt6::Charts
	Qt6::Gui
	Qt6::Quick
	Qt6::Graphs
	openvibe
  openvibe-toolkit
  openvibe-visualization-toolkit
)
add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

ADD_DEFINITIONS(-D_USE_MATH_DEFINES)	# Definition for constant math as M_PI

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

file(COPY share/ DESTINATION ${BUILD_DATADIR}/plugins/simple-visualization)

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/simple-visualization)
