/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "parameters/parameterUtils.h"
#include "parameters/parameterBool.h"
#include "parameters/parameterColor.h"
#include "parameters/parameterColorGradient.h"
#include "parameters/parameterInt.h"
#include "parameters/parameterDouble.h"
#include "parameters/parameterString.h"
#include "parameters/parameterListString.h"
#include "parameters/parameterMultiSelect.h"
#include "parameters/parameterPath.h"
#include "parameters/parameterTraits.h"
#include "CIdentifier.hpp"
#include "ov_defines.h"

#include <sstream>

namespace OpenViBE {
namespace Parameters {


OV_API const std::map<CIdentifier, std::string>& getTypeIdToTypeMap() {
    static const std::map<CIdentifier, std::string> typeIdToTypeMap = {
        { OV_TypeId_Boolean, "bool" },
        { OV_TypeId_Integer, "int" },
        { OV_TypeId_Float, "double" },
        { OV_TypeId_String, "string" },
        { OV_TypeId_Foldername, "path" },
        { OV_TypeId_Filename, "path" },
        { OV_TypeId_Script, "path" },
        { OV_TypeId_Color, "color" },
        { OV_TypeId_ColorGradient, "colorgradient" }
    };
    return typeIdToTypeMap;
}

OV_API std::string getTypeFromTypeId(const CIdentifier& typeId) {
    const auto it = getTypeIdToTypeMap().find(typeId);
    if (it != getTypeIdToTypeMap().end()) {
        return it->second;
    } else {
        // throw std runtime error
        throw std::runtime_error("getTypeFromTypeId(): no type found for type id " + typeId.str());
        return "";
    }
}

}  // namespace Parameters
}  // namespace OpenViBE
