# SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only
# From project gnomon: https://gitlab.inria.fr/gnomon/gnomon 

project(OVControls VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(CMAKE_AUTOMOC ON)

#
# SVG Icons are imported as resources
# Source: https://pictogrammers.com/
#
file(GLOB resources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/resources/*.svg)

# Qt QML module
qt_add_qml_module(${PROJECT_NAME} STATIC
URI
${PROJECT_NAME}
VERSION
  1.0
PLUGIN_TARGET
  ${PROJECT_NAME}
QML_FILES
  Button.qml
  CloseButton.qml
  ComboBox.qml
  ComboBoxDelegate.qml
  Dragger.qml
  Icon.qml
  Logger.qml
  Panel.qml
  ProgressBar.qml
  Separator.qml
  Slider.qml
  SpinBox.qml
  TextField.qml
  Toast.qml
  ToolTip.qml
SOURCES
  OVLogController.cpp
  OVLogController.h
  OVLogControllerItem.cpp
  OVLogControllerItem.h
  OVQmlLogListener.cpp
  OVQmlLogListener.h
  QmlHelper.h
  QmlHelper.cpp
RESOURCES
  ${resources}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE -DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} 
PUBLIC
  Qt::Gui
PRIVATE 
  Qt::Quick
  openvibe-kernel
)

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
