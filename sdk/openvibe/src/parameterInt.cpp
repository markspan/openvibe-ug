/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameters/parameterInt.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterInt::ParameterInt(void) : ParameterBase<int, ParameterInt>()
{
}

ParameterInt::ParameterInt(int value) : ParameterBase<int, ParameterInt>(value)
{
}

ParameterInt::ParameterInt(const std::string& name, const std::string& description, int value) : ParameterBase<int, ParameterInt>(name, description, value)
{
}

ParameterInt::ParameterInt(const std::string& name, const std::string& description, int value, int min, int max) : ParameterBase<int, ParameterInt>(name, description, value)
{
    m_min = min;
    m_max = max;
}

ParameterInt::ParameterInt(const ParameterInt& param) : ParameterBase<int, ParameterInt>(param)
{
    m_min = param.m_min;
    m_max = param.m_max;
}

ParameterInt::ParameterInt(ParameterInt&& param) noexcept : ParameterBase<int, ParameterInt>(std::move(param))
{
    m_min = std::move(param.m_min);
    m_max = std::move(param.m_max);
}

ParameterInt::~ParameterInt(void)
{
}

ParameterInt& ParameterInt::operator=(ParameterInt&& param) noexcept 
{
    if (this != &param) {
        ParameterBase<int, ParameterInt>::operator=(std::move(param));
        m_min = std::move(param.m_min);
        m_max = std::move(param.m_max);
    }
    return *this;
}

ParameterTraits* ParameterInt::clone() const
{
    return new ParameterInt(*this);
}

void ParameterInt::setValue(const int& value, bool notify)
{
    if(value < m_min) {
        m_value = m_min;
        sync(); // forece sync here to notify the gui that it's a new value
    }
    else if(value > m_max) {
        m_value = m_max;
        sync();
    }
    else {
        m_value = value;
        if(notify)
            sync();
    }
}

int ParameterInt::min() const
{
    return m_min;
}

void ParameterInt::setMin(int value, bool notify)
{
    m_min = value;
    if(m_max < value) {
       m_max = value;
        if(m_maxNotifier) {
            m_maxNotifier(m_max);
        }
    }

    if(this->value() < value)
        this->setValue(value);

    if(notify && m_minNotifier) {
        m_minNotifier(m_min);
    }
}

void ParameterInt::setMinNotifier(std::function<void(const int&)> func)
{
    m_minNotifier = func;
}

int ParameterInt::max() const
{
    return m_max;
}

void ParameterInt::setMax(int value, bool notify)
{
    m_max = value;
    if(m_min > value) {
       m_min = value;
        if(m_minNotifier) {
            m_minNotifier(m_min);
        }
    }

    if(this->value() > value)
        this->setValue(value);

    if(notify && m_maxNotifier) {      
        m_maxNotifier(m_max);
    }
}

void ParameterInt::setMaxNotifier(std::function<void(const int&)> func)
{
    m_maxNotifier = func;
}

std::string ParameterInt::toString() const
{
    return std::to_string(this->value());
}

void ParameterInt::fromString(const std::string& value)
{
    if (!value.empty()) {
        this->setValue(std::stoi(value));
    }
}

void ParameterInt::serialize(std::ostream& os) const {
    ParameterBase<int, ParameterInt>::serialize(os);
    os << writeField("min", std::to_string(m_min));
    os << writeField("max", std::to_string(m_max));
}

nlohmann::json ParameterInt::serialize() const {
    nlohmann::json j = ParameterBase<int, ParameterInt>::serialize();
    j["min"] = m_min;
    j["max"] = m_max;
    return j;
}

void ParameterInt::deserialize(std::istream& is) {
    ParameterBase<int, ParameterInt>::deserialize(is);
    m_min = std::stod(readField("min", is));
    m_max = std::stod(readField("max", is));
}

void ParameterInt::deserialize(const nlohmann::json& j)
{
    ParameterBase<int, ParameterInt>::deserialize(j);
    m_min = j.at("min").get<int>();
    m_max = j.at("max").get<int>();
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("int", ParameterInt);

} // namespace Parameters
} // namespace OpenViBE