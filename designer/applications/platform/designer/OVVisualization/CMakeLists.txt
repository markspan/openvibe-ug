# SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

project(OVVisualization VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set_source_files_properties(StreamColorMapper.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${PROJECT_NAME} STATIC
URI
${PROJECT_NAME}
VERSION
  1.0
PLUGIN_TARGET
  ${PROJECT_NAME}
QML_FILES
  Box.qml
  BoxPort.qml
  BoxTreeView.qml
  Comment.qml
  CommentEditDialog.qml
  Link.qml
  PipelineCanvas.qml
  ScenarioConfiguration.qml
  ScenarioIOConnection.qml
  StreamColorMapper.qml
  WindowManager.qml
  WindowManagerNode.qml
  WindowManagerNodeComponent.qml
  components/IOCard.qml
  components/IOSelectPopup.qml
  components/WindowManagerSplitView.qml
NO_CACHEGEN)

target_compile_definitions(${PROJECT_NAME} PRIVATE -DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

target_link_libraries(${PROJECT_NAME} 
PUBLIC
  Qt::Gui
  OVControls
  OVStyle
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
