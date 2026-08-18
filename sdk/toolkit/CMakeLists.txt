project(openvibe-toolkit VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

# Command to generate cpp compliant stimulation files from text file
# Creating output directories is mandatory as it is not handled by ADD_CUSTOM_COMMAND
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/toolkit)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/toolkit/ovtk_stimulations.h ${CMAKE_CURRENT_BINARY_DIR}/src/ovtk_stimulations.cpp
	DEPENDS openvibe-stimulation-generator ${CMAKE_CURRENT_SOURCE_DIR}/share/stimulation_list.txt
	COMMAND openvibe-stimulation-generator
	--cpp
	${CMAKE_CURRENT_SOURCE_DIR}/share/stimulation_list.txt
	${CMAKE_CURRENT_BINARY_DIR}/include/toolkit/ovtk_stimulations.h
	${CMAKE_CURRENT_BINARY_DIR}/src/ovtk_stimulations.cpp
	COMMENT "Generating stimulation sources..." )

# ADD_CUSTOM_TARGET will automatically trigger the command defined above
# as it depends on its outputs
add_custom_target(openvibe-generate-stimulation-file
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/toolkit/ovtk_stimulations.h ${CMAKE_CURRENT_BINARY_DIR}/src/ovtk_stimulations.cpp)

set_target_properties(openvibe-generate-stimulation-file PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  FOLDER ${MISC_FOLDER}
)

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.inl src/*.hpp include/*.h include/*.hpp)
include("FindSourceRCProperties")

add_library(${PROJECT_NAME} SHARED
			${SRC_FILES}
			${CMAKE_CURRENT_BINARY_DIR}/include/toolkit/ovtk_stimulations.h
			${CMAKE_CURRENT_BINARY_DIR}/src/ovtk_stimulations.cpp
)

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-module-xml
					  openvibe-module-fs
					  Boost::filesystem)

add_dependencies(${PROJECT_NAME} openvibe-generate-stimulation-file)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${KERNEL_FOLDER}
					  COMPILE_FLAGS "-DOVTK_Exports -DOVTK_Shared")

# This long list could be removed if header files where not using relative path includes
target_include_directories(${PROJECT_NAME} 
						   PRIVATE 
						   ${CMAKE_CURRENT_SOURCE_DIR}/include
						   ${CMAKE_CURRENT_SOURCE_DIR}/include/toolkit/algorithms/scenario-io
						   ${CMAKE_CURRENT_SOURCE_DIR}/include/toolkit/algorithms/classification
						   ${CMAKE_CURRENT_SOURCE_DIR}/include/toolkit/deprecated/writer
						   ${CMAKE_CURRENT_SOURCE_DIR}/include/oolkit/deprecated/reader
						   ${CMAKE_CURRENT_SOURCE_DIR}/include/toolkit/training
						   ${CMAKE_CURRENT_SOURCE_DIR}/include/toolkit/tools
						   PUBLIC
						   include
						   ${CMAKE_CURRENT_BINARY_DIR}/include
)

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${CMAKE_CURRENT_BINARY_DIR}/include/   DESTINATION  ${CMAKE_BINARY_DIR}/include FILES_MATCHING PATTERN "*.h")
file(COPY share/ DESTINATION ${BUILD_DATADIR}/toolkit PATTERN "*-base" EXCLUDE)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${CMAKE_CURRENT_BINARY_DIR}/include/   DESTINATION  ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/toolkit PATTERN "*-base" EXCLUDE)

