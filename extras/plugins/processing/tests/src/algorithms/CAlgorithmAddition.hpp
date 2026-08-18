///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmAddition.hpp
/// \author Yann Renard (Inria)
/// \version 1.0.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace Tests {
class CAlgorithmAddition final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, Box_AlgorithmAddition)

protected:
    Kernel::TParameterHandler<int64_t> m_parameter1;
    Kernel::TParameterHandler<int64_t> m_parameter2;
    Kernel::TParameterHandler<int64_t> m_parameter3;
};

class CAlgorithmAdditionDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Addition"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Computes and outputs the sum of two inputs"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Tests"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_AlgorithmAddition; }
    IPluginObject* create() override { return new CAlgorithmAddition(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(CIdentifier(0, 1), "First addition operand", Kernel::ParameterType_Integer);
        prototype.addInputParameter(CIdentifier(0, 2), "Second addition operand", Kernel::ParameterType_Integer);
        prototype.addOutputParameter(CIdentifier(0, 3), "Addition result", Kernel::ParameterType_Integer);

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, Box_AlgorithmAdditionDesc)
};
}  // namespace Tests
}  // namespace Plugins
}  // namespace OpenViBE
