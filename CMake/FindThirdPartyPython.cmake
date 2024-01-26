# ---------------------------------
# Finds third party python
# ---------------------------------

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyPython)


if (WIN32)
  # needed when we run cmake in a conda environment
  set(Python3_FIND_REGISTRY LAST)
endif()
if (APPLE)
  # needed when we run cmake in a conda environment
  set(Python3_FIND_FRAMEWORK LAST)
endif()

find_package(Python3 3.10 EXACT COMPONENTS Interpreter Development)

if(Python3_FOUND)
  OV_PRINT(OV_PRINTED "  Found Python 3 at ${Python3_LIBRARIES}")
   ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyPython3)
else()
   OV_PRINT(OV_PRINTED "  FAILED to find Python 3 (needs v3.10 with bitness matching build target ${PLATFORM_TARGET})")
endif()

set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})

include_directories(${Python3_INCLUDE_DIRS})


set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyPython "Yes")
