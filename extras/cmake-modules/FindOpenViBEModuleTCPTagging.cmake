# ---------------------------------
# Finds module TCPTagging
# Adds library to target
# Adds include path
# ---------------------------------

option(DYNAMIC_LINK_OPENVIBE_MODULE_TCPTAGGING "Dynamically link OpenViBE module TCPTagging" ON)

if(DYNAMIC_LINK_OPENVIBE_MODULE_TCPTAGGING)
	set(OPENVIBE_MODULE_TCPTAGGING_LINKING "")
else(DYNAMIC_LINK_OPENVIBE_MODULE_TCPTAGGING)
	set(OPENVIBE_MODULE_TCPTAGGING_LINKING "-static")
endif(DYNAMIC_LINK_OPENVIBE_MODULE_TCPTAGGING)

set(SRC_DIR ${OV_BASE_DIR}/modules/tcptagging/include)

find_path(PATH_OPENVIBE_MODULES_TCPTAGGING tcptagging/defines.h PATHS ${SRC_DIR} NO_DEFAULT_PATH)
if(PATH_OPENVIBE_MODULES_TCPTAGGING)
	message(STATUS "  Found OpenViBE module TCPTagging...      ${PATH_OPENVIBE_MODULES_TCPTAGGING}")
	include_directories(${PATH_OPENVIBE_MODULES_TCPTAGGING}/)

	target_link_libraries(${PROJECT_NAME} PUBLIC openvibe-module-tcptagging${OPENVIBE_MODULE_TCPTAGGING_LINKING})

	add_definitions(-DTARGET_HAS_TCPTagging)
else(PATH_OPENVIBE_MODULES_TCPTAGGING)
	message(WARNING "  FAILED to find OpenViBE module TCPTagging...")
endif(PATH_OPENVIBE_MODULES_TCPTAGGING)
