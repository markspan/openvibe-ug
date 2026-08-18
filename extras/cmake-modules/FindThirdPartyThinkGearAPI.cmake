# ---------------------------------
# Finds Neurosky ThinkGear library
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyThinkGearAPI)

IF(WIN32)	

	IF("${PLATFORM_TARGET}" STREQUAL "x64")
		SET(THINKGEAR_LIBDIR "x64")
		SET(THINKGEAR_LIB "thinkgear64")		
	ELSE()
		SET(THINKGEAR_LIBDIR "win32")
		SET(THINKGEAR_LIB "thinkgear")		
	ENDIF()
	
	FIND_PATH(PATH_ThinkGearAPI thinkgear.h PATHS 
		"C:/Program Files/MindSet Windows Developer Tools 3.2/Stream SDK for PC/libs/${THINKGEAR_LIBDIR}/" 
		"C:/Program Files (x86)/MindSet Windows Developer Tools 3.2/Stream SDK for PC/libs/${THINKGEAR_LIBDIR}/"
		NO_DEFAULT_PATH)
		
	IF(PATH_ThinkGearAPI)
		OV_PRINT(OV_PRINTED "  Found ThinkGear API...")
		INCLUDE_DIRECTORIES(${PATH_ThinkGearAPI})
		FIND_LIBRARY(LIB_ThinkGearAPI ${THINKGEAR_LIB} PATHS ${PATH_ThinkGearAPI} )
		IF(LIB_ThinkGearAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_ThinkGearAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_ThinkGearAPI} )
		ELSE(LIB_ThinkGearAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] lib thinkgear")
		ENDIF(LIB_ThinkGearAPI)

		# Copy the DLL file at build
		file(COPY "${PATH_ThinkGearAPI}/${THINKGEAR_LIB}.dll" DESTINATION ${CMAKE_BINARY_DIR}/bin)
		# Copy the DLL file at install
		INSTALL(PROGRAMS "${PATH_ThinkGearAPI}/${THINKGEAR_LIB}.dll" DESTINATION ${DIST_BINDIR})

		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyThinkGearAPI)
	ELSE(PATH_ThinkGearAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find a valid ThinkGear API for NeuroSky Mindsets (optional driver)")
	ENDIF(PATH_ThinkGearAPI)
ENDIF(WIN32)

IF(UNIX)
	OV_PRINT(OV_PRINTED "  Skipped ThinkGear API for Neurosky MindSet, only available on windows.")
ENDIF(UNIX)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyThinkGearAPI "Yes")

