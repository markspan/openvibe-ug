###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Module to find Eigen library
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

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyEigen)

find_package(Eigen3 3.3.7)

if(TARGET Eigen3::Eigen)
    message(STATUS "Found Eigen")
    target_compile_options(Eigen3::Eigen
                           INTERFACE -DTARGET_HAS_ThirdPartyEIGEN
    )
else()
    message(WARNING "Failed to find Eigen3")
endif()

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyEigen "Yes")
