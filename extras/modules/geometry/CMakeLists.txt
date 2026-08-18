project(openvibe-module-geometry VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp include/*.h)
include_directories(include)

# We use static library to allow template functions and stl object in class
add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  Boost::boost
					  Eigen3::Eigen
					  tinyxml2::tinyxml2)

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER ${MODULES_FOLDER})

if(UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif(UNIX)

if(WIN32)
ADD_DEFINITIONS(/bigobj)	# Definition for big obj file in debug mode with visual studio
ENDIF(WIN32)

ADD_DEFINITIONS(-D_USE_MATH_DEFINES)	# Definition for constant math as M_PI


# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

# ---------------------------------
# Test applications
# ---------------------------------
if(OV_COMPILE_TESTS)
ADD_SUBDIRECTORY(test)
endif()