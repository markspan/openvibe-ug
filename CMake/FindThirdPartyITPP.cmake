###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Module to find ITPP library
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

find_library(ITPP_LIBRARY itpp)
find_path(ITPP_INCLUDE_DIR itbase.h PATH_SUFFIXES itpp)

if(NOT ITPP_LIBRARY STREQUAL ITPP_LIBRARY-NOTFOUND
   AND NOT ITPP_INCLUDE_DIR STREQUAL ITPP_INCLUDE_DIR-NOTFOUND)
    ov_print(OV_PRINTED "Found ITPP library")

    add_library(itpp INTERFACE)
    target_include_directories(itpp INTERFACE ${ITPP_INCLUDE_DIR})
    target_link_libraries(itpp INTERFACE ${ITPP_LIBRARY})
    target_compile_definitions(itpp INTERFACE -DTARGET_HAS_ThirdPartyITPP)
else ()
    ov_print(OV_PRINTED "Failed to find ITPP library")
endif()
