###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Module to find Expat library
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

find_package(EXPAT REQUIRED)

IF(EXPAT_FOUND)
    message(STATUS "Found Expat")
	target_compile_options(EXPAT::EXPAT
                           INTERFACE -DTARGET_HAS_ThirdPartyExpat
    )
ELSE(EXPAT_FOUND)
	MESSAGE(WARNING "FAILED to find Expat")
ENDIF(EXPAT_FOUND)