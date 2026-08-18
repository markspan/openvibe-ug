/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterTraits.h"
#include "../ov_defines.h"

namespace OpenViBE {
namespace Parameters {

/**
 * @brief Selection of several values among a fixed set of available values.
 *
 * Where ParameterListString selects exactly one value out of its available values,
 * this one selects any number of them. The string form is the selected values joined
 * by OV_Value_EnumeratedStringSeparator (';'), which is also the composition format
 * the kernel type manager uses for bitmask types.
 */
class OV_API ParameterMultiSelect : public ParameterBase<std::vector<std::string>, ParameterMultiSelect>
{
public:
    ParameterMultiSelect(void);
    ParameterMultiSelect(const std::vector<std::string>& availableValues);
    ParameterMultiSelect(const std::string& name, const std::string& description, const std::vector<std::string>& availableValues, const std::vector<std::string>& value = {});
    ParameterMultiSelect(ParameterMultiSelect&& param) noexcept;
    ParameterMultiSelect(const ParameterMultiSelect& param);
    ~ParameterMultiSelect(void);

    ParameterMultiSelect& operator=(ParameterMultiSelect&& param) noexcept;

    ParameterTraits* clone() const override;

    void setValue(const std::vector<std::string>& value, bool notify=true) override;

    std::vector<std::string> availableValues() const;
    /// @warning Resets the current selection. Do not bind this from the UI.
    void setAvailableValues(const std::vector<std::string>& values, bool notify=true);
    void setAvailableValuesNotifier(std::function<void(const std::vector<std::string>&)> func);

    std::string type() const override { return "multiselect"; }

    //return or set selected values, it will not return the available values
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
