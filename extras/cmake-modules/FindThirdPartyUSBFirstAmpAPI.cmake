# ---------------------------------
# Finds VAmp FirstAmp library
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyUSBFirstAmpAPI)

IF(WIN32)
	IF("${PLATFORM_TARGET}" STREQUAL "x64")
		FIND_PATH(PATH_USBFirstAmpAPI FirstAmp.h PATHS "C:/Program Files/FaSDK-x64")	
	ELSE()
		FIND_PATH(PATH_USBFirstAmpAPI FirstAmp.h PATHS "C:/Program Files/FaSDK" "C:/Program Files (x86)/FaSDK")	
	ENDIF()
	
	IF(PATH_USBFirstAmpAPI)
		OV_PRINT(OV_PRINTED "  Found Brain Products FirstAmp (VAmp) API...")
		INCLUDE_DIRECTORIES(${PATH_USBFirstAmpAPI})
		FIND_LIBRARY(LIB_USBFirstAmpAPI FirstAmp PATHS ${PATH_USBFirstAmpAPI} )
		IF(LIB_USBFirstAmpAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_USBFirstAmpAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_USBFirstAmpAPI} )
		ELSE()
			OV_PRINT(OV_PRINTED "    [FAILED] lib FirstAmp")
		ENDIF()

		# Copy the DLL file at build
		file(COPY "${PATH_USBFirstAmpAPI}/FirstAmp.dll" DESTINATION ${CMAKE_BINARY_DIR}/bin)
		# Copy the DLL file at install
		INSTALL(PROGRAMS "${PATH_USBFirstAmpAPI}/FirstAmp.dll" DESTINATION ${DIST_BINDIR})
		
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyUSBFirstAmpAPI)
	ELSE()
		OV_PRINT(OV_PRINTED "  FAILED to find Brain Products FirstAmp (VAmp) API (optional driver)")
	ENDIF()
ENDIF()

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyUSBFirstAmpAPI "Yes")

