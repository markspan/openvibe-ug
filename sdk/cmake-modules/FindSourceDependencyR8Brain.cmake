# ---------------------------------
# Find r8brain sources
# Sets r9brain_source_files
# Adds appropriate include dir
# ---------------------------------

FILE(GLOB_RECURSE r8brain_source_files ${OV_SOURCE_DEPENDENCIES_PATH}/r8brain/*.cpp ${OV_SOURCE_DEPENDENCIES_PATH}/r8brain/*.h)
ADD_DEFINITIONS(-DTARGET_HAS_R8BRAIN)
INCLUDE_DIRECTORIES("${OV_SOURCE_DEPENDENCIES_PATH}")
SET(SRC_FILES "${SRC_FILES};${r8brain_source_files}")
