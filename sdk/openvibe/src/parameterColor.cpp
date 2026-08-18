/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameters/parameterColor.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterColor::ParameterColor(void) : ParameterBase<std::string, ParameterColor>()
{
}

ParameterColor::ParameterColor(uint8_t r, uint8_t g, uint8_t b) : ParameterBase<std::string, ParameterColor>(std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b))
{
}

ParameterColor::ParameterColor(const std::string& value) : ParameterBase<std::string, ParameterColor>(value)
{
}

ParameterColor::ParameterColor(const std::string& name, const std::string& description, std::string value) : ParameterBase<std::string, ParameterColor>(name, description, value)
{
}

ParameterColor::ParameterColor(ParameterColor&& param) noexcept : ParameterBase<std::string, ParameterColor>(std::move(param))
{
}

ParameterColor::ParameterColor(const ParameterColor& param) : ParameterBase<std::string, ParameterColor>(param)
{
}

ParameterColor::~ParameterColor(void)
{
}

ParameterColor& ParameterColor::operator=(ParameterColor&& param) noexcept
{
    if (this != &param) {
        ParameterBase<std::string, ParameterColor>::operator=(std::move(param));
    }
    return *this;
}

ParameterTraits* ParameterColor::clone() const
{
    return new ParameterColor(*this);
}

std::string ParameterColor::toString() const
{
    return this->value();
}

uint8_t ParameterColor::red() const
{
    std::string value = this->value();
    size_t pos = value.find(",");
    return std::stoi(value.substr(0, pos));
}

uint8_t ParameterColor::green() const
{
    std::string value = this->value();
    size_t pos = value.find(",");
    size_t pos2 = value.find(",", pos + 1);
    return std::stoi(value.substr(pos + 1, pos2 - pos - 1));
}

uint8_t ParameterColor::blue() const
{
    std::string value = this->value();
    size_t pos = value.find(",");
    size_t pos2 = value.find(",", pos + 1);
    return std::stoi(value.substr(pos2 + 1));
}

void ParameterColor::fromString(const std::string& value)
{
    this->setValue(value);
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("color", ParameterColor);

} // namespace Parameters
} // namespace OpenViBE