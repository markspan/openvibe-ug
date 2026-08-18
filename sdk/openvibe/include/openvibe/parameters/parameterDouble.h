/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include <float.h>

#include "../ov_defines.h"
#include "parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterDouble : public ParameterBase<double, ParameterDouble>
{
public:
    ParameterDouble(void);
    ParameterDouble(double value);
    ParameterDouble(const std::string& name, const std::string& description, double value);
    ParameterDouble(const std::string& name, const std::string& description, double value, double min, double max);
    ParameterDouble(const std::string& name, const std::string& description, double value, double min, double max, int decimals);
    ParameterDouble(ParameterDouble&& param) noexcept;
    ParameterDouble(const ParameterDouble& param);
    ~ParameterDouble(void);

    ParameterDouble& operator=(ParameterDouble&& param) noexcept;
    using ParameterBase<double, ParameterDouble>::operator=;

    ParameterTraits* clone() const override;

    void setValue(const double& value, bool notify=true) override;

    double min() const;
    void setMin(double value, bool notify=true);
    void setMinNotifier(std::function<void(const double&)> func);

    double max() const;
    void setMax(double value, bool notify=true);
    void setMaxNotifier(std::function<void(const double&)> func);

    int decimals() const;

    std::string type() const override { return "double"; }

    std::string toString() const override;
    void fromString(const std::string& value) override;

    void serialize(std::ostream& os) const override;
    nlohmann::json serialize() const override;
    void deserialize(std::istream& is) override;
    void deserialize(const nlohmann::json& j) override;

protected:
    double m_min = -DBL_MAX;
    std::function<void(const double&)> m_minNotifier = nullptr;
    double m_max = DBL_MAX;
    std::function<void(const double&)> m_maxNotifier = nullptr;
    int m_decimals = 2;
};

} // namespace Parameters
} // namespace OpenViBE