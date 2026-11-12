project(openvibe-plugins-simple-visualization VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(SRC_FILES
    src/main.cpp
	src/defines.hpp
    src/SDisplaySettings.h
    src/SignalScaling.h  # UG fork
    src/CTraceBuffer.cpp
    src/CTraceBuffer.h
    src/CSignalDisplayController.cpp
    src/CSignalDisplayController.h
    src/box-algorithms/CBoxAlgorithmErpPlot.cpp
    src/box-algorithms/CBoxAlgorithmErpPlot.h
    src/box-algorithms/CBoxAlgorithmLevelMeasure.cpp
    src/box-algorithms/CBoxAlgorithmLevelMeasure.h
    # UG fork: the P300 speller display, ported to Qt
    src/box-algorithms/CBoxAlgorithmP300SpellerVisualization.cpp
    src/box-algorithms/CBoxAlgorithmP300SpellerVisualization.hpp
    src/box-algorithms/CP300SpellerView.cpp
    src/box-algorithms/CP300SpellerView.h
    src/box-algorithms/P300SpellerState.cpp
    src/box-algorithms/P300SpellerState.h
	src/box-algorithms/CDisplayCueImage.cpp
	src/box-algorithms/CDisplayCueImage.h
    src/box-algorithms/CGraphWindow.cpp
    src/box-algorithms/CGraphWindow.h
    src/box-algorithms/CGrazMultiVisualization.cpp
    src/box-algorithms/CGrazMultiVisualization.h    
    src/box-algorithms/CGrazMultiVisuWindow.cpp
    src/box-algorithms/CGrazMultiVisuWindow.h
    src/box-algorithms/CGrazVisualization.cpp
    src/box-algorithms/CGrazVisualization.h    
    src/box-algorithms/CGrazVisuWindow.cpp
    src/box-algorithms/CGrazVisuWindow.h
    src/box-algorithms/CImageDisplayObject.cpp
	src/box-algorithms/CImageDisplayObject.h
    src/box-algorithms/CSignalDisplay.cpp
    src/box-algorithms/CSignalDisplay.h
    src/box-algorithms/LevelMeasureWindow.cpp
    src/box-algorithms/LevelMeasureWindow.h
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

qt_add_qml_module(${PROJECT_NAME}
    URI OpenViBE.SignalDisplay
    VERSION 1.0
    NO_PLUGIN
    SOURCES
      src/CSignalTraceItem.cpp
      src/CSignalTraceItem.h
    QML_FILES
      qml/SignalDisplay.qml
      qml/ChannelGutter.qml
      qml/AmplitudeScaleBar.qml
      qml/TimeRuler.qml
      qml/StimulationOverlay.qml
)

qt_add_resources(${PROJECT_NAME} "resourcesSimpleVisualization"
    FILES
      qml/OVDisplayCueImage.qml
      qml/CGraphWindow.qml
      qml/CGrazVisuWindow.qml
      qml/CGrazMultiVisuWindow.qml
      qml/LevelMeasureWindow.qml
      qml/P300SpellerVisualization.qml  # UG fork
)

target_link_libraries(${PROJECT_NAME}
                      PUBLIC
                      Qt6::Core
                      Qt6::Charts # still used by the ERP plot and the Graz visualizations
                      Qt6::Gui
                      Qt6::Quick
                      openvibe
                      openvibe-common
                      openvibe-toolkit
                      openvibe-module-system
                      openvibe-parameters-object
                      openvibe-visualization-toolkit
                      tinyxml2::tinyxml2 # UG fork: reads the P300 speller's grid file
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include("FindOpenViBEModuleTCPTagging")

file(COPY share/ DESTINATION ${BUILD_DATADIR}/plugins/simple-visualization)
file(COPY box-tutorials DESTINATION ${BUILD_DATADIR}/scenarios/)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/simple-visualization)
install(DIRECTORY box-tutorials DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)

add_subdirectory(test)
