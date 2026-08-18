# SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
# SPDX-FileCopyrightText: Inria <romain.tetley@inria.fr>
# SPDX-License-Identifier: LGPL
# From project gnomon: https://gitlab.inria.fr/gnomon/gnomon 

project(OVStyle VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(CMAKE_AUTOMOC ON)

set_source_files_properties(Style.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# qmlcachegen declares an .aotstats output for every compiled file but never writes one for a plain
# .js file, which has no AOT-compilable functions. Ninja then sees a declared output that does not
# exist, so the edge is dirty on every build and relinks OVStyle and everything using it.
set_source_files_properties(style.qml.js PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

qt_add_qml_module(${PROJECT_NAME} STATIC
URI
  ${PROJECT_NAME}
VERSION
  1.0
PLUGIN_TARGET
  ${PROJECT_NAME}
QML_FILES
  Style.qml
  style.qml.js
RESOURCES
  Poppins-Black.ttf
  Poppins-BlackItalic.ttf
  Poppins-Bold.ttf
  Poppins-BoldItalic.ttf
  Poppins-ExtraBold.ttf
  Poppins-ExtraBoldItalic.ttf
  Poppins-ExtraLight.ttf
  Poppins-ExtraLightItalic.ttf
  Poppins-Italic.ttf
  Poppins-Light.ttf
  Poppins-LightItalic.ttf
  Poppins-Medium.ttf
  Poppins-MediumItalic.ttf
  Poppins-Regular.ttf
  Poppins-SemiBold.ttf
  Poppins-SemiBoldItalic.ttf
  Poppins-Thin.ttf
  Poppins-ThinItalic.ttf
)

target_link_libraries(${PROJECT_NAME} 
PUBLIC
  Qt::Gui
PRIVATE 
  Qt::Quick
)

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
