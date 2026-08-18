# ---------------------------------
# Finds Matlab toolkit
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyMatlab)

# Clear cached variables, otherwise repeated builds lead to trouble
SET(Matlab_EXECUTABLE "Matlab_EXECUTABLE-NOTFOUND")
SET(Matlab_INCLUDE "Matlab_INCLUDE-NOTFOUND")
SET(Matlab_ROOT "Matlab_ROOT-NOTFOUND")

# See if we can locate the matlab executable 
IF (WIN32)
	# Unfortunately there doesn't seem to be a neat way to discover both 32 and 64bit matlabs
	IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		SET(MATLAB_PATHS "C:/Program Files/MATLAB/r2015b/bin")
	ELSEIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
		SET(MATLAB_PATHS "C:/Program Files (x86)/MATLAB/r2013b/bin")
	ELSE()
		SET(MATLAB_PATHS "")
	ENDIF()
	
	# Ignore path on the first call, if that fails, try defaults
	FIND_PROGRAM(Matlab_EXECUTABLE MATLAB PATHS ${MATLAB_PATHS} NO_DEFAULT_PATH)
	FIND_PROGRAM(Matlab_EXECUTABLE MATLAB)	
ENDIF(WIN32)
IF (UNIX)
	FIND_PROGRAM(Matlab_EXECUTABLE matlab)
	
	IF(NOT Matlab_EXECUTABLE)
		# Alternative way to try to find matlab
		FILE(GLOB_RECURSE Executable_Candidates1 "/usr/local/matlab*/matlab")
		FILE(GLOB_RECURSE Executable_Candidates2 "/usr/local/MATLAB*/matlab")
		SET(Executable_Candidates ${Executable_Candidates1} ${Executable_Candidates2})
		
		IF(Executable_Candidates) 
			LIST(GET Executable_Candidates 0 Matlab_EXECUTABLE)
		ENDIF()
	ENDIF()
ENDIF(UNIX)

# Figure out the paths to libs and includes
IF(Matlab_EXECUTABLE)
	# OV_PRINT(OV_PRINTED "Have Matlab_EXECUTABLE ${Matlab_EXECUTABLE}")
	# Try relative to the executable path
	GET_FILENAME_COMPONENT(Matlab_ROOT ${Matlab_EXECUTABLE} PATH)
	IF(Matlab_ROOT)
		# OV_PRINT(OV_PRINTED "Have Matlab_ROOT ${Matlab_ROOT}")
		SET(Matlab_ROOT ${Matlab_ROOT}/../)
		# OV_PRINT(OV_PRINTED " -> ${Matlab_ROOT}")	
		FIND_PATH(Matlab_INCLUDE "mex.h" PATHS ${Matlab_ROOT}/extern/include ${Matlab_ROOT}/extern/include/extern)
	ENDIF()

	# matlab executable path might have been pointing to a symbolic link elsewhere, try something else
	IF((NOT Matlab_INCLUDE) AND UNIX)
		EXECUTE_PROCESS(COMMAND matlab -e COMMAND grep "^MATLAB=" COMMAND sed "s/^MATLAB=//g" COMMAND tr "\n" "/" 
				OUTPUT_VARIABLE Matlab_ROOT)
		FIND_PATH(Matlab_INCLUDE "mex.h" PATHS ${Matlab_ROOT}/extern/include ${Matlab_ROOT}/extern/include/extern)
	ENDIF()

	IF(Matlab_INCLUDE)
		# OV_PRINT(OV_PRINTED "Have Matlab_INCLUDE ${Matlab_INCLUDE}")
		IF(UNIX)
			SET(Matlab_LIBRARIES mex mx eng)
			IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
				SET(Matlab_LIB_DIRECTORIES ${Matlab_ROOT}/bin/glnx86)
			ELSE()
				SET(Matlab_LIB_DIRECTORIES ${Matlab_ROOT}/bin/glnxa64)
			ENDIF()
		ENDIF(UNIX)
		IF(WIN32)
			SET(Matlab_LIBRARIES libmex libmx libeng) #mclmcrrt
			IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
				SET(Matlab_LIB_DIRECTORIES ${Matlab_ROOT}/extern/lib/win32/microsoft)
			ELSE()
				SET(Matlab_LIB_DIRECTORIES ${Matlab_ROOT}/extern/lib/win64/microsoft)
			ENDIF()
			# for delayed importation on windows
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} Delayimp )
			SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DELAYLOAD:libeng.dll /DELAYLOAD:libmx.dll")
			# /DELAYLOAD:libmex.dll /DELAYLOAD:mclmcrrt.dll --> useless, no import
		ENDIF(WIN32)
		SET(Matlab_FOUND TRUE)
	ENDIF()

ENDIF(Matlab_EXECUTABLE)

IF(Matlab_FOUND)
	OV_PRINT(OV_PRINTED "  Found Matlab in [${Matlab_ROOT}]")
	SET(Matlab_LIB_FOUND TRUE)
	INCLUDE_DIRECTORIES(${Matlab_INCLUDE})
	
	FOREACH(Matlab_LIB ${Matlab_LIBRARIES})
		SET(Matlab_LIB1 "Matlab_LIB1-NOTFOUND")
		FIND_LIBRARY(Matlab_LIB1 NAMES ${Matlab_LIB} PATHS ${Matlab_LIB_DIRECTORIES} NO_DEFAULT_PATH)
		FIND_LIBRARY(Matlab_LIB1 NAMES ${Matlab_LIB})
		IF(Matlab_LIB1)
			OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${Matlab_LIB1}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${Matlab_LIB1})
		ELSE(Matlab_LIB1)
			OV_PRINT(OV_PRINTED "    [FAILED] Third party lib ${Matlab_LIB}")
			SET(Matlab_LIB_FOUND FALSE)
		ENDIF(Matlab_LIB1)
	ENDFOREACH(Matlab_LIB)
	IF(Matlab_LIB_FOUND)
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyMatlab)
	ELSE(Matlab_LIB_FOUND)
		OV_PRINT(OV_PRINTED "  FAILED to find Matlab Libs, the plugins won't be built. You need a valid MATLAB installation, bitness matching platform target ${PLATFORM_TARGET}.")
	ENDIF(Matlab_LIB_FOUND)
ELSE()
	OV_PRINT(OV_PRINTED "  FAILED to find Matlab (optional) ...")
ENDIF()


SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyMatlab "Yes")

