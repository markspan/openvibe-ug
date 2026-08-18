///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmAdditionTest.hpp
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
class CBoxAlgorithmAdditionTest final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmAdditionTest() {}

    void release() override { delete this; }

    uint64_t getClockFrequency() override { return uint64_t(1LL) << 36; }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_BoxAlgorithmAdditionTest)

protected:
    Kernel::ELogLevel m_logLevel = Kernel::LogLevel_None;

    int64_t m_i1 = 0;
    int64_t m_i2 = 0;
    int64_t m_i3 = 0;
    int64_t m_i4 = 0;

    Kernel::IAlgorithmProxy* m_proxy1 = nullptr;
    Kernel::IAlgorithmProxy* m_proxy2 = nullptr;
    Kernel::IAlgorithmProxy* m_proxy3 = nullptr;
};

class CBoxAlgorithmAdditionTestDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Addition Test"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "This box illustrates how an algorithm can be used in a box"; }
    CString getDetailedDescription() const override
    {
        return "This specific sample computes 4 random numbers and uses 3 sum operator algorithms in order to get the total";
    }
    CString getCategory() const override { return "Tests"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_BoxAlgorithmAdditionTest; }
    IPluginObject* create() override { return new CBoxAlgorithmAdditionTest(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Log level to use", OV_TypeId_LogLevel, "Information");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_BoxAlgorithmAdditionTestDesc)
};
}  // namespace Tests
}  // namespace Plugins
}  // namespace OpenViBE
