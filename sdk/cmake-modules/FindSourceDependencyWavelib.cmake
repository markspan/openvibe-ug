# ---------------------------------
# Finds wavelib sources
# Sets wavelib_source_files
# Adds appropriate include dir
# ---------------------------------

FILE(GLOB_RECURSE wavelib_source_files ${OV_SOURCE_DEPENDENCIES_PATH}/wavelib/src/*.cpp ${OV_SOURCE_DEPENDENCIES_PATH}/wavelib/src/*.c ${OV_SOURCE_DEPENDENCIES_PATH}/wavelib/src/*.h ${OV_SOURCE_DEPENDENCIES_PATH}/wavelib/header/*.h)
ADD_DEFINITIONS(-DTARGET_HAS_WAVELIB)
INCLUDE_DIRECTORIES("${OV_SOURCE_DEPENDENCIES_PATH}")
SET(SRC_FILES "${SRC_FILES};${wavelib_source_files}")
