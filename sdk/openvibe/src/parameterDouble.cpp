/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

#include <sstream>
#include <iomanip>

#include "parameters/parameterDouble.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterDouble::ParameterDouble(void) : ParameterBase<double, ParameterDouble>()
{
}

ParameterDouble::ParameterDouble(double value) : ParameterBase<double, ParameterDouble>(value)
{
}

ParameterDouble::ParameterDouble(const std::string& name, const std::string& description, double value) : ParameterBase<double, ParameterDouble>(name, description, value)
{
}

ParameterDouble::ParameterDouble(const std::string& name, const std::string& description, double value, double min, double max) : ParameterBase<double, ParameterDouble>(name, description, value)
{
    m_min = min;
    m_max = max;
}

ParameterDouble::ParameterDouble(const std::string& name, const std::string& description, double value, double min, double max, int decimals) : ParameterBase<double, ParameterDouble>(name, description, value)
{
    m_min = min;
    m_max = max;
    m_decimals = decimals;
}

ParameterDouble::ParameterDouble(const ParameterDouble& param) : ParameterBase<double, ParameterDouble>(param)
{
    m_min = param.m_min;
    m_max = param.m_max;
}

ParameterDouble::ParameterDouble(ParameterDouble&& param) noexcept
{
    ParameterBase<double, ParameterDouble>::operator=(std::move(param));
    m_min = std::move(param.m_min);
    m_max = std::move(param.m_max);
    m_decimals = std::move(param.m_decimals);
}

ParameterDouble::~ParameterDouble(void)
{
}

ParameterDouble& ParameterDouble::operator=(ParameterDouble&& param) noexcept
{
    if (this != &param) {
        ParameterBase<double, ParameterDouble>::operator=(std::move(param));
        m_min = std::move(param.m_min);
        m_max = std::move(param.m_max);
        m_decimals = std::move(param.m_decimals);
    }
    return *this;
}

ParameterTraits* ParameterDouble::clone() const
{
    return new ParameterDouble(*this);
}

void ParameterDouble::setValue(const double& value, bool notify)
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

double ParameterDouble::min() const
{
    return m_min;
}

void ParameterDouble::setMin(double value, bool notify)
{
    m_min = value;
    if(m_max < value) {
       m_max = value;
       if(m_maxNotifier)
           m_maxNotifier(m_max);
    }

    if(this->value() < value) {
        this->setValue(value);
        this->sync(); // let the gui know
    }

    if(notify && m_minNotifier) {
        m_minNotifier(m_min);
    }
}

void ParameterDouble::setMinNotifier(std::function<void(const double&)> func)
{
    m_minNotifier = func;
}

double ParameterDouble::max() const
{
    return m_max;
}

void ParameterDouble::setMax(double value, bool notify)
{
    m_max = value;
    if(m_min > value) {
        m_min = value;
        if(m_minNotifier)
            m_minNotifier(m_min);
    }

    if(this->value() > value) {
        this->setValue(value);
        this->sync(); // let the gui know
    }

    if(notify && m_maxNotifier) {
        m_maxNotifier(m_max);
    }
}

void ParameterDouble::setMaxNotifier(std::function<void(const double&)> func)
{
    m_maxNotifier = func;
}

int ParameterDouble::decimals() const
{
    return m_decimals;
}

std::string ParameterDouble::toString() const
{
    std::ostringstream oss;
    oss << std::setprecision(12) << this->value();
    return oss.str();
}

void ParameterDouble::fromString(const std::string& value)
{
    if (!value.empty()) {
        this->setValue(std::stod(value));
    }
}

void ParameterDouble::serialize(std::ostream& os) const {
    ParameterBase<double, ParameterDouble>::serialize(os);
    os << writeField("min", std::to_string(m_min));
    os << writeField("max", std::to_string(m_max));
    os << writeField("decimals", std::to_string(m_decimals));
}

nlohmann::json ParameterDouble::serialize() const
{
    nlohmann::json j = ParameterBase<double, ParameterDouble>::serialize();
    j["min"] = m_min;
    j["max"] = m_max;
    j["decimals"] = m_decimals;
    return j;
}

void ParameterDouble::deserialize(std::istream& is) {
    ParameterBase<double, ParameterDouble>::deserialize(is);
    m_min = std::stod(readField("min", is));
    m_max = std::stod(readField("max", is));
    m_decimals = std::stoi(readField("decimals", is));
}

void ParameterDouble::deserialize(const nlohmann::json& j)
{
    //bounds first: the base sets the value through setValue(), which clamps it to them
    m_min = j.at("min").get<double>();
    m_max = j.at("max").get<double>();
    m_decimals = j.at("decimals").get<int>();
    ParameterBase<double, ParameterDouble>::deserialize(j);
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("double", ParameterDouble);

} // namespace Parameters
} // namespace OpenViBE