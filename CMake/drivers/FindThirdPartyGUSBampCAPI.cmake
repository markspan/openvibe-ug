# ---------------------------------
# Finds the gtec gUSBamp C API
#
# Defines the imported-like INTERFACE target "sdk-gtec-gusbamp", carrying the include
# directory, the link library and the TARGET_HAS_ThirdPartyGUSBampCAPI[_Linux] definition.
# Sets GUSBAMP_FOUND when the SDK is usable; consumers gate on it and simply link the
# target, without any directory-scoped side effect.
# ---------------------------------

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyGUSBampCAPI)

# Legacy call sites still include this module; searching once is enough and the
# INTERFACE target below may only be declared once.
if(TARGET sdk-gtec-gusbamp)
	return()
endif()

add_library(sdk-gtec-gusbamp INTERFACE)
set(GUSBAMP_FOUND FALSE)

if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(GUSBAMP_ARCH "x64")
	else()
		set(GUSBAMP_ARCH "Win32")
	endif()

	find_path(PATH_GUSBampCAPI gUSBamp.h PATHS
		"C:/Program Files/gtec/gUSBampCAPI/API"
		"C:/Program Files (x86)/gtec/gUSBampCAPI/API"
		"C:/Program Files/gtec/gUSBampCAPI/API/${GUSBAMP_ARCH}"
		"C:/Program Files (x86)/gtec/gUSBampCAPI/API/${GUSBAMP_ARCH}")

	if(PATH_GUSBampCAPI)
		OV_PRINT(OV_PRINTED "  Found gtec gUSBampCAPI...")
		find_library(LIB_GUSBampCAPI gUSBamp PATHS ${PATH_GUSBampCAPI})
		if(LIB_GUSBampCAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_GUSBampCAPI}")

			target_include_directories(sdk-gtec-gusbamp INTERFACE ${PATH_GUSBampCAPI})
			target_link_libraries(sdk-gtec-gusbamp INTERFACE ${LIB_GUSBampCAPI})
			target_compile_definitions(sdk-gtec-gusbamp INTERFACE TARGET_HAS_ThirdPartyGUSBampCAPI)
			set(GUSBAMP_FOUND TRUE)

			# The vendor DLL goes to bin/, next to the executable, not to bin/drivers/ with the
			# plugin: CInitApp adds the drivers folder to the DLL search path *in addition to*
			# the default dirs, which already include the executable's own directory.
			install(PROGRAMS "${PATH_GUSBampCAPI}/gUSBamp.dll" DESTINATION ${DIST_BINDIR})
		else()
			OV_PRINT(OV_PRINTED "    [FAILED] lib gUSBamp")
		endif()
	else()
		OV_PRINT(OV_PRINTED "  FAILED to find gtec gUSBampCAPI (optional driver)")
	endif()
endif(WIN32)


if(UNIX)
	# To try other versions of the gtec's library, change the number below.
	set(OV_SAVED_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".so.1.14")
	find_library(gUSBAmpLinux_LIBRARY NAMES "gusbampapi" PATHS "/usr/lib" "/usr/local/lib")
	# Reset CMAKE_FIND_LIBRARY_SUFFIXES to its original value not to affect future find_library calls.
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${OV_SAVED_FIND_LIBRARY_SUFFIXES})
	unset(OV_SAVED_FIND_LIBRARY_SUFFIXES)

	if(gUSBAmpLinux_LIBRARY)
		OV_PRINT(OV_PRINTED "  Found gtec gUSBAmpAPILinux...")
		OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${gUSBAmpLinux_LIBRARY}")

		target_link_libraries(sdk-gtec-gusbamp INTERFACE ${gUSBAmpLinux_LIBRARY})
		target_compile_definitions(sdk-gtec-gusbamp INTERFACE TARGET_HAS_ThirdPartyGUSBampCAPI_Linux)
		set(GUSBAMP_FOUND TRUE)
	else()
		OV_PRINT(OV_PRINTED "  FAILED to find gtec gUSBAmpAPI Linux... (optional)")
		OV_PRINT(OV_PRINTED "   : If it should be found, see that 'libgusbampapi.so.1.14' link exists on the fs, with no further nemeric suffixes in the filename.")
		OV_PRINT(OV_PRINTED "   : e.g. do 'cd /usr/lib/ ; ln -s libgusbampapi.so.1.14'. See gtec-bcilab/README for details.")
	endif()
endif(UNIX)

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyGUSBampCAPI "Yes")
