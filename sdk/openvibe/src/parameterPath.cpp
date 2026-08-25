/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameters/parameterPath.h"
#include "parameters/parameterFactory.h"

namespace OpenViBE {
namespace Parameters {

ParameterPath::ParameterPath(void) : ParameterBase<std::string, ParameterPath>()
{
}

ParameterPath::ParameterPath(const std::filesystem::path& path) : ParameterBase<std::string, ParameterPath>(path.string()), m_mode(Read)
{
}

ParameterPath::ParameterPath(const std::string& value, const OpenMode mode, const std::vector<std::string>& filters) : 
  ParameterBase<std::string, ParameterPath >(value), 
  m_mode(mode),
  m_filters(filters)
{
}

ParameterPath::ParameterPath(const std::string& name, const std::string& description, const std::string& value, const OpenMode mode, const std::vector<std::string>& filters) : 
  ParameterBase<std::string, ParameterPath>(name, description, value), 
  m_mode(mode),
  m_filters(filters)
{
}

ParameterPath::ParameterPath(const ParameterPath& param) : ParameterBase<std::string, ParameterPath>(param)
{
}

ParameterPath::ParameterPath(ParameterPath&& param) noexcept : ParameterBase<std::string, ParameterPath>(std::move(param))
{
    m_mode = std::move(param.m_mode);
    m_filters = std::move(param.m_filters);
}

ParameterPath::~ParameterPath(void)
{
}

ParameterPath& ParameterPath::operator=(ParameterPath&& param) noexcept
{
    if (this != &param) {
        ParameterBase<std::string, ParameterPath>::operator=(std::move(param));
        m_mode = std::move(param.m_mode);
        m_filters = std::move(param.m_filters);
    }
    return *this;
}

ParameterTraits* ParameterPath::clone() const
{
    return new ParameterPath(*this);
}

OpenMode ParameterPath::mode() const
{
    return m_mode;
}

void ParameterPath::setMode(const OpenMode mode)
{
    m_mode = mode;
}

std::vector<std::string> ParameterPath::filters() const
{
    return m_filters;
}

void ParameterPath::setFilters(const std::vector<std::string>& filters)
{
    m_filters = filters;
}


std::string ParameterPath::toString() const
{
    return this->value();
}

void ParameterPath::fromString(const std::string& value)
{
    this->setValue(value);
}

void ParameterPath::serialize(std::ostream& os) const {
    ParameterBase<std::string, ParameterPath>::serialize(os);
    os << writeField("mode", std::to_string(static_cast<int>(m_mode)));
    os << writeField("filtersCount", std::to_string(m_filters.size()));
    for(const auto& f : m_filters) {
        os << writeField("filter", f);
    }
}

nlohmann::json ParameterPath::serialize() const
{
    nlohmann::json jsonRes;
    jsonRes["type"] = type();
    jsonRes["name"] = name();
    jsonRes["description"] = description();
    jsonRes["value"] = value();
    jsonRes["mode"] = static_cast<int>(m_mode);
    jsonRes["filters"] = m_filters;
    return jsonRes;
}

void ParameterPath::deserialize(std::istream& is) {
    ParameterBase<std::string, ParameterPath>::deserialize(is);
    int mode = std::stoi(readField("mode", is));
    m_mode = static_cast<OpenMode>(mode);
    size_t filtersCount = std::stoi(readField("filtersCount", is));
    for(size_t i = 0; i < filtersCount; i++) {
        m_filters.push_back(readField("filter", is));
    }
}

void ParameterPath::deserialize(const nlohmann::json& j)
{
    m_mode = static_cast<OpenMode>(j.at("mode").get<int>());
    m_filters = j.at("filters").get<std::vector<std::string>>();
    ParameterBase<std::string, ParameterPath>::deserialize(j);
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("path", ParameterPath);

} // namespace Parameters
} // namespace OpenViBE
