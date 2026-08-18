///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmAmplitudeArtifactDetector.hpp
/// \brief Classes for the box Amplitude Artifact Detector.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/08/2019.
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

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace Artifact {

enum class EArtifactAction { Stop, StimulationsOnly, Cutoff, Zero };

inline std::string toString(const EArtifactAction action)
{
    switch (action) {
        case EArtifactAction::Stop: return "Stop";
        case EArtifactAction::StimulationsOnly: return "Stimulations Only";
        case EArtifactAction::Cutoff: return "Cutoff";
        case EArtifactAction::Zero: return "Zero";
    }
    return "Stop";
}

inline EArtifactAction stringToArtifactAction(const std::string& actionStr)
{
    if (actionStr == "Stop") { return EArtifactAction::Stop; }
    if (actionStr == "Stimulations Only") { return EArtifactAction::StimulationsOnly; }
    if (actionStr == "Cutoff") { return EArtifactAction::Cutoff; }
    if (actionStr == "Zero") { return EArtifactAction::Zero; }
    return EArtifactAction::Stop;
}

//-------------------------------------------------------------------------------------------------
/// <summary>   The class CBoxAlgorithmAmplitudeArtifactDetector describes the box Amplitude Artifact Detector. </summary>
class CBoxAlgorithmAmplitudeArtifactDetector final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_Artifact_Amplitude)

protected:
    // Codecs
    Toolkit::TSignalDecoder<CBoxAlgorithmAmplitudeArtifactDetector> m_iSignalDecoder;   ///< Input Signal decoder
    Toolkit::TSignalEncoder<CBoxAlgorithmAmplitudeArtifactDetector> m_oSignalEncoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmAmplitudeArtifactDetector> m_oStimulationEncoder;

    // Stimulations
    uint64_t m_stimId = 0;

    // Matrices
    CMatrix* m_iMatrix = nullptr;   ///< Input Matrix pointer
    CMatrix* m_oMatrix = nullptr;   ///< Output Matrix pointer

    // Settings
    double m_threshold       = 0;           ///< Amplitude max
    EArtifactAction m_action = EArtifactAction::Stop;  ///< Default action to take when threshold is crossed

    // Misc
    size_t m_nEpochs   = 0,         ///< Epoch checked
           m_nArtifact = 0;         ///< Artifact found
};

//-------------------------------------------------------------------------------------------------
/// <summary>   Descriptor of the box Artifact Detector. </summary>
class CBoxAlgorithmAmplitudeArtifactDetectorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Amplitude Artifact Detector"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Detects artifacts based on the amplitude of the signal."; }

    CString getDetailedDescription() const override
    {
        return R"(Sends a stimulation when the signal crosses the absolute value of the threshold. 
You can choose what to do with the signal when this happens:
    - "Stop" will cut the signal from before to after the artifact
    - "Cutoff" will change all the values outside the threshold to the value of the threshold
    - "Zero" will change all the values outside the threshold to 0
    - "Stimulations only" won't change the signal)";
    }

    CString getCategory() const override { return "Artifact"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-no"; }

    CIdentifier getCreatedClass() const override { return Box_Artifact_Amplitude; }
    IPluginObject* create() override { return new CBoxAlgorithmAmplitudeArtifactDetector; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Signal",OV_TypeId_Signal);
        prototype.addOutput("Non-artifact signal",OV_TypeId_Signal);
        prototype.addOutput("Artifact detected", OV_TypeId_Stimulations);

        prototype.addSetting("Threshold (mV)",OV_TypeId_Float, "100");
        prototype.addSetting("Action", TypeId_ArtifactAction, "Stop");
        prototype.addSetting("Artifact detected", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_Artifact_Amplitude_Desc)
};
}  // namespace Artifact
}  // namespace Plugins
}  // namespace OpenViBE
