/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "../ov_defines.h"
#include "parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterColor : public ParameterBase<std::string, ParameterColor>
{
public:
    ParameterColor(void);
    ParameterColor(const std::string& value);
    ParameterColor(uint8_t r, uint8_t g, uint8_t b);
    ParameterColor(const std::string& name, const std::string& description, std::string value);
    ParameterColor(ParameterColor&& param) noexcept;
    ParameterColor(const ParameterColor& param);
    ~ParameterColor(void);

    ParameterColor& operator=(ParameterColor&& param) noexcept;

    ParameterTraits* clone() const override;

    std::string type() const override { return "color"; }

    std::string toString() const override;
    uint8_t red() const;
    uint8_t green() const;
    uint8_t blue() const;
    void fromString(const std::string& value) override;

};

} // namespace Parameters
} // namespace OpenViBE