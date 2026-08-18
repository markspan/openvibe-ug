/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameters/parameterString.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterString::ParameterString(void) : ParameterBase<std::string, ParameterString>()
{
}

ParameterString::ParameterString(const std::string& value) : ParameterBase<std::string, ParameterString>(value)
{
}

ParameterString::ParameterString(const std::string& name, const std::string& description, std::string value) : ParameterBase<std::string, ParameterString>(name, description, value)
{
}

ParameterString::ParameterString(const ParameterString& param) : ParameterBase<std::string, ParameterString>(param)
{
}

ParameterString::ParameterString(ParameterString&& param) noexcept : ParameterBase<std::string, ParameterString>(std::move(param))
{
}

ParameterString::~ParameterString(void)
{
}

ParameterString& ParameterString::operator=(ParameterString&& param) noexcept
{
    if (this != &param) {
        ParameterBase<std::string, ParameterString>::operator=(std::move(param));
    }
    return *this;
}

ParameterTraits* ParameterString::clone() const
{
    return new ParameterString(*this);
}

std::string ParameterString::toString() const
{
    return this->value();
}

void ParameterString::fromString(const std::string& value)
{
    this->setValue(value);
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("string", ParameterString);

} // namespace Parameters
} // namespace OpenViBE