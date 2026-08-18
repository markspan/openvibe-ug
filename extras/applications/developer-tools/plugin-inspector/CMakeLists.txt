project(openvibe-plugin-inspector VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
# qt_add_executable rather than add_executable so the static QML modules linked below
# get their plugins registered: box snapshots are rendered with the designer's Box.qml.
qt_add_executable(${PROJECT_NAME} ${SRC_FILES})

# OVVisualization lives under designer/, which the root CMakeLists adds after extras/,
# so its target does not exist yet. Linking by name resolves at generate time, but the
# QML plugin import has to be finalized later - see qt_import_qml_plugins() in the root
# CMakeLists.txt, right after add_subdirectory(designer).
# PRIVATE: qt_add_executable() already uses the keyword signature internally, and CMake
# forbids mixing it with the plain one on the same target.
target_link_libraries(${PROJECT_NAME}
					  PRIVATE
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-system
					  Qt6::Gui
					  Qt6::Qml
					  Qt6::Quick
					  OVVisualization
					  DesignSystem
					  OVControls
					  OVStyle
)



if(UNIX)
	set(Qt6_GUI_INCLUDE_DIRS "${Qt6_DIR}/../../../include/qt6/QtGui/${Qt6_VERSION}/QtGui")
elseif (WIN32)
	set(Qt6_GUI_INCLUDE_DIRS "${Qt6_DIR}/../../../include/QtGui/${Qt6_VERSION}/QtGui")
endif()

# For QRhi rendering. We could use GuiPrivate COMPONENT, but it's not available on unix conda-forge qt6 builds.
# Linking with Qt6::Gui is enough, we just need to access the headers.
target_include_directories(${PROJECT_NAME} PRIVATE
	${Qt6_GUI_INCLUDE_DIRS}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

file(COPY share/ DESTINATION ${BUILD_DATADIR}/applications/plugin-inspector)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

# Must come after the install(TARGETS) above: install rules run in declaration order, and the alias
# copies the executable that rule puts in bin/.
install_root_alias($<TARGET_FILE_NAME:${PROJECT_NAME}>)

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/plugin-inspector)
