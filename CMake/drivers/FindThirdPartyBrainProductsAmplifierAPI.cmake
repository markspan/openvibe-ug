###############################################################################
# Software License Agreement (AGPL-3 License)
#
# Author: Thomas Prampart (Inria)
#
# Copyright (C) 2022 Inria
#
# Module to find BrainProducts Amplifier SDK
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

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_FindThirdPartyBrainProductsAmplifierSDK)

add_library(brainproducts-amplifier-sdk INTERFACE)

if(WIN32)
    find_library(BRAINPRODUCTS_AMPLIFIER_LIB AmplifierSDK)
    find_path(BRAINPRODUCTS_AMPLIFIER_SDK_DIR AmplifierSDK.h PATH_SUFFIXES sdk-brain-products)

    if(BRAINPRODUCTS_AMPLIFIER_LIB AND BRAINPRODUCTS_AMPLIFIER_SDK_DIR)
        ov_print(OV_PRINTED "  Found Brain Products Amplifier SDK API...")

        target_include_directories(brainproducts-amplifier-sdk INTERFACE ${BRAINPRODUCTS_AMPLIFIER_SDK_DIR})
        target_link_libraries(brainproducts-amplifier-sdk INTERFACE ${BRAINPRODUCTS_AMPLIFIER_LIB})
        target_compile_options(brainproducts-amplifier-sdk
                               INTERFACE -DTARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
        )

    else(BRAINPRODUCTS_AMPLIFIER_LIB AND BRAINPRODUCTS_AMPLIFIER_SDK_DIR)
        ov_print(OV_PRINTED "  FAILED to find Brain Products Amplifier API (optional)")
    endif(BRAINPRODUCTS_AMPLIFIER_LIB AND BRAINPRODUCTS_AMPLIFIER_SDK_DIR)
endif(WIN32)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_FindThirdPartyBrainProductsAmplifierSDK "Yes")
