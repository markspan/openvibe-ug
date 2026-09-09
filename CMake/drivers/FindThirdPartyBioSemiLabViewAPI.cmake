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

# The INTERFACE target below may only be declared once.
if(TARGET sdk-biosemi)
	return()
endif()

set(BIOSEMI_FOUND FALSE)

if(WIN32)
	set(PATH_BioSemiAPI "C:/Program Files/BioSemi/Developers_kit/C-code")

	find_file(LABVIEW_HEADER labview_dll.h
		PATHS ${PATH_BioSemiAPI}
		NO_SYSTEM_ENVIRONMENT_PATH
		NO_DEFAULT_PATH)

	if(LABVIEW_HEADER)
		message(STATUS "  Found BioSemi LabView API...")
		find_library(LIB_BioSemiAPI Labview_DLL PATHS ${PATH_BioSemiAPI} NO_SYSTEM_ENVIRONMENT_PATH NO_DEFAULT_PATH)
		if(LIB_BioSemiAPI)
			message(STATUS "    [  OK  ] lib ${LIB_BioSemiAPI}")

			add_library(sdk-biosemi INTERFACE)
			target_include_directories(sdk-biosemi INTERFACE ${PATH_BioSemiAPI})
			target_link_libraries(sdk-biosemi INTERFACE ${LIB_BioSemiAPI})
			target_compile_definitions(sdk-biosemi INTERFACE TARGET_HAS_ThirdPartyBioSemiAPI)
			set(BIOSEMI_FOUND TRUE)

			# The drivers are built into the acquisition server executable, so the vendor DLL
			# belongs next to it in bin/.
			file(COPY "${PATH_BioSemiAPI}/Labview_DLL.dll" DESTINATION ${CMAKE_BINARY_DIR}/bin)
			install(PROGRAMS "${PATH_BioSemiAPI}/Labview_DLL.dll" DESTINATION ${DIST_BINDIR})
		else()
			message(WARNING "    [FAILED] lib Labview_DLL")
		endif()
	else()
		message(WARNING "  FAILED to find BioSemi LabView API (optional driver)")
	endif()
endif()

