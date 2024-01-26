#
# Copyright (C) 2022 Inria
#
# Module to Find TMSI DLL
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

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyTMSi)

add_library(tmsi INTERFACE)
if(WIN32)
	find_path(TMSI_DLL_DIR TMSiSDK.dll)
endif(WIN32)

if(TMSI_DLL_DIR)
	OV_PRINT(OV_PRINTED "  Found TMSi dll...")
	OV_PRINT(OV_PRINTED "    [  OK  ] dll ${TMSI_DLL_DIR}/TMSiSDK.dll")
	
    target_compile_definitions(tmsi INTERFACE TARGET_HAS_ThirdPartyTMSi)
else(TMSI_DLL_DIR)
	OV_PRINT(OV_PRINTED "  FAILED to find TMSi device dll (optional driver)")
endif(TMSI_DLL_DIR)

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyTMSi "Yes")