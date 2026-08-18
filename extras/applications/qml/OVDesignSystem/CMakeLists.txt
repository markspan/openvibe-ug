# SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

project(DesignSystem VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB icons RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/*.svg)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${PROJECT_NAME} STATIC
    URI ${PROJECT_NAME}
    VERSION 1.0
    PLUGIN_TARGET ${PROJECT_NAME}
    SOURCES
        src/FontManager.cpp
        src/FontManager.h
    QML_FILES
        qml/Button.qml
        qml/CheckBox.qml
        qml/ComboBox.qml
        qml/HorizontalSpacer.qml
        qml/Icon.qml
        qml/IconButton.qml
        qml/Popup.qml
        qml/SpinButton.qml
        qml/Switch.qml
        qml/TextField.qml
        qml/ToolTip.qml
        qml/VerticalSpacer.qml
        Theme.qml
    RESOURCES
        # icons
        ${icons}
        # fonts
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-Black.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-BlackItalic.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-Bold.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-BoldItalic.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-ExtraBold.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-ExtraBoldItalic.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-ExtraLight.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-ExtraLightItalic.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-Italic.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-Light.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-LightItalic.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-Medium.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-MediumItalic.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-Regular.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-SemiBold.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-SemiBoldItalic.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-Thin.ttf
        resources/fonts/Be_Vietnam_Pro/BeVietnamPro-ThinItalic.ttf
        resources/fonts/Space_Grotesk/static/SpaceGrotesk-Bold.ttf
        resources/fonts/Space_Grotesk/static/SpaceGrotesk-Light.ttf
        resources/fonts/Space_Grotesk/static/SpaceGrotesk-Medium.ttf
        resources/fonts/Space_Grotesk/static/SpaceGrotesk-Regular.ttf
        resources/fonts/Space_Grotesk/static/SpaceGrotesk-SemiBold.ttf
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
)

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
