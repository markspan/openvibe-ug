# SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

###############################################################################
## CMake global configuration
###############################################################################

# Minimal version requied of CMake
cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project(openvibe-acquisition-server VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

## ############################################################################
## Define target name
## ############################################################################
set(TARGET_NAME ${PROJECT_NAME})

## ############################################################################
## The driver/writer abstraction layer is a shared library of its own, so that
## loadable driver and writer plugins can link against it.
## ############################################################################
add_subdirectory(abstractions)

## ############################################################################
## Define sources, headers, and directories
## ############################################################################
list(APPEND APP_SOURCES
    # Core
    core/AcquisitionManager.cpp
    core/Acquisition.cpp
    core/Api.cpp
    core/CircularBuffer.cpp
    core/DriftCalculator.cpp
    core/SampleBufferHandler.cpp
    core/Thread.cpp
	core/net-api/CNetAcqControlAPI.cpp

    # Built-in extensions
    builtin/drivers/stubs/DummyDriver.cpp
    builtin/drivers/generic-oscillator/ovasCDriverGenericOscillator.cpp
    builtin/drivers/tagging/TaggingDriver.cpp
    builtin/drivers/labstreaminglayer/CDriverLabStreamingLayer.cpp
    builtin/writers/net-writer/NetWriter.cpp
    builtin/writers/lsl-output/LSLOutputWriter.cpp
    
    # Settings
    settings/JsonSettings.cpp
    settings/SessionManager.cpp

    #main
    ApplicationPathHandling.cpp
    CInitApp.cpp
    main.cpp
)

list(APPEND APP_PRIVATE_HEADERS
    # Core
    core/AcquisitionManager.h
    core/Acquisition.h
    core/Api.h
    core/CircularBuffer.h
    core/DriftCalculator.h
    core/SampleBufferHandler.h
    core/Thread.h
    #core/Bridges/CBridgesCollection.h
	core/net-api/CNetAcqControlAPI.h

    # Built-in extensions
    builtin/drivers/generic-oscillator/ovasCDriverGenericOscillator.h
    builtin/drivers/stubs/DummyDriver.h
    builtin/drivers/tagging/TaggingDriver.h
    builtin/drivers/labstreaminglayer/CDriverLabStreamingLayer.h
    builtin/writers/net-writer/NetWriter.h
    builtin/writers/lsl-output/LSLOutputWriter.h
    
    # Settings    
    settings/ISettingsProvider.h
    settings/JsonSettings.h
    settings/SessionManager.h
    
    # main
    CInitApp.h
    ApplicationPathHandling.h
    LightArgs.hpp
)

## ############################################################################
## buitin drivers present conditionally (depending on OS and SDK availability)
## ############################################################################
if(WIN32 AND BIOSEMI_FOUND)
    list(APPEND APP_SOURCES
        builtin/drivers/biosemi/activetwo/CBridgeBioSemiActiveTwo.cpp
        builtin/drivers/biosemi/activetwo/CDriverBioSemiActiveTwo.cpp
        builtin/drivers/biosemi/activethree/CBridgeBioSemiActiveThree.cpp
        builtin/drivers/biosemi/activethree/CDriverBioSemiActiveThree.cpp
    )
    list(APPEND APP_PRIVATE_HEADERS
        builtin/drivers/biosemi/CommonBioSemi.h
        builtin/drivers/biosemi/activetwo/CBridgeBioSemiActiveTwo.h
        builtin/drivers/biosemi/activetwo/CDriverBioSemiActiveTwo.h
        builtin/drivers/biosemi/activethree/CBridgeBioSemiActiveThree.h
        builtin/drivers/biosemi/activethree/CDriverBioSemiActiveThree.h
    )
endif()

# UG fork: the TMSi driver, which upstream 4.0 does not have.
# TMSiSDK.dll is loaded at runtime, so the driver needs no SDK at build time
if(WIN32)
    list(APPEND APP_SOURCES
        builtin/drivers/tmsi/CDriverTMSi.cpp
    )
    list(APPEND APP_PRIVATE_HEADERS
        builtin/drivers/tmsi/CDriverTMSi.h
    )
endif()

extract_relative_directories( APP_PRIVATE_HEADERS   APP_PRIVATE_INCLUDE_DIRECTORIES)

## ############################################################################
## Create and configure the target
## ############################################################################

if(WIN32)
    # ov_configure_rc falls back to the target name for the product name, which is what Explorer and
    # Add/Remove Programs display. The generated .rc embeds the icon into the executable, which is
    # also where the NSIS Start-Menu shortcut picks it up from.
    set(PROJECT_PRODUCT_NAME "OpenViBE Acquisition Server")
    ov_configure_rc(NAME ${TARGET_NAME} ICON_PATH ${OV_ICON_FILE})
    set(OV_ACQUISITION_SERVER_RC "${CMAKE_BINARY_DIR}/resource-files/${TARGET_NAME}.rc")
endif()

qt_add_executable(${TARGET_NAME})
if(WIN32)
    # UG fork: no console window; main() sends what the server prints to a log file (openvibe/ConsoleLog.hpp)
    set_target_properties(${TARGET_NAME} PROPERTIES WIN32_EXECUTABLE ON)
endif()
target_sources(${TARGET_NAME} PRIVATE ${APP_SOURCES} ${APP_PRIVATE_HEADERS} ${OV_ACQUISITION_SERVER_RC})
setup_output_paths_general(${TARGET_NAME})
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${APP_PRIVATE_INCLUDE_DIRECTORIES}
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

target_compile_definitions(${TARGET_NAME} PRIVATE -DWIN32_LEAN_AND_MEAN)

# Keep main.cpp on ":/qt/qml/AcquisitionServer/ov-icon.ico" and not worrying about the real file path
set_source_files_properties(${OV_ICON_FILE} PROPERTIES QT_RESOURCE_ALIAS ov-icon.ico)

qt_add_qml_module(${TARGET_NAME}
    URI
        AcquisitionServer
    VERSION
        ${PROJECT_VERSION}
    QML_FILES
        ui/qml/ActionBar.qml
        ui/qml/Main.qml
        ui/qml/Impedances.qml
        ui/qml/ChannelRename.qml
        ui/qml/DriverNote.qml     # UG fork
        ui/qml/LevelBar.qml       # UG fork: with BufferControl, in place of DriftBar and DriftControl
        ui/qml/BufferControl.qml  # UG fork
    SOURCES
        ui/AcquisitionController.h
        ui/AcquisitionController.cpp
        ui/Charmeleon.h           # UG fork
        ui/Charmeleon.cpp         # UG fork
        ui/ImpedanceItem.h
    RESOURCES
        ${OV_ICON_FILE}
)


target_link_libraries(${TARGET_NAME} PRIVATE
    NetService
    TcpTaggingService
    openvibe-acquisition-abstractions
    openvibe
    openvibe-toolkit
    openvibe-module-system
    openvibe-module-system-delayload
    openvibe-parameters-object
    OVParameters
    DesignSystem
    OVControls
    nlohmann_json::nlohmann_json
    Qt::Gui
    Qt::Quick
    Qt6::QuickControls2
    LSL::lsl
)

if(WIN32 AND BIOSEMI_FOUND)
    target_link_libraries(${TARGET_NAME} PRIVATE sdk-biosemi)
endif()

create_filters(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME} PUBLIC -D_USE_MATH_DEFINES)

if(MSVC)
    target_link_options(${TARGET_NAME} PRIVATE "/DELAYLOAD:NetService.dll" "/DELAYLOAD:TcpTaggingService.dll")
endif()

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        BUILTIN_PLUGIN
)

if(OV_COMPILE_TESTS)
    add_subdirectory(test)
endif(OV_COMPILE_TESTS)
