#
# Copyright (C) 2022 Inria
#
# Module to Find Micromed DLL
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

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyMicromed)

add_library(micromed INTERFACE)
if(WIN32)
	find_path(MICROMED_DLL_DIR dllMicromed.dll)
endif(WIN32)

if(MICROMED_DLL_DIR)
	OV_PRINT(OV_PRINTED "  Found Micromed dll...")
	OV_PRINT(OV_PRINTED "    [  OK  ] dll ${MICROMED_DLL_DIR}/dllMicromed.dll")
	
	target_compile_definitions(micromed INTERFACE TARGET_HAS_ThirdPartyMicromed)
else(MICROMED_DLL_DIR)
	OV_PRINT(OV_PRINTED "  FAILED to find Micromed device dlls (optional driver)")
endif(MICROMED_DLL_DIR)

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyMicromed "Yes")