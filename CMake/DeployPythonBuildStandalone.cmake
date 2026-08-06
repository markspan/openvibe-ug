# Driven by a stamp file rather than by a bare custom target: a custom target will trigger ninja to
# re-run every time.
set(PBS_DEPLOY_STAMP ${CMAKE_BINARY_DIR}/python-build-standalone-deploy.stamp)

add_custom_command(
    OUTPUT ${PBS_DEPLOY_STAMP}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${python_standalone_SOURCE_DIR}/ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/
    #COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/python${PY_VER_MAJOR}${PY_VER_MINOR}.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
    COMMAND ${PBS_EXECUTABLE} -s -m pip install numpy
    COMMAND ${CMAKE_COMMAND} -E touch ${PBS_DEPLOY_STAMP}
    # Re-deploy when FetchContent brings in a different runtime (e.g. after a PBS_VERSION bump).
    DEPENDS ${python_standalone_SOURCE_DIR}/${PBS_LIB_PATH}
    COMMENT "Copy Python-build-standalone to build directory"
)

add_custom_target(PythonBuildStandalone_Deploy ALL DEPENDS ${PBS_DEPLOY_STAMP})

if(WIN32)
    #add_custom_command(TARGET PythonBuildStandalone_Deploy POST_BUILD
    #    COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/python${PY_VER_MAJOR}${PY_VER_MINOR}.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python${PY_VER_MAJOR}${PY_VER_MINOR}.dll
    #)
    install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime
        DESTINATION bin
    )
else()
    # conda env injects -rpath via LDFLAGS, which would appear before our python_runtime in RPATH.
    # Strip it from linker flags and re-add both paths in the correct order via CMAKE_BUILD_RPATH.
    if(APPLE)
        set(PBS_RPATH_ORIGIN "@loader_path")
    else()
        set(PBS_RPATH_ORIGIN "$ORIGIN")
    endif()

    set(CMAKE_BUILD_RPATH_USE_LINK_PATH FALSE)
    foreach(flag_var CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
        string(REGEX REPLACE "-Wl,-rpath(-link)?[^ ]*" " " ${flag_var} "${${flag_var}}")
        string(REGEX REPLACE "-Wl,--disable-new-dtags" " " ${flag_var} "${${flag_var}}")
    endforeach()

    list(PREPEND CMAKE_BUILD_RPATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/lib")
    #list(PREPEND CMAKE_INSTALL_RPATH "${PBS_RPATH_ORIGIN}/../bin/python_runtime/lib") # libopenvibe*.so live in lib/ next to bin/
    if(DEFINED ENV{CONDA_PREFIX})
        list(APPEND CMAKE_BUILD_RPATH "$ENV{CONDA_PREFIX}/lib")
        #list(APPEND CMAKE_INSTALL_RPATH "$ENV{CONDA_PREFIX}/lib")
    endif()
endif()
