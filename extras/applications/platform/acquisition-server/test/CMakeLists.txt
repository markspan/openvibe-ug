# SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

project(test-acquisition-server-core VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

# Test files: auto-discovered. Add a new *Test.cpp here to cover a new class.
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*Test.cpp)

# Tested production sources: explicit, one line per class covered.
# Only add a class here once its dependencies are confirmed test-binary-safe (no Qt/NetService pulled in).
set(TESTED_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/DriftCalculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/DriftCorrector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/Acquisition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/CircularBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/SampleBufferHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/Thread.cpp
)

add_executable(${PROJECT_NAME} ${TEST_SOURCES} ${TESTED_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../core)
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(${PROJECT_NAME} PRIVATE
    openvibe
    openvibe-acquisition-abstractions
    GTest::GTest
    GTest::Main
)
SET_BUILD_PLATFORM()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
