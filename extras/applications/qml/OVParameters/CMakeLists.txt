# SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only
# From project gnomon: https://gitlab.inria.fr/gnomon/gnomon 

project(OVParameters VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(CMAKE_AUTOMOC ON)

add_subdirectory(src)

qt_add_qml_module(${PROJECT_NAME} STATIC
URI
${PROJECT_NAME}
VERSION
  1.0
PLUGIN_TARGET
  ${PROJECT_NAME}
QML_FILES
  BoolParameter.qml
  ColorParameter.qml
  ColorGradientParameter.qml
  ListParameter.qml
  MultiSelectParameter.qml
  NumericParameter.qml
  Parameters.qml
  PathParameter.qml
  StringParameter.qml)

target_compile_definitions(${PROJECT_NAME} PRIVATE -DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

target_link_libraries(${PROJECT_NAME} 
PUBLIC
  Qt::Gui
  OVControls
  openvibe-parameters-object
  openvibe
PRIVATE 
  Qt::Quick
)

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
