///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmLatencyEvaluation.hpp
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
namespace Tools {
class CBoxAlgorithmLatencyEvaluation final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override { return true; }
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_LatencyEvaluation)

    uint64_t m_StartTime         = 0;
    Kernel::ELogLevel m_LogLevel = Kernel::ELogLevel::LogLevel_None;
};

class CBoxAlgorithmLatencyEvaluationDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Latency evaluation"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Evaluates i/o jittering and outputs values to log manager"; }

    CString getDetailedDescription() const override
    {
        return "This box evaluates i/o jittering comparing input chunk' start and end time against current clock time";
    }

    CString getCategory() const override { return "Tools"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-info"; }

    CIdentifier getCreatedClass() const override { return Box_LatencyEvaluation; }
    IPluginObject* create() override { return new CBoxAlgorithmLatencyEvaluation; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("input", OV_TypeId_EBMLStream);
        prototype.addSetting("Log level to use", OV_TypeId_LogLevel, "Trace");
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_LatencyEvaluationDesc)
};
}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
