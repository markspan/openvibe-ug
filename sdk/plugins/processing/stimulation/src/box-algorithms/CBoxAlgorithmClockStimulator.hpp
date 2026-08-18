///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmClockStimulator.hpp
/// \brief Classes for the Box Clock stimulator.
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
class CBoxAlgorithmClockStimulator final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return (1LL << 32) * 32; }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ClockStimulator)

protected:
    Toolkit::TStimulationEncoder<CBoxAlgorithmClockStimulator> m_encoder;
    uint64_t m_stimulationID       = 0;
    uint64_t m_lastStimulationDate = 0;
    uint64_t m_lastEndTime         = 0;

    uint64_t m_nSentStimulation  = 0;
    double m_stimulationInterval = 0;
};

class CBoxAlgorithmClockStimulatorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Clock stimulator"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Periodic stimulation generator"; }
    CString getDetailedDescription() const override { return "Triggers stimulation at fixed frequency"; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_ClockStimulator; }
    IPluginObject* create() override { return new CBoxAlgorithmClockStimulator; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Generated stimulations", OV_TypeId_Stimulations);
        prototype.addSetting("Interstimulation interval (in sec)", OV_TypeId_Float, "1.0");
        prototype.addSetting("Stimulation", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ClockStimulatorDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
