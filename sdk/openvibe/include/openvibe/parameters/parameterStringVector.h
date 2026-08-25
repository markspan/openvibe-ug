/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterStringVector : public ParameterBase<std::vector<std::string>, ParameterStringVector>
{
    public:
        ParameterStringVector(void);
        ParameterStringVector(const std::vector<std::string>& value);
        ParameterStringVector(const std::string& name, const std::string& description, const std::vector<std::string>& value);
        ParameterStringVector(ParameterStringVector&& param) noexcept;
        ParameterStringVector(const ParameterStringVector& param);
        ~ParameterStringVector(void);

        ParameterStringVector& operator=(ParameterStringVector&& param) noexcept;

        ParameterTraits* clone() const override;

        void setValue(const std::vector<std::string>& value, bool notify=true) override;
        void resize(const size_t size, bool notify=true);
        void setAt(size_t index, const std::string& value, bool notify=true);
        std::string getAt(size_t index) const;

        std::string type() const override { return "stringvector"; }

        std::string toString() const override;
        void fromString(const std::string& value) override;


};

} // namespace Parameters 
} // namespace OpenViBE