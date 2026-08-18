PROJECT(openvibe-scenario-update-2to3 VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(CMAKE_AUTOMOC ON)

qt_add_executable(${PROJECT_NAME}
	src/main.cpp
	src/updateHandler.cpp
	src/updateHandler.h
)

qt_add_qml_module(${PROJECT_NAME}
URI
  ${PROJECT_NAME}
VERSION
  ${PROJECT_VERSION}
QML_FILES
  qml/Main.qml)

target_link_libraries(${PROJECT_NAME}
					  PRIVATE
                      tinyxml2::tinyxml2
					  Qt6::Quick
					  Qt6::QuickControls2
)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})