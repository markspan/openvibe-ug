///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmASRProcessor.hpp
/// \brief Classes for the box ASR Processor.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 08/12/2020.
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
#include <geometry/artifacts/CASR.hpp>

namespace OpenViBE {
namespace Plugins {
namespace Artifact {
//-------------------------------------------------------------------------------------------------
/// <summary>   The class CBoxAlgorithmASRProcessor describes the box Artifact Subspace Reconstruction (ASR) Processor. </summary>
class CBoxAlgorithmASRProcessor final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ASR_Processor)

protected:
    //***** Codecs *****
    Toolkit::TSignalDecoder<CBoxAlgorithmASRProcessor> m_signalDecoder;             ///< Input Signal Decoder
    Toolkit::TStimulationEncoder<CBoxAlgorithmASRProcessor> m_stimulationEncoder;   ///< Output Stimulation Encoder
    Toolkit::TSignalEncoder<CBoxAlgorithmASRProcessor> m_signalEncoder;             ///< Output Signal Encoder

    //***** Pointers *****
    CMatrix *m_iMatrix              = nullptr,  ///< Input Matrix Pointer
            *m_oMatrix              = nullptr;  ///< Output Matrix Pointer
    CStimulationSet* m_oStimulation = nullptr;  ///< Output Stimulation Pointer

    //***** ASR *****
    std::string m_filename;                     ///< ASR Model Path
    Geometry::CASR m_asr;                       ///< ASR Model
};

//-------------------------------------------------------------------------------------------------
/// <summary>   Descriptor of the box Artifact Subspace Reconstruction (ASR) Processor. </summary>
class CBoxAlgorithmASRProcessorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "ASR Processor"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Artifact Subspace Reconstruction (ASR) Processor."; }
    CString getDetailedDescription() const override { return "Artifact Subspace Reconstruction (ASR) Processor."; }
    CString getCategory() const override { return "Artifact"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_ASR_Processor; }
    IPluginObject* create() override { return new CBoxAlgorithmASRProcessor; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Signal", OV_TypeId_Signal);

        prototype.addOutput("Signal Reconstructed",OV_TypeId_Stimulations);
        prototype.addOutput("Output Signal", OV_TypeId_Signal);

        prototype.addSetting("Filename to load model", OV_TypeId_Filename, "${Player_ScenarioDirectory}/ASR-model.xml");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ASR_Processor_Desc)
};
} // namespace Artifact
}  // namespace Plugins
}  // namespace OpenViBE
