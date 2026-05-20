# SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

include(FetchContent)

set(PBS_VERSION "20260504")
set(PY_VER_MAJOR "3")
set(PY_VER_MINOR "10")
set(PY_VER_PATCH "20")

if(WIN32)
    set(PYTHON_ARCH "x86_64-pc-windows-msvc-install_only")
    set(PBS_LIB_PATH "libs/python${PY_VER_MAJOR}${PY_VER_MINOR}.lib")
    set(PBS_INCLUDE_PATH "include")
    set(PBS_EXECUTABLE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/python.exe)
    set(PBS_SITE_PACKAGES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/Lib/site-packages")
else()
    if(APPLE)
        set(PYTHON_ARCH "aarch64-apple-darwin-install_only")
        set(PBS_LIB_PATH "lib/libpython${PY_VER_MAJOR}.${PY_VER_MINOR}.dylib")
    else()
        set(PYTHON_ARCH "x86_64-unknown-linux-gnu-install_only")
        set(PBS_LIB_PATH "lib/libpython${PY_VER_MAJOR}.${PY_VER_MINOR}.so")
    endif()

    set(PBS_INCLUDE_PATH "include/python${PY_VER_MAJOR}.${PY_VER_MINOR}")
    set(PBS_EXECUTABLE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/bin/python)
    set(PBS_SITE_PACKAGES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/lib/python${PY_VER_MAJOR}.${PY_VER_MINOR}/site-packages")
endif()


FetchContent_Declare(
    python_standalone
    URL "https://github.com/astral-sh/python-build-standalone/releases/download/${PBS_VERSION}/cpython-${PY_VER_MAJOR}.${PY_VER_MINOR}.${PY_VER_PATCH}+${PBS_VERSION}-${PYTHON_ARCH}.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(python_standalone)

add_library(PythonBuildStandalone INTERFACE)
target_include_directories(PythonBuildStandalone INTERFACE ${python_standalone_SOURCE_DIR}/${PBS_INCLUDE_PATH})
target_link_libraries(PythonBuildStandalone INTERFACE ${python_standalone_SOURCE_DIR}/${PBS_LIB_PATH})
target_compile_options(PythonBuildStandalone INTERFACE -DTARGET_HAS_ThirdPartyPython3)

add_custom_target(PythonBuildStandalone_Deploy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${python_standalone_SOURCE_DIR}/ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/
    #COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/python${PY_VER_MAJOR}${PY_VER_MINOR}.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
    COMMAND ${PBS_EXECUTABLE} -s -m pip install numpy
    COMMENT "Copy Python-build-standalone to build directory"
)

if(WIN32)
    add_custom_command(TARGET PythonBuildStandalone_Deploy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime/python${PY_VER_MAJOR}${PY_VER_MINOR}.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
    )
    install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python_runtime
        DESTINATION bin
    )
    install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python${PY_VER_MAJOR}${PY_VER_MINOR}.dll
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