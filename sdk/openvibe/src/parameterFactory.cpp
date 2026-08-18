/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: LGPL-3.0-only
*/ 

#include <stdexcept>

#include <nlohmann/json.hpp>

#include "parameters/parameterFactory.h"
#include "parameters/parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

std::map<std::string, std::function<ParameterTraits* ()>>& ParameterFactory::creators() {
    static std::map<std::string, std::function<ParameterTraits* ()>> map;
    return map;
}

ParameterTraits* ParameterFactory::create(const std::string& type) {
    ParameterTraits* res = nullptr;
    auto it = creators().find(type);

    if (it == creators().end()) {
        throw std::runtime_error("Unknown type: " + type);
    }
    else {
        res = it->second();
    }
    return res;
}

ParameterTraits* ParameterFactory::create(std::istream& is) {
    auto pos = is.tellg();
    std::string type = ParameterTraits::readField("type", is);
    is.seekg(pos);

    ParameterTraits* param = ParameterFactory::create(type);
    if (param) {
        param->deserialize(is);
    }
    return param;
}

ParameterTraits* ParameterFactory::create(const std::string& type, std::any data) {
    ParameterTraits* param = ParameterFactory::create(type);
    if (param) {
        param->setValue(data);
    }
    return param;
}

ParameterTraits* ParameterFactory::create(nlohmann::json& j) {
    ParameterTraits* res = nullptr;

    if (!j.contains("type") || !j["type"].is_string()) {
        throw std::runtime_error("JSON object must contain a string field 'type'");
    }
    else {
        std::string type = j["type"];
        auto it = creators().find(type);
        if (it == creators().end()) {
            throw std::runtime_error("Unknown type: " + type);
        }
        else {
            res = it->second();
            res->deserialize(j);
        }
    }
    return res;
}

std::vector<std::string> ParameterFactory::types() {
    std::vector<std::string> typesList;
    for (const auto& pair : creators()) {
        typesList.push_back(pair.first);
    }
    return typesList;
}

bool ParameterFactory::register_creator(const std::string& type, std::function<ParameterTraits* ()> creator) {
    bool registered = false;

    if (creators().find(type) == creators().end()) {
        creators()[type] = creator;
        registered = true;
    }
    return registered;
}

} // namespace Parameters
} // namespace OpenViBE