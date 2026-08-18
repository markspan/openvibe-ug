project(test_tcptagging-service VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

add_executable(${PROJECT_NAME} test_tcptaggingservice.cpp)

target_link_libraries(${PROJECT_NAME}
    TcpTaggingService
    openvibe
    openvibe-module-network
    openvibe-module-system
    GTest::GTest
    GTest::Main
)

if(UNIX AND NOT APPLE)
    find_library(LIB_RT rt)
    if(LIB_RT)
        target_link_libraries(${PROJECT_NAME} ${LIB_RT})
    else()
        message(WARNING "  FAILED to find rt...")
    endif()
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${TESTS_FOLDER})

# TcpTaggingService is built as a shared lib into a "services" output subdirectory
# (setup_output_paths_services(), mirroring the real app's plugin layout), which
# the OS loader does not search implicitly on any platform. Point the relevant
# shared-library search env var at it so the test executable can find it at
# runtime, regardless of platform.
if(WIN32)
    set(_tcptagging_test_env "PATH=$<TARGET_FILE_DIR:TcpTaggingService>\;$ENV{PATH}")
elseif(APPLE)
    set(_tcptagging_test_env "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:TcpTaggingService>:$ENV{DYLD_LIBRARY_PATH}")
else()
    set(_tcptagging_test_env "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:TcpTaggingService>:$ENV{LD_LIBRARY_PATH}")
endif()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT "${_tcptagging_test_env}")
