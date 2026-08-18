///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStreamEndDetector.hpp
/// \brief Classes for the Box Stream End Detector.
/// \author Jozef Legeny (Mensia Technologies).
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
class CBoxAlgorithmStreamEndDetector final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    static CIdentifier InputEBMLId() { return CIdentifier(0, 1); }
    static CIdentifier OutputStimulationsID() { return CIdentifier(1, 1); }
    static CIdentifier SettingStimulationNameID() { return CIdentifier(2, 1); }

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StreamEndDetector)

protected:
    enum class EEndState { WaitingForEnd, EndReceived, StimulationSent, Finished };


    Toolkit::TStreamStructureDecoder<CBoxAlgorithmStreamEndDetector> m_decoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmStreamEndDetector> m_encoder;

    uint64_t m_stimulationID = 0;
    uint64_t m_actionID      = 0;

    uint64_t m_endDate             = 0;
    uint64_t m_currentChunkEndDate = 0;
    uint64_t m_previousTime        = 0;
    size_t m_inputEBMLIdx          = 0;
    size_t m_outputStimulationsIdx = 0;
    bool m_isHeaderSent            = false;
    EEndState m_endState           = EEndState::WaitingForEnd;
};

class CBoxAlgorithmStreamEndDetectorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stream End Detector"; }
    CString getAuthorName() const override { return "Jozef Legeny"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return "Sends a stimulation upon receiving an End chunk"; }
    CString getDetailedDescription() const override { return "Sends a stimulation upon receiving an End chunk"; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_StreamEndDetector; }
    IPluginObject* create() override { return new CBoxAlgorithmStreamEndDetector; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("EBML Stream", OV_TypeId_EBMLStream, CBoxAlgorithmStreamEndDetector::InputEBMLId());
        prototype.addOutput("Output Stimulations", OV_TypeId_Stimulations, CBoxAlgorithmStreamEndDetector::OutputStimulationsID());
        prototype.addSetting("Stimulation name", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00", false,
                             CBoxAlgorithmStreamEndDetector::SettingStimulationNameID());

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StreamEndDetectorDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
