#######################################################################
# Software License Agreement (AGPL-3 License)
# 
# OpenViBE SDK Test Software
# Based on OpenViBE V1.1.0, Copyright (C) Inria, 2006-2015
# Copyright (C) Inria, 2015-2017,V1.0
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License version 3,
# as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.
# If not, see <http://www.gnu.org/licenses/>.
#######################################################################

# Cmake script used to wrap multiple commands into one.
#
# Mandatory Input parameters: 
# -UNQUOTE: set to 1 to unquote commands before processing, 0 otherwise
# -CMD1 First commands to execute
#
# Optional Input parameters:
# -CMD2 to CMD5: Other commands to execute
#
# Example usage:
#	ADD_TEST(NAME ${TEST_NAME} 
#	COMMAND ${CMAKE_COMMAND}
#   -DUNQUOTE=1
#	-DCMD1="Command_1"
#   -DCMD2="Command_2"
#	-DCMD3="Command_3"
#	-P ${OpenViBE_TEST_CMAKE_DIR}/OvtRunMultipleCommand3.cmake)

# Macro used to execute a command and check the result
MACRO(EXEC_CHECK RAW_CMD UNQUOTE_CMD)
	
	SET(CMD_ARGS_RAW ${RAW_CMD})

	IF(${UNQUOTE_CMD})
		# First processing to remove enclosing quotes
		STRING(LENGTH "${RAW_CMD}" CMD_STRING_LENGTH)
		MATH(EXPR CMD_STRING_LENGTH "${CMD_STRING_LENGTH} - 2")
		STRING(SUBSTRING "${RAW_CMD}" 1 ${CMD_STRING_LENGTH} CMD_ARGS_RAW)
	ENDIF()
	
	# Process arguments to build an arg list
	SEPARATE_ARGUMENTS(CMD_ARGS WINDOWS_COMMAND ${CMD_ARGS_RAW})
	
	EXECUTE_PROCESS(COMMAND ${CMD_ARGS} RESULT_VARIABLE ret_var OUTPUT_VARIABLE output_var)
	
	MESSAGE(STATUS "----------------------------------------------\n")
	MESSAGE(STATUS "-----------NEW COMMAND EXECUTED-----------\n\n") 
	MESSAGE(STATUS "----------------------------------------------\n\n")
  
	MESSAGE(STATUS "-----------Command Name & Arguments-----------\n${RAW_CMD}\n\n")
	MESSAGE(STATUS "-----------Command Output-----------\n${output_var}\n\n")
  
	IF(ret_var)
		MESSAGE(FATAL_ERROR "-----------Error-----------\n$Running command failed with code: ${ret_var}\n\n")
	ENDIF()

ENDMACRO()

IF(NOT DEFINED UNQUOTE)
	MESSAGE(FATAL_ERROR "Missing UNQUOTE parameter")
ENDIF()

IF(DEFINED CMD1)
	EXEC_CHECK(${CMD1} ${UNQUOTE})
ELSE()
	MESSAGE(FATAL_ERROR "At least one command needed in OvtRunMultipleCommand script")
ENDIF()

IF(DEFINED CMD2)
	EXEC_CHECK(${CMD2} ${UNQUOTE})
ENDIF()

IF(DEFINED CMD3)
	EXEC_CHECK(${CMD3} ${UNQUOTE})
ENDIF()

IF(DEFINED CMD4)
	EXEC_CHECK(${CMD4} ${UNQUOTE})
ENDIF()

IF(DEFINED CMD5)
	EXEC_CHECK(${CMD5} ${UNQUOTE})
ENDIF()
