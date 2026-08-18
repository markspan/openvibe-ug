/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "../ov_defines.h"
#include "parameterTraits.h"

#include <optional>
#include <vector>
#include <array>

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterColorGradient : public ParameterBase<std::string, ParameterColorGradient>
{
public:
    ParameterColorGradient(void);
    ParameterColorGradient(const std::string& value);
    ParameterColorGradient(const std::string& name, const std::string& description, std::string value);
    ParameterColorGradient(ParameterColorGradient&& param) noexcept;
    ParameterColorGradient(const ParameterColorGradient& param);
    ~ParameterColorGradient(void);

    ParameterColorGradient& operator=(ParameterColorGradient&& param) noexcept;

    ParameterTraits* clone() const override;

    std::string type() const override { return "colorgradient"; }

    std::string toString() const override;
    void fromString(const std::string& value) override;

    // value will be in the format `%:r,g,b ; %:r,g,b` for example: 0:2,36,58; 50:100,100,100; 100:83,17,20
    void setValue(const std::string& value, bool notify=true) override;    

    //interpolate the color gradient for a given number of steps
    //color is r,g,b with range 0-255
    std::vector<std::array<int, 4> > interpolate(int steps) const;

    static std::optional<std::vector<std::array<int, 4>>> validateValue(const std::string& value);

protected:
    std::vector<std::array<int, 4> > m_colors; // Store colors as {percentage, r, g, b}
};

} // namespace Parameters
} // namespace OpenViBE