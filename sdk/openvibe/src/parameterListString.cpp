/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include <algorithm>
#include "parameters/parameterListString.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterListString::ParameterListString(void) : ParameterBase<std::string, ParameterListString>()
{
}

ParameterListString::ParameterListString(std::vector<std::string> availableValues) : ParameterBase<std::string, ParameterListString>()
{
    m_availableValues = availableValues;
    if(availableValues.size() > 0) {
        m_value = availableValues.at(0);
    }
}

ParameterListString::ParameterListString(const std::string& name, const std::string& description, const std::vector<std::string>& availableValues, const std::string& value) : ParameterBase<std::string, ParameterListString>(name, description, value)
{
    m_availableValues = availableValues;
    if(availableValues.size() > 0) {
        m_value = availableValues.at(0);
    }

    if(!value.empty())
        setValue(value);
}

ParameterListString::ParameterListString(const ParameterListString& param) : ParameterBase<std::string, ParameterListString>(param)
{
    m_availableValues = param.m_availableValues;
}

ParameterListString::ParameterListString(ParameterListString&& param) noexcept : ParameterBase<std::string, ParameterListString>(std::move(param))
{
    m_availableValues = std::move(param.m_availableValues);
}

ParameterListString::~ParameterListString(void)
{
}

ParameterListString& ParameterListString::operator=(ParameterListString&& param) noexcept
{
    if (this != &param) {
        ParameterBase<std::string, ParameterListString>::operator=(std::move(param));
        m_availableValues = std::move(param.m_availableValues);
    }
    return *this;
}

ParameterTraits* ParameterListString::clone() const
{
    return new ParameterListString(*this);
}

void ParameterListString::setValue(const std::string& value, bool notify)
{
    if(std::find(m_availableValues.begin(), m_availableValues.end(), value) != m_availableValues.end()) {
        m_value = value;
        if(notify)
            sync();
    } else {
        std::cout << "Parameter ListString " << this->name() << "cannot set value to " << value << std::endl;
    }
}

std::vector<std::string> ParameterListString::availableValues() const
{
    return m_availableValues;
}

void ParameterListString::setAvailableValues(const std::vector<std::string>& values, bool notify)
{
    m_availableValues = values;
    if(std::find(m_availableValues.begin(), m_availableValues.end(), m_value) == m_availableValues.end()) {
        m_value = m_availableValues.empty() ? std::string{} : m_availableValues.at(0);
        sync();
    }

    if(notify && m_availableValuesNotifier)
        m_availableValuesNotifier(m_availableValues);
}

void ParameterListString::setAvailableValuesNotifier(std::function<void(const std::vector<std::string>&)> func)
{
    m_availableValuesNotifier = func;
}

std::string ParameterListString::toString() const
{
    return this->value();
}

void ParameterListString::fromString(const std::string& value)
{
    this->setValue(value);
}

void ParameterListString::serialize(std::ostream& os) const {
    ParameterBase<std::string, ParameterListString>::serialize(os);
    // add available values
    os << writeField("availableValuesCount", std::to_string(m_availableValues.size()));
    for(const auto& v : m_availableValues) {
        os << writeField("availableValue", v);
    }
}

nlohmann::json ParameterListString::serialize() const {
    nlohmann::json j = ParameterBase<std::string, ParameterListString>::serialize();
    j["availableValues"] = m_availableValues;
    return j;
}

void ParameterListString::deserialize(std::istream& is) {
    ParameterBase<std::string, ParameterListString>::deserialize(is);
    size_t availableValuesCount = std::stoi(readField("availableValuesCount", is));
    for(size_t i = 0; i < availableValuesCount; i++) {
        m_availableValues.push_back(readField("availableValue", is));
    }
}

void ParameterListString::deserialize(const nlohmann::json& j)
{
    ParameterBase<std::string, ParameterListString>::deserialize(j);
    m_availableValues = j.at("availableValues").get<std::vector<std::string>>();
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("stringlist", ParameterListString);

} // namespace Parameters
} // namespace OpenViBE