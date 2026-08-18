/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "../ov_defines.h"
#include "parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterString : public ParameterBase<std::string, ParameterString>
{
public:
    ParameterString(void);
    ParameterString(const std::string& value);
    ParameterString(const std::string& name, const std::string& description, std::string value);
    ParameterString(ParameterString&& param) noexcept;
    ParameterString(const ParameterString& param);
    ~ParameterString(void);

    ParameterString& operator=(ParameterString&& param) noexcept;

    ParameterTraits* clone() const override;

    std::string type() const override { return "string"; }

    std::string toString() const override;
    void fromString(const std::string& value) override;

};

} // namespace Parameters
} // namespace OpenViBE