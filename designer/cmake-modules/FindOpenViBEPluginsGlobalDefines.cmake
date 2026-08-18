# ---------------------------------
# Finds OpenViBEPluginsGlobalDefines
# Adds library to target
# Adds include path
#
# @deprecated Running FindOpenVibeCommon.cmake is sufficient
#
# ---------------------------------
SET(PATH_OpenViBEPluginsGlobalDefines "PATH_OpenViBEPluginsGlobalDefines-NOTFOUND")
FIND_PATH(PATH_OpenViBEPluginsGlobalDefines ovp_global_defines.h PATHS ${OV_BASE_DIR}/common/include NO_DEFAULT_PATH)
IF(PATH_OpenViBEPluginsGlobalDefines)
	debug_message( "  Found OpenViBE plugins global defines...")
	INCLUDE_DIRECTORIES(${PATH_OpenViBEPluginsGlobalDefines})

	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)
ELSE(PATH_OpenViBEPluginsGlobalDefines)
	MESSAGE(WARNING "  FAILED to find OpenViBE plugins global defines")
ENDIF(PATH_OpenViBEPluginsGlobalDefines)
