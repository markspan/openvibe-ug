/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "parameters/parameterStringVector.h"
#include "parameters/parameterFactory.h"

#include <sstream>

namespace OpenViBE {
namespace Parameters {

ParameterStringVector::ParameterStringVector(void) : ParameterBase<std::vector<std::string>, ParameterStringVector>()
{
}

ParameterStringVector::ParameterStringVector(const std::vector<std::string>& value) : ParameterBase<std::vector<std::string>, ParameterStringVector>(value)
{
}

ParameterStringVector::ParameterStringVector(const std::string& name, const std::string& description, const std::vector<std::string>& value) : ParameterBase<std::vector<std::string>, ParameterStringVector>(name, description, value)
{
}

ParameterStringVector::ParameterStringVector(ParameterStringVector&& param) noexcept : ParameterBase<std::vector<std::string>, ParameterStringVector>(std::move(param))
{
}

ParameterStringVector::ParameterStringVector(const ParameterStringVector& param) : ParameterBase<std::vector<std::string>, ParameterStringVector>(param)
{
}

ParameterStringVector::~ParameterStringVector(void)
{
}

ParameterStringVector& ParameterStringVector::operator=(ParameterStringVector&& param) noexcept
{
    if (this != &param) {
        ParameterBase<std::vector<std::string>, ParameterStringVector>::operator=(std::move(param));
    }
    return *this;
}

ParameterTraits* ParameterStringVector::clone() const
{
    return new ParameterStringVector(*this);
}

void ParameterStringVector::setValue(const std::vector<std::string>& value, bool notify)
{
    m_value = value;
    if(notify)
        sync();
}

void ParameterStringVector::resize(const size_t size, bool notify)
{
    if (m_value.size() != size) {
        m_value.resize(size);
        if (notify) {
            sync();
        }
    }
}

void ParameterStringVector::setAt(size_t index, const std::string& value, bool notify)
{
    if (index < m_value.size()) {
        m_value[index] = value;
        if(notify)
            sync();
    }
}

std::string ParameterStringVector::getAt(size_t index) const
{
    if (index < m_value.size()) {
        return m_value[index];
    }
    return "";
}

std::string ParameterStringVector::toString() const
{
    if (m_value.empty()) return "";

    std::string str;
    for (const auto& s : m_value) {
        for (char c : s) {
            if (c == '\\') str += "\\\\";
            else if (c == ';') str += "\\;";
            else str += c;
        }
        str += ";";
    }
    str.pop_back();
    return str;
}

void ParameterStringVector::fromString(const std::string& value)
{
    if (value.empty()) {
        this->setValue({});
        return;
    }

    std::vector<std::string> values;
    std::string current;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            ++i;
            current += value[i];
        } else if (value[i] == ';') {
            values.push_back(current);
            current.clear();
        } else {
            current += value[i];
        }
    }
    values.push_back(current);
    this->setValue(values);
}

void ParameterStringVector::serialize(std::ostream& os) const
{
    // Base writes type/name/description/value via virtual toString() — no extra write needed.
    ParameterBase<std::vector<std::string>, ParameterStringVector>::serialize(os);
}

nlohmann::json ParameterStringVector::serialize() const
{
    nlohmann::json j;
    j["type"] = type();
    j["name"] = m_name;
    j["description"] = m_description;
    j["value"] = toString();
    return j;
}

void ParameterStringVector::deserialize(std::istream& is)
{
    // Base reads type/name/description/value and calls virtual fromString() — no extra read needed.
    ParameterBase<std::vector<std::string>, ParameterStringVector>::deserialize(is);
}

void ParameterStringVector::deserialize(const nlohmann::json& j)
{
    if (j.at("type").get<std::string>() != type()) {
        throw std::runtime_error("Type mismatch in deserialize: expected " + type() + ", got " + j.at("type").get<std::string>());
    }
    m_name = j.at("name").get<std::string>();
    m_description = j.at("description").get<std::string>();
    fromString(j.at("value").get<std::string>());
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("stringvector", ParameterStringVector);

} // namespace Parameters
} // namespace OpenViBE