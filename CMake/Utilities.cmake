# SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

function(debug_message)
	if(${Flag_VerboseOutput})
		message(STATUS "${ARGV}")
	endif()
endfunction()

# Usage: install_root_alias(<file name>)
#        install_root_alias(<link name> <target file name>)
#
# Creates an alias of bin/<target file name> at the root of the install tree.
#
# Windows gets a copy rather than a symlink. "cmake -E create_symlink" needs Developer Mode or
# elevation there.
function(install_root_alias link_name)
	if(ARGC GREATER 1)
		set(target_file_name "${ARGV1}")
	else()
		set(target_file_name "${link_name}")
	endif()

	# COMMAND_ERROR_IS_FATAL turns a failure into a failed install rather than a package that is
	# quietly missing its aliases. Install rules run in declaration order, so call this *after* the
	# install(TARGETS) that puts the executable in bin/.
	if(WIN32)
		install(CODE
			"execute_process( \
				COMMAND ${CMAKE_COMMAND} -E copy_if_different \
				\${CMAKE_INSTALL_PREFIX}/${DIST_BINDIR}/${target_file_name} \
				\${CMAKE_INSTALL_PREFIX}/${link_name} \
				COMMAND_ERROR_IS_FATAL ANY \
			)"
		)
	else()
		install(CODE
			"execute_process( \
				COMMAND ${CMAKE_COMMAND} -E create_symlink \
				${DIST_BINDIR}/${target_file_name} \
				\${CMAKE_INSTALL_PREFIX}/${link_name} \
				COMMAND_ERROR_IS_FATAL ANY \
			)"
		)
	endif()
endfunction()

# set(OV_COMPILE_TESTS "true")
function(SET_BUILD_PLATFORM)
	if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		add_definitions(-DTARGET_ARCHITECTURE_x64)
	elseif("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
		add_definitions(-DTARGET_ARCHITECTURE_i386)
	else()
		add_definitions(-DTARGET_ARCHITECTURE_Unknown)
	endif()

	if(WIN32)
		add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
		add_definitions(-DTARGET_OS_Windows)
		add_definitions(-DTARGET_COMPILER_VisualStudio)
	elseif(APPLE)
		add_definitions(-fnon-call-exceptions)
		add_definitions(-DTARGET_OS_MacOS)
		# add_definitions(-DTARGET_ARCHITECTURE_x64)
		add_definitions(-DTARGET_COMPILER_LLVM)
	elseif(UNIX)
		# add_definitions(-fvisibility=hidden) # This flag should be present... man gcc
		add_definitions(-fnon-call-exceptions)
		add_definitions(-DTARGET_OS_Linux)
		add_definitions(-DTARGET_COMPILER_GCC)
	endif()

endfunction()

# Set version based on git tag.
#  if current commit is tagged, use the tag as it is, and add build number based on content of .build file, written by Jenkins
#  Else use last tag major and minor number and set patch number to 99
#
# This function should remain generic to be usable in every projects.
function(set_version)
	find_package(Git)
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.version)

		# These versions are used by the subprojects by default.
		# if you wish to maintain specific version numbers for a subproject, please do so in the projects CMakeLists.txt
		file(READ ${CMAKE_CURRENT_SOURCE_DIR}/.version PROJECT_VERSION)
		string(STRIP ${PROJECT_VERSION} PROJECT_VERSION)
		string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
		list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
		list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
		list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)

	elseif(NOT GIT_FOUND)
		message(WARNING "Git not found, set version to 0.0.0")
		set(PROJECT_VERSION "0.0.0")
		set(PROJECT_BRANCH_STRING "unknown")
		set(PROJECT_COMMITHASH_STRING "0")
	else()
		debug_message("Found Git: ${GIT_EXECUTABLE}")
		execute_process(COMMAND ${GIT_EXECUTABLE} describe
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
			OUTPUT_VARIABLE  PROJECT_VERSION
			ERROR_VARIABLE  ERROR)
		if(ERROR)
			message(WARNING "No tags found, set version to 0.0.0")
			set(PROJECT_VERSION "0.0.0")
			set(PROJECT_BRANCH_STRING "unknown")
			set(PROJECT_COMMITHASH_STRING "0")
		else()
			# codename = the name of the current branch
			execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				OUTPUT_VARIABLE  PROJECT_BRANCH_STRING)
			# command output may contain carriage return
			string(STRIP ${PROJECT_BRANCH_STRING} PROJECT_BRANCH_STRING)

			# commithash = short hash of latest revision
			execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				OUTPUT_VARIABLE  PROJECT_COMMITHASH_STRING)
			# command output may contain carriage return
			string(STRIP ${PROJECT_COMMITHASH_STRING} PROJECT_COMMITHASH_STRING)
		endif()
	endif()

	# if current commit is not tagged result is formed as: "major.minor.patch-number of commits since last tag-hash"
	string(STRIP ${PROJECT_VERSION} PROJECT_VERSION)
	string(REPLACE "-" ";" version_list ${PROJECT_VERSION})
	list(LENGTH version_list version_list_length)
	if(${version_list_length} EQUAL 3) # if result is formed as "major.minor.patch-number of commits since last tag-hash" set patch as 99
		list(GET version_list 0 PROJECT_VERSION)
		set(PROJECT_VERSION_PATCH 99)
	endif()

	string(REPLACE "." ";" version_list ${PROJECT_VERSION})
	list(GET version_list 0 PROJECT_VERSION_MAJOR)
	list(GET version_list 1 PROJECT_VERSION_MINOR)
	if(NOT PROJECT_VERSION_PATCH)
		list(GET version_list 2 PROJECT_VERSION_PATCH)
	endif()

	# These versions are used by the subprojects by default.
	# if you wish to maintain specific version numbers for a subproject, please do so in the projects CMakeLists.txt
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.build)
		file(READ ${CMAKE_CURRENT_SOURCE_DIR}/.build PROJECT_VERSION_BUILD)
		string(STRIP ${PROJECT_VERSION_BUILD} PROJECT_VERSION_BUILD)
	else()
		set(PROJECT_VERSION_BUILD 0)
	endif()
	
	set(PROJECT_BRANCH ${PROJECT_BRANCH_STRING} PARENT_SCOPE)
	set(PROJECT_COMMITHASH ${PROJECT_COMMITHASH_STRING} PARENT_SCOPE)

	set(PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} PARENT_SCOPE)
	set(PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR} PARENT_SCOPE)
	set(PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH} PARENT_SCOPE)
	set(PROJECT_VERSION_BUILD ${PROJECT_VERSION_BUILD} PARENT_SCOPE)
	set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} PARENT_SCOPE)
endfunction()

