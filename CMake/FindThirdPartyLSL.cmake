###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Module to find the LabStreamingLayer library
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

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyLSL)

find_package(LSL 1.16)

if(LSL_FOUND)
    message(STATUS "Found LSL")
    target_compile_options(LSL::lsl
                           INTERFACE -DTARGET_HAS_ThirdPartyLSL
    )
else()
    message(WARNING "Failed to find LSL")
endif()

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyLSL "Yes")