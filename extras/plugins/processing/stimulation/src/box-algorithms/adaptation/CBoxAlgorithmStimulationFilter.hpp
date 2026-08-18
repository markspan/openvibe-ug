///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStimulationFilter.hpp
/// \author Yann Renard (Inria).
/// \version 1.1.
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

#include "../../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {
class CBoxAlgorithmStimulationFilter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StimulationFilter)

protected:
    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer;
    Kernel::TParameterHandler<CStimulationSet*> op_stimSet;
    Kernel::TParameterHandler<CStimulationSet*> ip_stimSet;
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer;

    typedef struct SRule
    {
        uint64_t action;
        uint64_t startStimID;
        uint64_t EndStimID;
    } rule_t;

    uint64_t m_defaultAction = 0;
    uint64_t m_startTime     = 0;
    uint64_t m_endTime       = 0;
    std::vector<rule_t> m_rules;
};

class CBoxAlgorithmStimulationFilterListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingAdded(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(index);
        //we had a whole rule (3 settings)
        box.addSetting("Action to perform", TypeId_StimulationFilterAction, "Select");
        box.addSetting("Stimulation range begin", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        box.addSetting("Stimulation range end", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_0F");

        return true;
    }

    bool onSettingRemoved(Kernel::IBox& box, const size_t index) override
    {
        //we must remove the 2 other settings corresponding to the rule
        const size_t settingGroupIdx = (index - 3) / 3;
        box.removeSetting(settingGroupIdx * 3 + 3);
        box.removeSetting(settingGroupIdx * 3 + 3);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmStimulationFilterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stimulation Filter"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Filters incoming stimulations selecting or rejecting specific ranges of stimulations"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stimulation/Adaptation"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-missing-image"; }

    CIdentifier getCreatedClass() const override { return Box_StimulationFilter; }
    IPluginObject* create() override { return new CBoxAlgorithmStimulationFilter; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addOutput("Modified Stimulations", OV_TypeId_Stimulations);

        prototype.addSetting("Default action", TypeId_StimulationFilterAction, "Reject");
        prototype.addSetting("Time range begin", OV_TypeId_Float, "0");
        prototype.addSetting("Time range end", OV_TypeId_Float, "0");
        prototype.addSetting("Action to perform", TypeId_StimulationFilterAction, "Select");
        prototype.addSetting("Stimulation range begin", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Stimulation range end", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_0F");

        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
        // prototype.addFlag   (Box_FlagIsUnstable);
        return true;
    }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmStimulationFilterListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StimulationFilterDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
