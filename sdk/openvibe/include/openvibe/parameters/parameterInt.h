/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include <limits.h>

#include "../ov_defines.h"
#include "parameterTraits.h"

class ParameterIntObject;

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterInt : public ParameterBase<int, ParameterInt>
{
public:
    ParameterInt(void);
    ParameterInt(int value);
    ParameterInt(const std::string& name, const std::string& description, int value);
    ParameterInt(const std::string& name, const std::string& description, int value, int min, int max);
    ParameterInt(ParameterInt&& param) noexcept;
    ParameterInt(const ParameterInt& param);
    ~ParameterInt(void);

    ParameterInt& operator=(ParameterInt&& param) noexcept;
    using ParameterBase<int, ParameterInt>::operator=;

    ParameterTraits* clone() const override;

    void setValue(const int& value, bool notify=true) override;

    int min() const;
    void setMin(int value, bool notify=true);
    void setMinNotifier(std::function<void(const int&)> func);

    int max() const;
    void setMax(int value, bool notify=true);
    void setMaxNotifier(std::function<void(const int&)> func);

    std::string type() const override { return "int"; }

    std::string toString() const override;
    void fromString(const std::string& value) override;

    void serialize(std::ostream& os) const override;
    nlohmann::json serialize() const override;
    void deserialize(std::istream& is) override;
    void deserialize(const nlohmann::json& j) override;

protected:
    int m_min = INT_MIN;
    std::function<void(const int&)> m_minNotifier = nullptr;
    int m_max = INT_MAX;
    std::function<void(const int&)> m_maxNotifier = nullptr;
};

} // namespace Parameters
} // namespace OpenViBE