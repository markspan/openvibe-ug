/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 
#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "../ov_defines.h"
#include "parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

enum OpenMode
{
    Read,
    Write,
    ReadWrite,
    Append
};

class OV_API ParameterPath : public ParameterBase<std::string, ParameterPath>
{
public:
    ParameterPath(void);
    ParameterPath(const std::filesystem::path& path);
    ParameterPath(const std::string& path, const OpenMode mode = Read, const std::vector<std::string>& filters = {});
    ParameterPath(const std::string& name, const std::string& description, const std::string& value, const OpenMode mode = Read, const std::vector<std::string>& filters = {});
    ParameterPath(ParameterPath&& param) noexcept;
    ParameterPath(const ParameterPath& param);
    ~ParameterPath(void);

    ParameterPath& operator=(ParameterPath&& param) noexcept;

    ParameterTraits* clone() const override;

    std::string type() const override { return "path"; }

    OpenMode mode() const;
    void setMode(const OpenMode mode);

    std::vector<std::string> filters() const;
    void setFilters(const std::vector<std::string>& filters);

    std::string toString() const override;
    void fromString(const std::string& value) override;

    void serialize(std::ostream& os) const override;
    nlohmann::json serialize() const override;
    void deserialize(std::istream& is) override;
    void deserialize(const nlohmann::json& j) override;

protected:
    OpenMode m_mode = Read;
    std::vector<std::string> m_filters;
};

} // namespace Parameters
} // namespace OpenViBE