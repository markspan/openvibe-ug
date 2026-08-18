# ---------------------------------
# Finds BioSemi LabView API
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyBioSemiLabViewAPI)

IF(WIN32)	
	# note: care must be taken here that lib of wrong bitness is not picked
	IF("${PLATFORM_TARGET}" STREQUAL "x64")
		SET(PATH_BioSemiAPI "C:/Program Files/BioSemi/Developers_kit/C-code")
	ELSE()
		SET(PATH_BioSemiAPI "C:/Program Files (x86)/BioSemi/Developers_kit/C-code")
	ENDIF()
	
	FIND_FILE(LABVIEW_HEADER labview_dll.h
		PATHS ${PATH_BioSemiAPI}
		NO_SYSTEM_ENVIRONMENT_PATH
		NO_DEFAULT_PATH)
	
	IF(LABVIEW_HEADER)
		OV_PRINT(OV_PRINTED "  Found BioSemi LabView API...")
		INCLUDE_DIRECTORIES(${PATH_BioSemiAPI})
		FIND_LIBRARY(LIB_BioSemiAPI Labview_DLL PATHS ${PATH_BioSemiAPI} NO_SYSTEM_ENVIRONMENT_PATH NO_DEFAULT_PATH)
		IF(LIB_BioSemiAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_BioSemiAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_BioSemiAPI} )
		ELSE(LIB_BioSemiAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] lib Labview_DLL")
		ENDIF(LIB_BioSemiAPI)

		# Copy the DLL file at build
		file(COPY "${PATH_BioSemiAPI}/Labview_DLL.dll" DESTINATION ${CMAKE_BINARY_DIR}/bin)
		# Copy the DLL file at install
		INSTALL(PROGRAMS "${PATH_BioSemiAPI}/Labview_DLL.dll" DESTINATION ${DIST_BINDIR})

		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyBioSemiAPI)
	ELSE()
		OV_PRINT(OV_PRINTED "  FAILED to find BioSemi LabView API (optional driver)")
	ENDIF()
ENDIF(WIN32)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyBioSemiLabViewAPI "Yes")

