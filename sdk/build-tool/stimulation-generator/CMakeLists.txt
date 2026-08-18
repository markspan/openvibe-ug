PROJECT(openvibe-stimulation-generator VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)
ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	FOLDER ${MISC_FOLDER}
)

# if(UNIX)
# SET(stim_generator_path  "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "stim_generator_path")
# else()
# get_property(path_to_target TARGET openvibe-stimulation-generator PROPERTY LOCATION)
# SET(stim_generator_path  "${path_to_target}" CACHE INTERNAL "stim_generator_path")
# MESSAGE(STATUS "MOO ${stim_generator_path}")
#endif()

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()
