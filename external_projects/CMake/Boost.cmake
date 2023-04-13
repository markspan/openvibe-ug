###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Module to add boost as an external project.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.
# If not, see <http://www.gnu.org/licenses/>.
###############################################################################
include(ExternalProject)

# #############################################################################
# Library details
# #############################################################################
set(LIB boost)
set(LIB_VERSION 1.71.0)

# #############################################################################
# List the dependencies of the project
# #############################################################################

list(APPEND ${LIB}_DEPENDENCIES "")

# #############################################################################
# Prepare the project
# #############################################################################

if (NOT USE_SYSTEM_${LIB})

    # Setup download
    set(GIT_URL https://github.com/boostorg/boost.git)
    set(GIT_TAG boost-${LIB_VERSION})

    # Modules to build
    set(WITH_MODULES
            --with-atomic
            --with-chrono
            --with-date_time
            --with-filesystem
            --with-regex
            --with-system
            --with-thread
    )

    # Libs needed / Saves a few minutes of dependency building to limit to what is needed.
    # Will need updating when another lib is needed.
    # Can be removed if too constraining.
    set(${LIB}_MODULES
            tools
            libs/config
            libs/headers
            libs/algorithm
            libs/array
            libs/asio
            libs/assert
            libs/atomic
            libs/bind
            libs/chrono
            libs/concept_check
            libs/container
            libs/container_hash
            libs/core
            libs/date_time
            libs/detail
            libs/exception
            libs/filesystem
            libs/foreach
            libs/format
            libs/function
            libs/function_types
            libs/fusion
            libs/interprocess
            libs/integer
            libs/intrusive
            libs/iterator
            libs/io
            libs/iostreams
            libs/lexical_cast
            libs/lockfree
            libs/math
            libs/move
            libs/mp11
            libs/mpl
            libs/multiprecision
            libs/numeric
            libs/optional
            libs/parameter
            libs/phoenix
            libs/predef
            libs/preprocessor
            libs/proto
            libs/range
            libs/ratio
            libs/regex
            libs/serialization
            libs/smart_ptr
            libs/spirit
            libs/static_assert
            libs/system
            libs/thread
            libs/throw_exception
            libs/tokenizer
            libs/tuple
            libs/type_index
            libs/type_traits
            libs/typeof
            libs/unordered
            libs/utility
            libs/variant
    )

    # Setup build
    if(UNIX)
        set(CONFIG_CMD ./bootstrap.sh)
        set(BUILD_CMD ./b2)
        set(${LIB}_CXXFLAGS cxxflags="-fPIC")
    else()
        if(WIN32)
            set(CONFIG_CMD bootstrap.bat)
            set(BUILD_CMD b2.exe)
            list(APPEND ${LIB}_MODULES libs/winapi)
            message(STATUS "toolset veresion: ${MSVC_TOOLSET_VERSION}")
            if(${MSVC_TOOLSET_VERSION} STREQUAL "120")
                set(${LIB}_TOOLSET toolset=msvc-12.0)
            elseif(${MSVC_TOOLSET_VERSION} STREQUAL "140")
                set(${LIB}_TOOLSET toolset=msvc-14.0)
            elseif(${MSVC_TOOLSET_VERSION} STREQUAL "141")
                set(${LIB}_TOOLSET toolset=msvc-14.1)
            elseif(${MSVC_TOOLSET_VERSION} STREQUAL "142")
                set(${LIB}_TOOLSET toolset=msvc-14.2)
            else()
                message(WARNING "${LIB} install MSVC toolset not defined")
            endif()
            if(${CMAKE_GENERATOR_PLATFORM} STREQUAL "x64")
                set(${LIB}_ADDRESS_MODEL "address-model=64")
            else()
                set(${LIB}_ADDRESS_MODEL "address-model=32")
            endif()
        endif()
    endif()


    # Setup build type (options are "release | debug")
    string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
    set(${LIB}_BUILD_TYPE "variant=${CMAKE_BUILD_TYPE_TOLOWER}")

    # Setup link option (options are "static", "shared" or "static,shared")
    set(${LIB}_LINK "link=static")

    find_program(GIT_BIN NAMES git)
    set (patch_cmd ${GIT_BIN} apply --ignore-whitespace --directory=libs/regex ${CMAKE_SOURCE_DIR}/patches/BoostRegexConfig.patch)

    ## #############################################################################
    ## Add external-project
    ## #############################################################################
    ExternalProject_Add(${LIB}
            PREFIX ${EP_DEPENDENCIES_WORK_DIR}/${LIB}
            GIT_REPOSITORY ${GIT_URL}
            GIT_TAG ${GIT_TAG}
            GIT_SUBMODULES ${${LIB}_MODULES}
            PATCH_COMMAND "${patch_cmd}"
            BUILD_IN_SOURCE ON
            CONFIGURE_COMMAND ${CONFIG_CMD}
            BUILD_COMMAND ${BUILD_CMD} ${${LIB}_BUILD_TYPE} ${${LIB}_ADDRESS_MODEL} ${${LIB}_TOOLSET} ${${LIB}_LINK} ${${LIB}_CXXFLAGS} ${WITH_MODULES}
            INSTALL_COMMAND ${BUILD_CMD} install ${${LIB}_BUILD_TYPE} ${${LIB}_ADDRESS_MODEL} ${${LIB}_TOOLSET} ${${LIB}_LINK} ${${LIB}_CXXFLAGS} ${WITH_MODULES} --prefix=${EP_DEPENDENCIES_DIR}/${LIB}
            DEPENDS ${${LIB}_DEPENDENCIES}
    )

endif() #NOT USE_SYSTEM_LIB

unset(LIB)
unset(LIB_VERSION)