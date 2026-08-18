///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmPlayerController.hpp
/// \brief Classes for the Box Player Controller.
/// \author Yann Renard (Inria).
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
namespace Stimulation {
class CBoxAlgorithmPlayerController final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_PlayerController)

protected:
    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer;
    Kernel::TParameterHandler<CStimulationSet*> op_stimulationSet;

    uint64_t m_stimulationID = 0;
    uint64_t m_actionID      = 0;
};

class CBoxAlgorithmPlayerControllerDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Player Controller"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Controls the player execution"; }
    CString getDetailedDescription() const override { return "Add some settings to configure the way you want to control the player"; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_PlayerController; }
    IPluginObject* create() override { return new CBoxAlgorithmPlayerController; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addSetting("Stimulation name", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Action to perform", PlayerAction, "Pause");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_PlayerControllerDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
