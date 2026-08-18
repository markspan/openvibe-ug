///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmTimeout.hpp
/// \brief Classes for the Box Timeout.
/// \author Jozef Legeny (Inria).
/// \version 1.1.
/// \date 21/03/2013
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

/// <summary> The class CBoxAlgorithmTimeout describes the box Timeout. </summary>
class CBoxAlgorithmTimeout final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(const size_t index) override;
    uint64_t getClockFrequency() override { return 16LL << 32; }
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_Timeout)

protected:
    Toolkit::TStimulationEncoder<CBoxAlgorithmTimeout> m_encoder;

private:
    enum class ETimeoutState { No, Occurred, Sent };

    ETimeoutState m_timeoutState = ETimeoutState::No;
    bool m_isHeaderSent          = false;

    uint64_t m_timeout           = 0;
    uint64_t m_lastTimePolled    = 0;
    uint64_t m_previousTime      = 0;
    uint64_t m_stimulationToSend = 0;
};

/**
 * \class CBoxAlgorithmTimeoutDesc
 * \author Jozef Legény (Inria)
 * \date Thu Mar 21 14:40:23 2013
 * \brief Descriptor of the box Timeout.
 *
 */
class CBoxAlgorithmTimeoutDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Timeout"; }
    CString getAuthorName() const override { return "Jozef Legény"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Sends a stimulation after a period of time without receiving signal"; }

    CString getDetailedDescription() const override
    {
        return "Sends a stimulation after a period of time without receiving signal. Useful for stopping scenarios after hardware disconnection.";
    }

    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return Box_Timeout; }
    IPluginObject* create() override { return new CBoxAlgorithmTimeout; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Stream",OV_TypeId_StreamedMatrix);

        prototype.addOutput("Output Stimulations",OV_TypeId_Stimulations);

        prototype.addSetting("Timeout delay",OV_TypeId_Integer, "5");
        prototype.addSetting("Output Stimulation",OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_TimeoutDesc)
};

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
