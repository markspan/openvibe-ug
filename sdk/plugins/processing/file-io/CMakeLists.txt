project(openvibe-plugins-sdk-file-io VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.c src/*.h src/*.hpp src/*.inl)
list(REMOVE_ITEM SRC_FILES
	src/box-algorithms/csv/ovpCBoxAlgorithmCSVFileReader.cpp
	src/box-algorithms/csv/ovpCBoxAlgorithmCSVFileWriter.cpp
	src/box-algorithms/csv/ovpCBoxAlgorithmCSVFileReader.h
	src/box-algorithms/csv/ovpCBoxAlgorithmCSVFileWriter.h)

include("FindSourceRCProperties")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-csv
                      openvibe-module-ebml
					  openvibe-module-fs
					  XercesC::XercesC
)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${PLUGINS_FOLDER}
					  COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

file(COPY box-tutorials  DESTINATION ${BUILD_DATADIR}/scenarios/)
file(COPY signals DESTINATION ${BUILD_DATADIR}/scenarios/)
file(COPY electrode_sets DESTINATION ${BUILD_DATADIR}/)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY box-tutorials  DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
install(DIRECTORY signals DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
install(DIRECTORY electrode_sets DESTINATION ${DIST_DATADIR}/openvibe/)