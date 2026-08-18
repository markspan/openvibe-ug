# ---------------------------------
# Finds DSP filters sources
# Sets dsp_filters_source_files
# Adds appropriate include dir
# ---------------------------------

FILE(GLOB_RECURSE dsp_filters_source_files ${OV_SOURCE_DEPENDENCIES_PATH}/dsp-filters/*.cpp ${OV_SOURCE_DEPENDENCIES_PATH}/dsp-filters/*.c ${OV_SOURCE_DEPENDENCIES_PATH}/dsp-filters/*.h)
ADD_DEFINITIONS(-DTARGET_HAS_DSPFilters)
INCLUDE_DIRECTORIES("${OV_SOURCE_DEPENDENCIES_PATH}")
SET(SRC_FILES "${SRC_FILES};${dsp_filters_source_files}")
