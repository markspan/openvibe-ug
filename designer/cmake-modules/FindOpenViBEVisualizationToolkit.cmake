# ---------------------------------
# Finds openvibe-toolkit
# Adds library to target
# Adds include path
# ---------------------------------
option(LINK_OPENVIBE_VISUALIZATION_TOOLKIT "By default, link openvibe-visualization-toolkit, otherwise only use the includes" ON)
option(DYNAMIC_LINK_OPENVIBE_VISUALIZATION_TOOLKIT "Dynamically link openvibe-visualization-toolkit" ON)

if(DYNAMIC_LINK_OPENVIBE_VISUALIZATION_TOOLKIT)
	set(OPENVIBE_VISUALIZATION_TOOLKIT_LINKING "")
	add_definitions(-DOVVIZ_Shared)
else()
	set(OPENVIBE_VISUALIZATION_TOOLKIT_LINKING "-static")
	add_definitions(-DOVVIZ_Static)
endif()

set(SRC_DIR ${OV_BASE_DIR}/visualization-toolkit/include)

set(PATH_OPENVIBE_VISUALIZATION_TOOLKIT "PATH_OPENVIBE_VISUALIZATION_TOOLKIT-NOTFOUND")
find_path(PATH_OPENVIBE_VISUALIZATION_TOOLKIT visualization-toolkit/ovviz_all.h PATHS ${SRC_DIR} NO_DEFAULT_PATH)
if(PATH_OPENVIBE_VISUALIZATION_TOOLKIT)
	debug_message( "  Found openvibe-toolkit...  ${PATH_OPENVIBE_VISUALIZATION_TOOLKIT}")
	include_directories(${PATH_OPENVIBE_VISUALIZATION_TOOLKIT}/)
		
	if(LINK_OPENVIBE_VISUALIZATION_TOOLKIT)
		target_link_libraries(${PROJECT_NAME} PRIVATE openvibe-visualization-toolkit${OPENVIBE_VISUALIZATION_TOOLKIT_LINKING})
	endif()

	add_definitions(-DTARGET_HAS_OpenViBEVisualizationToolkit)
else()
	message(WARNING "  FAILED to find openvibe-visualization-toolkit...")
endif()

