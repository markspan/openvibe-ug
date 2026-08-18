if(OV_DISABLE_GTK)
    message(STATUS  "Skipping Tracker, no GTK")
    return()
endif(OV_DISABLE_GTK)

project(openvibe-tracker VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl include/*.h include/*.hpp)

# FIXME Laurent G : 19/09/2023
# Problems linking with mendia library, have to be created first
If (APPLE)
  message(WARNING "Problems linking with mendia library, have to be created first. ${PROJECT_NAME} will not be build on APPLE")
  return()
endif()

# @FIXME bad hack, we are cannibalizing a lot of non-exported code from Designer and Kernel
# Its better than copy-paste but it'd be preferable to either export the necessary materials
# or make another solution.
set(OV_AVP_PATH "${CMAKE_SOURCE_DIR}/designer/plugins/visualization/ovp-advanced-visualization/src/")
LIST(APPEND SRC_FILES ${OV_AVP_PATH}/defines.hpp
    ${OV_AVP_PATH}/GtkGL.cpp
    ${OV_AVP_PATH}/GtkGL.hpp
    ${OV_AVP_PATH}/VisualizationTools.cpp
    ${OV_AVP_PATH}/VisualizationTools.hpp
    ${OV_AVP_PATH}/TGtkGLWidget.hpp
    ${OV_AVP_PATH}/IRuler.hpp
)

file(GLOB RULER_FILES RELATIVE ${OV_AVP_PATH}/ruler/ *.hpp)
# message(STATUS "FOUND ${RULER_FILES} in ${OV_AVP_PATH}")
# AUX_SOURCE_DIRECTORY(${OV_AVP_PATH}/ruler RULER_FILES)
LIST(APPEND SRC_FILES ${RULER_FILES})

# from wip-jlindgre-feature-renderer-offset
add_definitions("-DRENDERER_SUPPORTS_OFFSET")

set(OV_DESIGNER_PATH "${CMAKE_SOURCE_DIR}/designer/applications/platform/designer/src/")
file(GLOB designer_files
    ${OV_DESIGNER_PATH}/dynamic_settings/*.h ${OV_DESIGNER_PATH}/dynamic_settings/*.cpp
    ${OV_DESIGNER_PATH}/base.hpp
    ${OV_DESIGNER_PATH}/CBoxConfigurationDialog.*
    ${OV_DESIGNER_PATH}/CLogListenerDesigner.*
    ${OV_DESIGNER_PATH}/CCommentEditorDialog.*
    ${OV_DESIGNER_PATH}/CSettingCollectionHelper.*
)
LIST(APPEND SRC_FILES ${designer_files})

configure_file(${OV_DESIGNER_PATH}/../share/interface.ui-base "${CMAKE_CURRENT_BINARY_DIR}/designer-interface.ui")
configure_file(${OV_DESIGNER_PATH}/../share/interface-settings.ui-base "${CMAKE_CURRENT_BINARY_DIR}/designer-interface-settings.ui")

file(COPY ${CMAKE_CURRENT_BINARY_DIR}/designer-interface.ui DESTINATION ${BUILD_DATADIR}/applications/tracker)
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/designer-interface-settings.ui DESTINATION ${BUILD_DATADIR}/applications/tracker)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/designer-interface.ui" DESTINATION ${DIST_DATADIR}/openvibe/applications/tracker)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/designer-interface-settings.ui" DESTINATION ${DIST_DATADIR}/openvibe/applications/tracker)

set(OV_KERNEL_PATH "${CMAKE_SOURCE_DIR}/sdk/kernel/src/kernel/")
file(GLOB kernel_files
    ${OV_KERNEL_PATH}/scenario/ovkCBoxProto.*
    ${OV_KERNEL_PATH}/scenario/ovkTAttributable.h
    ${OV_KERNEL_PATH}/ovkTKernelObject.h
)
LIST(APPEND SRC_FILES ${kernel_files})

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
                      openvibe
                      openvibe-common
                      openvibe-toolkit
                      openvibe-module-ebml
                      openvibe-module-socket
                      openvibe-module-system
                      openvibe-module-fs
                      openvibe-module-communication
                      openvibe-visualization-toolkit
                      mensia-advanced-visualization-static
                      gtk2
                      Boost::boost
                      Boost::serialization
                      Boost::system
                      tinyxml2::tinyxml2
                      OpenGL::GL
)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})


add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include_directories("include")
include_directories(${OV_AVP_PATH})
include_directories(${OV_DESIGNER_PATH})
include_directories(${OV_KERNEL_PATH})
include_directories(${OV_KERNEL_PATH}/scenario)

if(WIN32)
    # @FIXME CERT getting timeBeginPeriod() linker issues without this
    target_link_libraries(${PROJECT_NAME} winmm)
endif(WIN32)

include("FindThirdPartyRT")  # external stims shared memory needs this

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif(WIN32)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${DIST_BINDIR}
    LIBRARY DESTINATION ${DIST_LIBDIR}
    ARCHIVE DESTINATION ${DIST_LIBDIR})

install_root_alias($<TARGET_FILE_NAME:${PROJECT_NAME}>)

configure_file(share/tracker.ui-base "${CMAKE_CURRENT_BINARY_DIR}/tracker.ui" @ONLY)

file(COPY ${CMAKE_CURRENT_BINARY_DIR}/tracker.ui DESTINATION ${BUILD_DATADIR}/applications/tracker)
file(COPY signals     DESTINATION ${BUILD_DATADIR}/scenarios/)
file(COPY share/      DESTINATION ${BUILD_DATADIR}/applications/tracker PATTERN "*-base" EXCLUDE)
file(COPY workspaces  DESTINATION ${BUILD_DATADIR}/scenarios/)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tracker.ui" DESTINATION ${DIST_DATADIR}/openvibe/applications/tracker)

install(DIRECTORY signals     DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
install(DIRECTORY share/      DESTINATION ${DIST_DATADIR}/openvibe/applications/tracker PATTERN "*-base" EXCLUDE)
install(DIRECTORY workspaces  DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
