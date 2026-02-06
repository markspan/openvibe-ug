# SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

get_property(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyNlohmannJSON)

find_package(nlohmann_json 3.11.3 REQUIRED)

IF(nlohmann_json_FOUND)

    ov_print(OV_PRINTED "Found nlohmann_json library")
    target_compile_definitions(nlohmann_json::nlohmann_json 
                               INTERFACE -DTARGET_HAS_ThirdPartyNlohmannJSON)

ELSE()
    ov_print(OV_PRINTED "  FAILED to find NLhomann JSON")
ENDIF()

set_property(GLOBAL PROPERTY OV_TRIED_ThirdPartyNlohmannJSON "Yes")
