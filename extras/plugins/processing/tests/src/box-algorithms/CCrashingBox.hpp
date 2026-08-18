///-------------------------------------------------------------------------------------------------
/// 
/// \file CCrashingBox.hpp
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
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Tests {
class CCrashingBox final : public IBoxAlgorithm
{
public:
    void release() override { delete this; }

    bool initialize(Kernel::IBoxAlgorithmContext& context) override;
    bool uninitialize(Kernel::IBoxAlgorithmContext& context) override;

    bool processInput(Kernel::IBoxAlgorithmContext& context, const size_t index) override;
    bool process(Kernel::IBoxAlgorithmContext& context) override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, Box_CrashingBox)
};

class CCrashingBoxDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Crashing box"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "A box which code launches exceptions"; }
    CString getDetailedDescription() const override { return "This box illustrates the behavior of the platform given a crashing plugin code"; }
    CString getCategory() const override { return "Tests"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_CrashingBox; }
    IPluginObject* create() override { return new CCrashingBox(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("an input", CIdentifier::undefined());
        return true;
    }

    CString getStockItemName() const override { return "gtk-cancel"; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_CrashingBoxDesc)
};
}  // namespace Tests
}  // namespace Plugins
}  // namespace OpenViBE
