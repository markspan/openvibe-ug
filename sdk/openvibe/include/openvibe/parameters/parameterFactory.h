/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include <map>
#include <memory>
#include <string>
#include <functional>
#include <any>

#include <nlohmann/json_fwd.hpp>

#include "../ov_defines.h"


namespace OpenViBE {
namespace Parameters {

class ParameterTraits;

class OV_API ParameterFactory {
public:
    /**
     * @brief Builds a parameter of corresponding type
     * @param type The desired type for the parameter
     * @return The parameter, owned by the caller
     */
    static ParameterTraits* create(const std::string& type);

    /**
     * @brief Builds a paramter of corresponding type
     * This overload avoid ambiguity between std::string and nlohmann::json overloads
     * @param type The desired type for the parameter
     * @return The parameter, owned by the caller
     */
    static ParameterTraits* create(const char* type);

    /**
     * @brief Builds a parameter with corresponding type, from input stream
     * @param is The input stream to read the type from
     * @return The parameter, owned by the caller
     */
    static ParameterTraits* create(std::istream& is);

    /**
     * @brief Builds a parameter from a json object.
     * @param j The json object, holding at least a string "type" field.
     * @return The parameter, owned by the caller.
     */
    static ParameterTraits* create(const nlohmann::json& j);

    /**
     * @brief Builds a parameter with corresponding type and default value
     * @param type The desired type for the parameter
     * @param data The default value for the parameter
     * @return The parameter, owned by the caller
     */
    static ParameterTraits* create(const std::string& type, std::any data);

    static std::vector<std::string> types();

    static bool register_creator(const std::string& type, std::function<ParameterTraits* ()> creator);

private:
    static std::map<std::string, std::function<ParameterTraits* ()>>& creators();
};

} // namespace Parameters
} // namespace OpenViBE

#define REGISTER_PARAMETER_TYPE(name, type) \
namespace { \
    static const bool type##_registered = \
    ParameterFactory::register_creator(name, []() { return new type(); }); \
} //namespace
