/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include <algorithm>

#include <nlohmann/json.hpp>

#include "parameters/parameterBool.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterBool::ParameterBool(void) : ParameterBase<bool, ParameterBool>()
{
    this->setValue(false);   // by default
}

ParameterBool::ParameterBool(bool value) : ParameterBase<bool, ParameterBool>(value)
{
}

ParameterBool::ParameterBool(const std::string& name, const std::string& description, bool value) : ParameterBase<bool, ParameterBool>(name, description, value)
{
}

ParameterBool::ParameterBool(const ParameterBool& param) : ParameterBase<bool, ParameterBool>(param)
{
}

ParameterBool::ParameterBool(ParameterBool&& param) noexcept : ParameterBase<bool, ParameterBool>(std::move(param))
{
}

ParameterBool& ParameterBool::operator=(ParameterBool&& param) noexcept 
{
    if (this != &param) {
        ParameterBase<bool, ParameterBool>::operator=(std::move(param));
    }
    return *this;
}

ParameterBool::~ParameterBool(void)
{
}

ParameterTraits* ParameterBool::clone() const
{
    return new ParameterBool(*this);
}

std::string ParameterBool::toString() const
{
    return this->value() ? "true" : "false";
}

void ParameterBool::fromString(const std::string& value)
{   
    std::string data;
    data.resize(value.size());
    std::transform(value.begin(), value.end(), data.begin(),
        [](unsigned char c){ return std::tolower(c); });
    this->setValue(data == "true");
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("bool", ParameterBool);

} // namespace Parameters
} // namespace OpenViBE
