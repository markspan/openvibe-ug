/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 
#pragma once

#include "../ov_defines.h"
#include "parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterBool : public ParameterBase<bool, ParameterBool>
{
public:
    ParameterBool(void);
    ParameterBool(bool value);
    ParameterBool(const std::string& name, const std::string& description, bool value);
    ParameterBool(const ParameterBool& param);
    ParameterBool(ParameterBool&& param) noexcept;
    ~ParameterBool(void);

    ParameterBool& operator=(ParameterBool&& param) noexcept;
    using ParameterBase<bool, ParameterBool>::operator=;

    ParameterTraits* clone() const override;

    std::string type() const override { return "bool"; }

    std::string toString() const override;
    void fromString(const std::string& value) override;
};

} // namespace Parameters
} // namespace OpenViBE