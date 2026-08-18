project(openvibe-acquisition-server-qml VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

qt_add_executable(${PROJECT_NAME}
  main.cpp)

qt_add_qml_module(${PROJECT_NAME}
URI
  ${PROJECT_NAME}
VERSION
  ${PROJECT_VERSION}
QML_FILES
  main.qml
  Settgs.qml
  Impedances.qml
  ChannelRename.qml)

target_link_libraries(${PROJECT_NAME}
PRIVATE
  Qt::Quick
  Qt6::QuickControls2
  OVASInterface
  openvibe
  openvibe-parameters-object
  OVParameters
  OVControls
  OVStyle)

SET_BUILD_PLATFORM()

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${DIST_BINDIR}
  LIBRARY DESTINATION ${DIST_LIBDIR}
  ARCHIVE DESTINATION ${DIST_LIBDIR})
