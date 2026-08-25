/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include <algorithm>
#include "parameters/parameterListInt.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterListInt::ParameterListInt(void) : ParameterBase<int, ParameterListInt>()
{
}

ParameterListInt::ParameterListInt(std::vector<int> availableValues) : ParameterBase<int, ParameterListInt>()
{
    m_availableValues = availableValues;
    if (!availableValues.empty()) {
        m_value = availableValues.at(0);
    }
}

ParameterListInt::ParameterListInt(const std::string& name, const std::string& description, const std::vector<int>& availableValues, int value) : ParameterBase<int, ParameterListInt>(name, description, value)
{
    m_availableValues = availableValues;
    if (!availableValues.empty()) {
        m_value = availableValues.at(0);
    }

    setValue(value);
}

ParameterListInt::ParameterListInt(const ParameterListInt& param) : ParameterBase<int, ParameterListInt>(param)
{
    m_availableValues = param.m_availableValues;
}

ParameterListInt::ParameterListInt(ParameterListInt&& param) noexcept : ParameterBase<int, ParameterListInt>(std::move(param))
{
    m_availableValues = std::move(param.m_availableValues);
}

ParameterListInt::~ParameterListInt(void)
{
}

ParameterListInt& ParameterListInt::operator=(ParameterListInt&& param) noexcept
{
    if (this != &param) {
        ParameterBase<int, ParameterListInt>::operator=(std::move(param));
        m_availableValues = std::move(param.m_availableValues);
    }
    return *this;
}

ParameterTraits* ParameterListInt::clone() const
{
    return new ParameterListInt(*this);
}

void ParameterListInt::setValue(const int& value, bool notify)
{
    if (std::find(m_availableValues.begin(), m_availableValues.end(), value) != m_availableValues.end()) {
        m_value = value;
        if (notify)
            sync();
    } else {
        std::cout << "Parameter ListInt " << this->name() << " cannot set value to " << value << std::endl;
    }
}

std::vector<int> ParameterListInt::availableValues() const
{
    return m_availableValues;
}

void ParameterListInt::setAvailableValues(const std::vector<int>& values, bool notify)
{
    m_availableValues = values;
    if (!m_availableValues.empty()
        && std::find(m_availableValues.begin(), m_availableValues.end(), m_value) == m_availableValues.end()) {
        m_value = m_availableValues.at(0);
        sync();
    }

    if (notify && m_availableValuesNotifier)
        m_availableValuesNotifier(m_availableValues);
}

void ParameterListInt::setAvailableValuesNotifier(std::function<void(const std::vector<int>&)> func)
{
    m_availableValuesNotifier = func;
}

std::string ParameterListInt::toString() const
{
    return std::to_string(this->value());
}

void ParameterListInt::fromString(const std::string& value)
{
    this->setValue(std::stoi(value));
}

void ParameterListInt::serialize(std::ostream& os) const
{
    ParameterBase<int, ParameterListInt>::serialize(os);
    os << writeField("availableValuesCount", std::to_string(m_availableValues.size()));
    for (const auto& v : m_availableValues) {
        os << writeField("availableValue", std::to_string(v));
    }
}

nlohmann::json ParameterListInt::serialize() const
{
    nlohmann::json j = ParameterBase<int, ParameterListInt>::serialize();
    j["availableValues"] = m_availableValues;
    return j;
}

void ParameterListInt::deserialize(std::istream& is)
{
    ParameterBase<int, ParameterListInt>::deserialize(is);
    size_t availableValuesCount = std::stoi(readField("availableValuesCount", is));
    for (size_t i = 0; i < availableValuesCount; i++) {
        m_availableValues.push_back(std::stoi(readField("availableValue", is)));
    }
}

void ParameterListInt::deserialize(const nlohmann::json& j)
{
    //available values first: the base sets the value through setValue() which needs to be in the list
    m_availableValues = j.at("availableValues").get<std::vector<int>>();
    ParameterBase<int, ParameterListInt>::deserialize(j);
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("intlist", ParameterListInt);

} // namespace Parameters
} // namespace OpenViBE
