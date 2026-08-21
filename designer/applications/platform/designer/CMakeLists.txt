project(designer VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(CMAKE_AUTOMOC ON)

SET_BUILD_PLATFORM()
add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)
add_subdirectory(OVVisualization)

include("FindSourceDependencyJSON")

if(WIN32)
	# ov_configure_rc falls back to the target name ("designer") for the product name, which is what
	# Explorer and Add/Remove Programs display.
	set(PROJECT_PRODUCT_NAME "OpenViBE Designer")
	ov_configure_rc(NAME ${PROJECT_NAME} ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/share/ov-icon.ico)
	set(OV_DESIGNER_RC "${CMAKE_BINARY_DIR}/resource-files/${PROJECT_NAME}.rc")
endif()

qt_add_executable(${PROJECT_NAME}
  src/BoxObject.h
  src/BoxObject.cpp
  src/BoxTreeItem.h
  src/BoxTreeItem.cpp
  src/BoxTreeModel.h
  src/BoxTreeModel.cpp
  src/BoxTreeModelFilter.h
  src/CommentObject.h
  src/CommentObject.cpp
  src/DesignerScenario.h
  src/DesignerScenario.cpp
  src/IOPort.h
  src/IOPort.cpp
  src/LinkObject.h
  src/LinkObject.cpp
  src/main.cpp
  src/OVDesigner.h
  src/OVDesigner.cpp
  src/ParameterHelper.h
  src/ParameterHelper.cpp
  src/PlayerBridge.h
  src/PlayerBridge.cpp
  src/QmlEngineSingleton.h
  src/ScenarioIOConnection.h
  src/ScenarioIOConnection.cpp
  src/ScenarioParameterConnectionObject.h
  src/ScenarioParameterConnectionObject.cpp
  src/WindowManager.h
  src/WindowManager.cpp
  src/WindowManagerNode.h
  src/WindowManagerNode.cpp
  src/visualization/CVisualizationTree.hpp
  src/visualization/CVisualizationTree.cpp
  src/visualization/CVisualizationWidget.hpp
  src/visualization/CVisualizationWidget.cpp
  ${json_source_files}
  ${OV_DESIGNER_RC}
)

if(WIN32)
	# Avoids console allocation when program is NOT ran from a terminal.
  # Basically avoids the console when click on the app icon, while keeping it in terminals.
	set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
endif()

set_source_files_properties(MenuModel.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(ShortcutManager.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Keep UI simpler by accessing icon with ":/qt/qml/designer/ov-icon.ico" and not worrying
# about the real file path
set_source_files_properties(share/ov-icon.ico PROPERTIES QT_RESOURCE_ALIAS ov-icon.ico)

qt_add_qml_module(${PROJECT_NAME}
URI
  ${PROJECT_NAME}
VERSION
  ${PROJECT_VERSION}
QML_FILES
  ActionBar.qml
  BoxSettings.qml
  Footer.qml
  Main.qml
  MenuModel.qml
  ScenarioTab.qml
  SettingsMenu.qml
  SettingsPopup.qml
  ShortcutManager.qml
  Shortcuts.qml
  Toolbox.qml
  TopBar.qml
RESOURCES
  share/ov-icon.ico)

target_include_directories(${PROJECT_NAME} PRIVATE src)
#target_compile_definitions(${PROJECT_NAME} PRIVATE QML_PATH=\"${QT_QML_OUTPUT_DIRECTORY}\")

target_link_libraries(${PROJECT_NAME}
PRIVATE
  Qt6::Quick
  Qt6::QuickControls2
  Qt6::Charts
  Qt6::Widgets
  openvibe
  openvibe-kernel
  openvibe-toolkit
  openvibe-module-system
  openvibe-module-system-delayload
  openvibe-module-fs
  openvibe-module-communication
  openvibe-parameters-object
  #openvibe-plugins-designer-qml-vis
  OVControls
  OVParameters
  OVStyle
  OVVisualization
  DesignSystem
  tinyxml2::tinyxml2
)

if (MSVC)
  target_link_options(${PROJECT_NAME} PRIVATE "/DELAYLOAD:Python${PY_VER_MAJOR}${PY_VER_MINOR}.dll")
endif()

# todo: review CMakelists.txt in visualization-toolkit and add to target_link_libraries
include("FindOpenViBEVisualizationToolkit")

configure_file(share/designer.conf-base "${BUILD_DATADIR}/applications/designer/designer.conf" @ONLY)

file(COPY share/ DESTINATION ${BUILD_DATADIR}/applications/designer PATTERN "*-base*" EXCLUDE)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openvibe-designer
COMMENT "Creating symlink to ${PROJECT_NAME}")

# Add Qt dependencies to build
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --qmldir ${CMAKE_SOURCE_DIR} $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Deploying Qt libraries"
        )
    endif()
endif()

install(FILES "${BUILD_DATADIR}/applications/designer/designer.conf" DESTINATION ${DIST_DATADIR}/openvibe/applications/designer)
install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/designer PATTERN "*-base*" EXCLUDE PATTERN "ov-icon-*.png" EXCLUDE)
  
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${DIST_BINDIR}
    LIBRARY DESTINATION ${DIST_LIBDIR}
    ARCHIVE DESTINATION ${DIST_LIBDIR})
