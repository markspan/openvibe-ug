project(OVASInterface VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})


qt_add_qml_module(${PROJECT_NAME} STATIC
URI
  ${PROJECT_NAME}
VERSION
  1.0
PLUGIN_TARGET
  ${PROJECT_NAME}
SOURCES
  ImpedanceItem.h
  OVMaster.h
  OVMaster.cpp)

target_compile_definitions(${PROJECT_NAME} PRIVATE -DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DTARGET_HAS_OpenViBEContributions)

target_link_libraries(${PROJECT_NAME} 
                      PUBLIC 
                        Qt::Gui
                      PRIVATE 
                        Qt::Quick
                        OVASCore
                        OVASDrivers
                        OVControls
                        openvibe-parameters-object
                        openvibe-module-system
                        openvibe-kernel
)

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
