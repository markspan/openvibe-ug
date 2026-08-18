project(openvibe-visualization-toolkit VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

OV_ADD_THIS_TO_PROJECT_LIST()

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp include/*.h include/*.hpp)
include("FindSourceRCProperties")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
include_directories(${GTK2_INCLUDE_DIRS})

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(${PROJECT_NAME}
						   PRIVATE include/visualization-toolkit
						   PUBLIC include
)

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-toolkit
)

target_compile_options(${PROJECT_NAME}
					   PRIVATE -DOVVIZ_Exports
					   PUBLIC -DOVVIZ_Shared)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  FOLDER ${MODULES_FOLDER})

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)


# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY include/   DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
