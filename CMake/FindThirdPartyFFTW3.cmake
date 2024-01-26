###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Module to find FFTW library
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

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyFFTW)

find_package(FFTW3 REQUIRED)

if(FFTW3_FOUND)
    message(STATUS "Found FFTW3")

    add_library(fftw3 INTERFACE)
    target_include_directories(fftw3
                               INTERFACE ${FFTW3_INCLUDE_DIRS}
    )
    foreach(fftw3_library ${FFTW3_LIBRARIES})
        set(fftw3_lib "FFTW3_LIB-NOTFOUND")
        find_library(fftw3_lib NAMES ${fftw3_library} PATHS ${FFTW3_LIBRARY_DIRS} NO_DEFAULT_PATH)
        if (fftw3_lib)
            target_link_libraries(fftw3 INTERFACE ${fftw3_lib})
        else()
            message(WARNING "Failed to find FFTW3 library ${fftw3_library} in ${FFTW3_LIBRARY_DIRS}")
        endif()
    endforeach()
    target_compile_options(fftw3
                           INTERFACE -DTARGET_HAS_ThirdPartyFFTW3
    )
else()
    message(WARNING "Failed to find FFTW3")
endif()

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyFFTW "Yes")