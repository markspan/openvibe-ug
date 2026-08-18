/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "../ov_defines.h"
#include "parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterListString : public ParameterBase<std::string, ParameterListString>
{
public:
    ParameterListString(void);
    ParameterListString(std::vector<std::string> availableValues);
    ParameterListString(const std::string& name, const std::string& description, const std::vector<std::string>& availableValues, const std::string& value = "");
    ParameterListString(ParameterListString&& param) noexcept;
    ParameterListString(const ParameterListString& param);
    ~ParameterListString(void);

    ParameterListString& operator=(ParameterListString&& param) noexcept;

    ParameterTraits* clone() const override;

    void setValue(const std::string& value, bool notify=true) override;

    std::vector<std::string> availableValues() const;
    void setAvailableValues(const std::vector<std::string>& values, bool notify=true);
    void setAvailableValuesNotifier(std::function<void(const std::vector<std::string>&)> func);

    std::string type() const override { return "stringlist"; }

    std::string toString() const override;
    void fromString(const std::string& value) override;

    void serialize(std::ostream& os) const override;
    nlohmann::json serialize() const override;
    void deserialize(std::istream& is) override;
    void deserialize(const nlohmann::json& j) override;

protected:
    std::vector<std::string> m_availableValues;
    std::function<void(const std::vector<std::string>&)> m_availableValuesNotifier = nullptr;
};

} // namespace Parameters
} // namespace OpenViBE
