project(test-module-eigen VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE TESTS_SRC_FILES *.cpp *.hpp)

add_executable(${PROJECT_NAME} ${TESTS_SRC_FILES})
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})	# Place project in folder unit-test (for some IDE)

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-module-eigen
					  Eigen3::Eigen
					  GTest::GTest
)

# ---------------------------------
# Target macros
# Defines target operating system, architecture and compiler
# ---------------------------------
SET_BUILD_PLATFORM()

ADD_TEST(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
