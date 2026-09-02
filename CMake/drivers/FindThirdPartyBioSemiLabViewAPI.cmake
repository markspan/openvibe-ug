# ---------------------------------
# Finds the BioSemi LabView API
#
# Defines the imported-like INTERFACE target "sdk-biosemi", carrying the include
# directory, the link library and the TARGET_HAS_ThirdPartyBioSemiAPI definition.
# Sets BIOSEMI_FOUND when the SDK is usable; consumers gate on it and simply link the
# target, without any directory-scoped side effect.
#
# The SDK ships only for Windows, so the ActiveTwo and ActiveThree drivers are a
# Windows-only build.
# ---------------------------------

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyBioSemiLabViewAPI)

# The INTERFACE target below may only be declared once.
if(TARGET sdk-biosemi)
	return()
endif()

add_library(sdk-biosemi INTERFACE)
set(BIOSEMI_FOUND FALSE)

if(WIN32)
	# Care must be taken here that a lib of the wrong bitness is not picked: the vendor
	# installer puts the 32-bit kit under "Program Files (x86)".
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(PATH_BioSemiAPI "C:/Program Files/BioSemi/Developers_kit/C-code")
	else()
		set(PATH_BioSemiAPI "C:/Program Files (x86)/BioSemi/Developers_kit/C-code")
	endif()

	find_file(LABVIEW_HEADER labview_dll.h
		PATHS ${PATH_BioSemiAPI}
		NO_SYSTEM_ENVIRONMENT_PATH
		NO_DEFAULT_PATH)

	if(LABVIEW_HEADER)
		OV_PRINT(OV_PRINTED "  Found BioSemi LabView API...")
		find_library(LIB_BioSemiAPI Labview_DLL PATHS ${PATH_BioSemiAPI} NO_SYSTEM_ENVIRONMENT_PATH NO_DEFAULT_PATH)
		if(LIB_BioSemiAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_BioSemiAPI}")

			target_include_directories(sdk-biosemi INTERFACE ${PATH_BioSemiAPI})
			target_link_libraries(sdk-biosemi INTERFACE ${LIB_BioSemiAPI})
			target_compile_definitions(sdk-biosemi INTERFACE TARGET_HAS_ThirdPartyBioSemiAPI)
			set(BIOSEMI_FOUND TRUE)

			# The drivers are built into the acquisition server executable, so the vendor DLL
			# belongs next to it in bin/.
			file(COPY "${PATH_BioSemiAPI}/Labview_DLL.dll" DESTINATION ${CMAKE_BINARY_DIR}/bin)
			install(PROGRAMS "${PATH_BioSemiAPI}/Labview_DLL.dll" DESTINATION ${DIST_BINDIR})
		else()
			OV_PRINT(OV_PRINTED "    [FAILED] lib Labview_DLL")
		endif()
	else()
		OV_PRINT(OV_PRINTED "  FAILED to find BioSemi LabView API (optional driver)")
	endif()
endif(WIN32)

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyBioSemiLabViewAPI "Yes")
