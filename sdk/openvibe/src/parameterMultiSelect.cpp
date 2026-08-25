/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include <algorithm>
#include <sstream>
#include "parameters/parameterMultiSelect.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterMultiSelect::ParameterMultiSelect(void) : ParameterBase<std::vector<std::string>, ParameterMultiSelect>()
{
}

ParameterMultiSelect::ParameterMultiSelect(const std::vector<std::string>& availableValues) : ParameterBase<std::vector<std::string>, ParameterMultiSelect>()
{
    m_availableValues = availableValues;
}

ParameterMultiSelect::ParameterMultiSelect(const std::string& name, const std::string& description, const std::vector<std::string>& availableValues, const std::vector<std::string>& value) : ParameterBase<std::vector<std::string>, ParameterMultiSelect>(name, description, value)
{
    m_availableValues = availableValues;
    if(!value.empty()) {
        setValue(value);
    }
}

ParameterMultiSelect::ParameterMultiSelect(const ParameterMultiSelect& param) : ParameterBase<std::vector<std::string>, ParameterMultiSelect>(param)
{
    m_availableValues = param.m_availableValues;
}

ParameterMultiSelect::ParameterMultiSelect(ParameterMultiSelect&& param) noexcept : ParameterBase(std::move(param))
{
    m_availableValues = std::move(param.m_availableValues);
}

ParameterMultiSelect::~ParameterMultiSelect(void)
{
}

ParameterMultiSelect& ParameterMultiSelect::operator=(ParameterMultiSelect&& param) noexcept
{
    if (this != &param) {
        ParameterTraits::operator=(std::move(param));
        m_availableValues = std::move(param.m_availableValues);
    }
    return *this;
}

ParameterTraits* ParameterMultiSelect::clone() const
{
    return new ParameterMultiSelect(*this);
}

void ParameterMultiSelect::setValue(const std::vector<std::string>& value, bool notify)
{
    m_value.clear();
    for(const auto& v : value) {
        if(std::find(m_availableValues.begin(), m_availableValues.end(), v) != m_availableValues.end()) {
            m_value.push_back(v);
        } else {
            std::cout << "Parameter MultiSelect " << this->name() << "cannot set value to " << v << std::endl;
        }
    }

    //will always notify as we may have just removed all values
    if(notify)
        sync();
}

std::vector<std::string> ParameterMultiSelect::availableValues() const
{
    return m_availableValues;
}

void ParameterMultiSelect::setAvailableValues(const std::vector<std::string>& values, bool notify)
{
    m_availableValues = values;
    m_value.clear();
    sync();

    if(notify && m_availableValuesNotifier)
        m_availableValuesNotifier(m_availableValues);
}

void ParameterMultiSelect::setAvailableValuesNotifier(std::function<void(const std::vector<std::string>&)> func)
{
    m_availableValuesNotifier = func;
}

std::string ParameterMultiSelect::toString() const
{
    // An empty selection is a legitimate state, so the separator has to be written
    // between values rather than trimmed off the end afterwards.
    std::stringstream ss;
    bool first = true;
    for(const auto& s : this->value()) {
        if(!first) {
            ss << ';';
        }
        ss << s;
        first = false;
    }

    return ss.str();
}

void ParameterMultiSelect::fromString(const std::string& value)
{
    std::vector<std::string> values;
    std::string tmp;
    std::istringstream tokenStream(value);
    while (std::getline(tokenStream, tmp, ';')) {
        values.push_back(tmp);
    }

    this->setValue(values);
}

void ParameterMultiSelect::serialize(std::ostream& os) const {
    ParameterBase<std::vector<std::string>, ParameterMultiSelect>::serialize(os);
    // add available values
    os << writeField("availableValuesCount", std::to_string(m_availableValues.size()));
    for(const auto& v : m_availableValues) {
        os << writeField("availableValue", v);
    }
    os << writeField("selectedValuesCount", std::to_string(m_value.size()));
    for(const auto& v : m_value) {
        os << writeField("selectedValue", v);
    }
}

nlohmann::json ParameterMultiSelect::serialize() const
{
    nlohmann::json jsonRes = ParameterBase<std::vector<std::string>, ParameterMultiSelect>::serialize();

    jsonRes["availableValues"] = m_availableValues;
    jsonRes["selectedValues"] = m_value;

    return jsonRes;
}

void ParameterMultiSelect::deserialize(std::istream& is) {
    ParameterBase<std::vector<std::string>, ParameterMultiSelect>::deserialize(is);
    size_t availableValuesCount = std::stoi(readField("availableValuesCount", is));
    for(size_t i = 0; i < availableValuesCount; i++) {
        m_availableValues.push_back(readField("availableValue", is));
    }
    size_t selectedValuesCount = std::stoi(readField("selectedValuesCount", is));
    for(size_t i = 0; i < selectedValuesCount; i++) {
        m_value.push_back(readField("selectedValue", is));
    }
}

void ParameterMultiSelect::deserialize(const nlohmann::json& j) {
    //available values first: the base sets the value through setValue(), which needs to be in the list
    m_availableValues = j.at("availableValues").get<std::vector<std::string>>();
    ParameterBase<std::vector<std::string>, ParameterMultiSelect>::deserialize(j);
}

// Register the parameter in the parameter factory
REGISTER_PARAMETER_TYPE("multiselect", ParameterMultiSelect);

} // namespace Parameters
} // namespace OpenViBE
