project(openvibe-plugins-evaluation VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp)

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
qt_add_resources(${PROJECT_NAME} "resourcesEvaluation"
    FILES 
      qml/ClassifierAccuracyMeasure.qml
      qml/ROCCurve.qml
)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    Qt::Core
    Qt6::Charts
    Qt6::Gui
    Qt6::Quick
    openvibe-parameters-object

PUBLIC
  openvibe
  openvibe-common
  openvibe-toolkit
  tinyxml2::tinyxml2
  openvibe-visualization-toolkit
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

file(COPY share/ DESTINATION ${BUILD_DATADIR}/plugins/evaluation)

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# ---------------------------------
# Test applications
# ---------------------------------
if(OV_COMPILE_TESTS)
	ADD_SUBDIRECTORY(test)
endif(OV_COMPILE_TESTS)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/evaluation)

