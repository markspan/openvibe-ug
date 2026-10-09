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
	# x64 is the only supported architecture.
	add_definitions(-DTARGET_ARCHITECTURE_x64)

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
