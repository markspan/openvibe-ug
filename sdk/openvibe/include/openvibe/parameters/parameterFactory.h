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
    // Return raw pointers for clear ownership transfer
    static ParameterTraits* create(const std::string& type);
    static ParameterTraits* create(std::istream& is);
    static ParameterTraits* create(nlohmann::json& j);
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
