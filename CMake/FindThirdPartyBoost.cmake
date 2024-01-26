# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost)

#set(Boost_NO_SYSTEM_PATHS ON)
#set(Boost_USE_MULTITHREAD ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.71.0 COMPONENTS chrono filesystem regex serialization system thread)

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost "Yes")