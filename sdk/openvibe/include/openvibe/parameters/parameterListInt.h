/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "../ov_defines.h"
#include "parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterListInt : public ParameterBase<int, ParameterListInt>
{
public:
    ParameterListInt(void);
    ParameterListInt(std::vector<int> availableValues);
    ParameterListInt(const std::string& name, const std::string& description, const std::vector<int>& availableValues, int value = 0);
    ParameterListInt(ParameterListInt&& param) noexcept;
    ParameterListInt(const ParameterListInt& param);
    ~ParameterListInt(void);

    ParameterListInt& operator=(ParameterListInt&& param) noexcept;

    ParameterTraits* clone() const override;

    void setValue(const int& value, bool notify=true) override;

    std::vector<int> availableValues() const;
    void setAvailableValues(const std::vector<int>& values, bool notify=true);
    void setAvailableValuesNotifier(std::function<void(const std::vector<int>&)> func);

    std::string type() const override { return "intlist"; }

    std::string toString() const override;
    void fromString(const std::string& value) override;

    void serialize(std::ostream& os) const override;
    nlohmann::json serialize() const override;
    void deserialize(std::istream& is) override;
    void deserialize(const nlohmann::json& j) override;

protected:
    std::vector<int> m_availableValues;
    std::function<void(const std::vector<int>&)> m_availableValuesNotifier = nullptr;
};

} // namespace Parameters
} // namespace OpenViBE
