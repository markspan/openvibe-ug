project(openvibe-plugins-matlab VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/share/OV_stimulations.m
   DEPENDS openvibe-stimulation-generator ${CMAKE_SOURCE_DIR}/sdk/toolkit/share/stimulation_list.txt
   COMMAND openvibe-stimulation-generator
   --matlab
   ${CMAKE_SOURCE_DIR}/sdk/toolkit/share/stimulation_list.txt
   ${CMAKE_CURRENT_SOURCE_DIR}/share/OV_stimulations.m
   COMMENT "Generating matlab stimulation sources..." )
	
# UG fork: built always (ALL), not only as a step of the plugin: OV_stimulations.m is installed with the
# other MATLAB files also when the plugin is not built, so that copying the DLL is all a PC needs. See UG.md, 2.51.
add_custom_target(generate_matlab_stimulations_file ALL DEPENDS share/OV_stimulations.m)

set_target_properties(generate_matlab_stimulations_file PROPERTIES
	VERSION ${PROJECT_VERSION}
	FOLDER ${MISC_FOLDER}
)

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-system
					  openvibe-module-fs)

add_dependencies(openvibe-plugins-matlab generate_matlab_stimulations_file)

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${PLUGINS_FOLDER}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include("FindThirdPartyMatlab")

# UG fork: without MATLAB at build time the DLL holds no box, and Designer warned at every start ("No 'plugin
# object descriptor' found"). Such a build, the GitHub one too, now leaves the DLL out. See UG.md, 2.51.
if(NOT Matlab_LIB_FOUND)
	set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

file(COPY share/ DESTINATION ${BUILD_DATADIR}/plugins/matlab)
file(COPY box-tutorials DESTINATION ${BUILD_DATADIR}/scenarios/)

# -----------------------------
# Install files
# -----------------------------
if(Matlab_LIB_FOUND) # UG fork: see above
	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
endif()

install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/matlab)

install(DIRECTORY box-tutorials DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
