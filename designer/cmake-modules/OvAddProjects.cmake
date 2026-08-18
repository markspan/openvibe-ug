# Adds the current project to the global properties
FUNCTION(OV_ADD_THIS_TO_PROJECT_LIST)
	# Add the dir to be parsed for documentation later. We need to do this before adding subdir, in case the subdir is the actual docs dir
	GET_PROPERTY(OV_TMP GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS)
	SET(OV_TMP "${OV_TMP};${CMAKE_CURRENT_SOURCE_DIR}")
	SET_PROPERTY(GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS ${OV_TMP})
ENDFUNCTION()

#
# Adds all directories as subdirectories to the CMake build, using the branch specified (if any) in the root CMakeList.txt or
# trunk otherwise.
#
# The branch variable name that is checked is made up from ${CURRENT_BRANCH_PREFIX}_${DIRNAMEUPPER}.
#
# The script also adds the directory to the global list of projects.
#

FUNCTION(OV_ADD_PROJECTS CURRENT_FOLDER_PREFIX)

	FILE(GLOB FILENAMES "*")

	FOREACH(FULLPATH ${FILENAMES})
		IF(IS_DIRECTORY ${FULLPATH} AND EXISTS "${FULLPATH}/CMakeLists.txt")
			GET_FILENAME_COMPONENT(DIRNAME ${FULLPATH} NAME)

			STRING(TOUPPER ${DIRNAME} DIRNAMEUPPER)
			SET(SKIP_THIS_FOLDER "SKIP_${CURRENT_FOLDER_PREFIX}_${DIRNAMEUPPER}")

			debug_message( "Checking SKIP_${CURRENT_FOLDER_PREFIX}_${DIRNAMEUPPER} as branch var ${SKIP_THIS_FOLDER}")

			IF(${SKIP_THIS_FOLDER})
				debug_message( "Note: ${FULLPATH} has been skipped by setting SKIP_${CURRENT_FOLDER_PREFIX}_${DIRNAMEUPPER}")
			ELSE()
				debug_message( "Inserting folder ${FULLPATH}")

				# Add the dir to be parsed for documentation later. We need to do this before adding subdir, in case the subdir is the actual docs dir
				GET_PROPERTY(OV_TMP GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS)
				SET(OV_TMP "${OV_TMP};${FULLPATH}")
				SET_PROPERTY(GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS ${OV_TMP})

				SET(OV_CURRENT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${DIRNAME}")
				GET_PROPERTY(OV_TMP GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS_BUILD_DIR)
				SET(OV_TMP "${OV_TMP};${OV_CURRENT_BINARY_DIR}")
				SET_PROPERTY(GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS_BUILD_DIR ${OV_TMP})

				add_subdirectory(${FULLPATH})

			ENDIF()
		ENDIF()
	ENDFOREACH(FULLPATH ${FILENAMES})

ENDFUNCTION(OV_ADD_PROJECTS)
