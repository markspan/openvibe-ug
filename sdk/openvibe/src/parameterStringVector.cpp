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

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("stringvector", ParameterStringVector);

} // namespace Parameters
} // namespace OpenViBE
