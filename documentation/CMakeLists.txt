message(STATUS "***** Begin META Documentation *****")

# Documentation is now a build-time dependency graph:
#
#   plugins + plugin-inspector  ->  box snapshots and .dox-skeleton files
#                               ->  skeletons expanded with the hand-written .dox-part content
#                               ->  doxygen
#
# The "doc" target is off ALL, in order not to build it each time.
# Therefore, build target by name (e.g. `ninja doc`)

# look for doxygen, if not present, no need to generate documentation
find_program(doxygen_bin "doxygen" PATHS $ENV{CONDA_PREFIX}/bin $ENV{CONDA_PREFIX}/Library/bin NO_DEFAULT_PATH)

if(NOT doxygen_bin)
	message(STATUS "  FAILED to find doxygen, the 'doc' target will not be available")
	return()
endif()

message(STATUS "  Found doxygen...")
set(ROOT_DOC_DIR "${CMAKE_BINARY_DIR}/documentation")
message(STATUS "    Root Folder for Documentation : ${ROOT_DOC_DIR}")

file(MAKE_DIRECTORY "${ROOT_DOC_DIR}")

configure_file("Doxyfile-skeleton" "${ROOT_DOC_DIR}/Doxyfile" @ONLY)
configure_file("Main-skeleton.md" "${ROOT_DOC_DIR}/Main.md" @ONLY)

# ----- Run the plugin inspector against the build tree -----
set(INSPECTOR_STAMP "${ROOT_DOC_DIR}/plugin-inspector.stamp")
add_custom_command(
	OUTPUT ${INSPECTOR_STAMP}
	# Clean doc output first.
	COMMAND ${CMAKE_COMMAND} -E rm -rf "${ROOT_DOC_DIR}/box-algorithm-doc" "${ROOT_DOC_DIR}/box-algorithm-snapshots" "${ROOT_DOC_DIR}/dox"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${ROOT_DOC_DIR}/box-algorithm-doc" "${ROOT_DOC_DIR}/box-algorithm-snapshots" "${ROOT_DOC_DIR}/dox"
	COMMAND ${CMAKE_COMMAND} -E env
			"OV_PATH_ROOT=${CMAKE_BINARY_DIR}"
			$<TARGET_FILE:openvibe-plugin-inspector>
				-d "${ROOT_DOC_DIR}"
				-p $<TARGET_FILE_DIR:openvibe-kernel>
				-k $<TARGET_FILE_DIR:openvibe-kernel>
				-c "${BUILD_DATADIR}/kernel/openvibe.conf"
	COMMAND ${CMAKE_COMMAND} -E touch ${INSPECTOR_STAMP}
	# Some plugins require python runtime, so let's depend on it too.
	DEPENDS openvibe-plugin-inspector ${OV_PLUGIN_TARGETS} ${PBS_DEPLOY_STAMP}
	# Needed to resolve dependencies DLLs
	WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
	COMMENT "Generating box snapshots and documentation skeletons"
	VERBATIM
)

# ----- Expand the hand-written documentation into the skeletons -----
# Globbed here only to retrigger the command below; ExpandBoxDoc.cmake globs them again itself.
file(GLOB_RECURSE DOX_PART_FILES CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/sdk/*.dox-part"
	"${CMAKE_SOURCE_DIR}/extras/*.dox-part"
	"${CMAKE_SOURCE_DIR}/designer/*.dox-part")

set(DOX_STAMP "${ROOT_DOC_DIR}/dox.stamp")
add_custom_command(
	OUTPUT ${DOX_STAMP}
	COMMAND ${CMAKE_COMMAND}
			-DROOT_DOC_DIR=${ROOT_DOC_DIR}
			-DOV_SOURCE_DIR=${CMAKE_SOURCE_DIR}
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ExpandBoxDoc.cmake"
	COMMAND ${CMAKE_COMMAND} -E touch ${DOX_STAMP}
	DEPENDS ${INSPECTOR_STAMP} ${DOX_PART_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ExpandBoxDoc.cmake"
	COMMENT "Expanding box documentation"
	VERBATIM
)

# ----- Run doxygen -----
# The working directory stays the source documentation folder: doxygen resolves the relative INPUT
# and IMAGE_PATH entries of the Doxyfile against its own working directory, not against the
# location of the configuration file.
add_custom_command(
	OUTPUT "${ROOT_DOC_DIR}/html/index.html"
	COMMAND ${doxygen_bin} "${ROOT_DOC_DIR}/Doxyfile"
	DEPENDS ${DOX_STAMP} "${ROOT_DOC_DIR}/Doxyfile" "${ROOT_DOC_DIR}/Main.md"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	COMMENT "Running doxygen"
	VERBATIM
)

add_custom_target(doc DEPENDS "${ROOT_DOC_DIR}/html/index.html")
set_property(TARGET doc PROPERTY FOLDER ${MISC_FOLDER})

# Ship the generated HTML in the package. OPTIONAL: the 'doc' target is off ALL, so a plain install or
# cpack on a tree where the documentation was never built must still succeed (the designer then reports
# that no local documentation is available on F1). Installed under the same relative name as in the
# build tree, so that designer.conf can point at ${Path_Bin}/../documentation/html in both layouts.
install(DIRECTORY "${ROOT_DOC_DIR}/html" DESTINATION "${DIST_ROOT}/documentation" OPTIONAL)
