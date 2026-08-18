# ---------------------------------
# Finds module Geometry
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEModuleGeometry)

set(SRC_DIR ${OV_BASE_DIR}/modules/geometry/include)

FIND_PATH(PATH_OPENVIBE_MODULES_GEOMETRY geometry PATHS ${SRC_DIR} NO_DEFAULT_PATH)
IF(PATH_OPENVIBE_MODULES_GEOMETRY)
	OV_PRINT(OV_PRINTED "  Found OpenViBE module Geometry...        ${PATH_OPENVIBE_MODULES_GEOMETRY}")
	INCLUDE_DIRECTORIES(${PATH_OPENVIBE_MODULES_GEOMETRY}/)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} openvibe-module-geometry)
	ADD_DEFINITIONS(-DTARGET_HAS_Geometry)
ELSE(PATH_OPENVIBE_MODULES_GEOMETRY)
	OV_PRINT(OV_PRINTED "  FAILED to find OpenViBE module Geometry...")
ENDIF(PATH_OPENVIBE_MODULES_GEOMETRY)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEModuleGeometry "Yes")

