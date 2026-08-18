#######################################################################
# Software License Agreement : GNU Affero General Public License v3.0
# https://choosealicense.com/licenses/agpl-3.0/ 
#######################################################################

project(ov-base-test VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

SET_BUILD_PLATFORM()	# Default build Platform

file(GLOB_RECURSE SRC_FILES *.cpp *.hpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  GTest::GTest
					  GTest::Main
)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})  # Place project in folder unit-test (for some IDE)

add_test(NAME base COMMAND ${PROJECT_NAME})
