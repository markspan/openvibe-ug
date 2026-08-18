#
# The gMobilab driver (Linux) was contributed by Lucie Daubigney from Supelec Metz
#
# Windows-compatibility added by Jussi T. Lindgren / Inria
#

# ---------------------------------
# Finds GTecMobiLabPlus+
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyGMobiLabPlusAPI)

IF(WIN32)
	# note that the API bitness must match the OV build bitness
	FIND_PATH(PATH_GMobiLabCAPI GMobiLabPlus.h PATHS 
		"C:/Program Files/gtec/GMobiLabCAPI/Lib"
		"C:/Program Files (x86)/gtec/GMobiLabCAPI/Lib")
	IF("${PLATFORM_TARGET}" STREQUAL "x64")		
		# We need to copy the DLL on install; note that System32 *is* the 64bit folder on 64bit arch
		FIND_PATH(PATH_GMobiLabDLL gMOBIlabplus.dll PATHS 
			"C:/Windows/System32")		
		FIND_LIBRARY(LIB_GMobiLabCAPI GMobiLabplus PATHS ${PATH_GMobiLabCAPI}/x64)	
	ELSE()
		# We need to copy the DLL on install
		FIND_PATH(PATH_GMobiLabDLL gMOBIlabplus.dll PATHS 
			"C:/Windows/System32" 
			"C:/Windows/SysWOW64")		
		FIND_LIBRARY(LIB_GMobiLabCAPI GMobiLabplus PATHS ${PATH_GMobiLabCAPI}/x86)
	ENDIF()
	
	IF(PATH_GMobiLabCAPI AND PATH_GMobiLabDLL AND LIB_GMobiLabCAPI)
		OV_PRINT(OV_PRINTED "  Found gtec gMobiLabCAPI ...")
		OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_GMobiLabCAPI}")
			
		INCLUDE_DIRECTORIES(${PATH_GMobiLabCAPI})
			
		# Do not link to the dll! Its opened runtime with dlopen()
		# TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GMobiLabCAPI} )
		
		# Copy the DLL file at build
		file(COPY ${PATH_GMobiLabDLL}/gMOBIlabplus.dll DESTINATION ${CMAKE_BINARY_DIR}/bin)
		# Copy the DLL file at install
		INSTALL(PROGRAMS ${PATH_GMobiLabDLL}/gMOBIlabplus.dll DESTINATION ${DIST_BINDIR})
		
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGMobiLabPlusAPI)
		SET(OV_ThirdPartyGMobilab "YES")

	ELSE()
		OV_PRINT(OV_PRINTED "  FAILED to find gtec gMobiLabPlusAPI + lib + dll (optional driver)")
		#OV_PRINT(OV_PRINTED "    Results were ${PATH_GMobiLabCAPI} AND ${PATH_GMobiLabDLL} AND ${LIB_GMobiLabCAPI}")
	ENDIF()
ENDIF()

IF(UNIX)
	FIND_LIBRARY(gMOBIlabplus_LIBRARY NAMES "gMOBIlabplus" "gmobilabplusapi" PATHS "/usr/lib" "/usr/local/lib")
	IF(gMOBIlabplus_LIBRARY)
		OV_PRINT(OV_PRINTED "  Found gtec gMobiLabPlusAPI...")
		OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${gMOBIlabplus_LIBRARY}")
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGMobiLabPlusAPI)
		# Do not link to the dll! Its opened runtime with dlopen()
		# TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${gMOBIlabplus_LIBRARY} )
	ELSE()
		OV_PRINT(OV_PRINTED "  FAILED to find gtec gMobiLabPlusAPI... (optional driver)")
		OV_PRINT(OV_PRINTED "   : If it should be found, see that 'gmobilabapi.so' link exists on the fs, with no numeric suffixes in the filename.")
		OV_PRINT(OV_PRINTED "   : e.g. do 'cd /usr/lib/ ; ln -s libgmobilabplusapi.so.1.12 libgmobilabplusapi.so' ")
	ENDIF()
ENDIF()

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyGMobiLabPlusAPI "Yes")

