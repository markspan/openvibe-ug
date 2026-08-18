/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <any>

#include "ov_common_defines.h"
#include "../CIdentifier.hpp"
#include "parameterTraits.h"


namespace OpenViBE {
namespace Parameters {


/**
 * @brief Get the correspondance map between type ID and type string
 * @return The map
 */
OV_API const std::map<CIdentifier, std::string>& getTypeIdToTypeMap();

/**
 * @brief Get the new string type from CIdentifier type ID
 * @param type [in] : The type ID
 * @return The string type
 */
OV_API std::string getTypeFromTypeId(const CIdentifier& typeId);

}
}